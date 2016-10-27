#include "mcg.h"

/* mcg stack frames are laid out as:
 *
 * |    ...params...
 * |  --------------- <- ab
 * |      spills
 * |  ---------------
 * |     saved regs
 * |         LR
 * |         FP
 * |  --------------- <- st, fp (a.k.a. lb)
 * |      locals
 * |  --------------- <- sp
 * V  ...user area...
 *
 * st indexes up; lb indexes down.
 *
 * We ensure that dereferencing fp always produces the caller's fp.
 */

static ARRAYOF(struct hreg) saved_regs;

void platform_calculate_offsets(void)
{
    int i;

    saved_regs.count = 0;
    for (i=0; i<current_proc->usedregs.count; i++)
    {
        struct hreg* hreg = current_proc->usedregs.item[i];

        if (!(hreg->attrs & burm_volatile_ATTR) &&
            ((hreg->attrs & burm_int_ATTR) || (hreg->attrs & burm_float_ATTR)))
        {
            current_proc->saved_size += 4;
            array_append(&saved_regs, hreg);
        }
    }

	current_proc->fp_to_st = 0;
	current_proc->fp_to_ab = current_proc->spills_size + current_proc->saved_size + 8;
	current_proc->fp_to_lb = 0;
}

struct hop* platform_prologue(void)
{
    int i;
    int saved_offset;
	struct hop* hop = new_hop(current_proc->entry, NULL);

	hop_add_insel(hop, "! saved_size = %d+8 bytes", current_proc->saved_size);
	hop_add_insel(hop, "! spills_size = %d bytes", current_proc->spills_size);
	hop_add_insel(hop, "! locals_size = %d bytes", current_proc->locals_size);
	hop_add_insel(hop, "addi sp, sp, %d", -(current_proc->fp_to_ab + current_proc->locals_size));
	hop_add_insel(hop, "mfspr r0, lr");

	hop_add_insel(hop, "stw fp, %d(sp)", current_proc->locals_size + 0);
	hop_add_insel(hop, "stw r0, %d(sp)", current_proc->locals_size + 4);
	hop_add_insel(hop, "addi fp, sp, %d", current_proc->locals_size);

    /* Saved reg offsets are negative. */
    saved_offset = current_proc->saved_size + 8;
    for (i=0; i<saved_regs.count; i++)
    {
        struct hreg* hreg = saved_regs.item[i];
        if (hreg->attrs & burm_int_ATTR)
            hop_add_insel(hop, "stw %H, %d(fp)", hreg, saved_offset);
        else if (hreg->attrs & burm_float_ATTR)
            hop_add_insel(hop, "stfs %H, %d(fp)", hreg, saved_offset);
        saved_offset += 4;
    }
	return hop;
}

struct hop* platform_epilogue(void)
{
	struct hop* hop = new_hop(current_proc->exit, NULL);
    int i;
    int saved_offset;

    /* Saved reg offsets are negative. */
    saved_offset = current_proc->saved_size + 8;
    for (i=0; i<saved_regs.count; i++)
    {
        struct hreg* hreg = saved_regs.item[i];
        if (hreg->attrs & burm_int_ATTR)
            hop_add_insel(hop, "lwz %H, %d(fp)", hreg, saved_offset);
        else if (hreg->attrs & burm_float_ATTR)
            hop_add_insel(hop, "lfs %H, %d(fp)", hreg, saved_offset);
        saved_offset += 4;
    }

    hop_add_insel(hop, "lwz r0, 4(fp)");
    hop_add_insel(hop, "mtspr lr, r0");
    hop_add_insel(hop, "lwz r0, 0(fp)"); /* load old fp */
    hop_add_insel(hop, "addi sp, fp, %d", current_proc->fp_to_ab);
    hop_add_insel(hop, "mr fp, r0");
    hop_add_insel(hop, "bclr 20, 0, 0");

	return hop;
}

struct hop* platform_move(struct basicblock* bb, struct hreg* src, struct hreg* dest)
{
    struct hop* hop = new_hop(bb, NULL);

    if ((src->attrs & TYPE_ATTRS) != (dest->attrs & TYPE_ATTRS))
    {
        assert(!src->is_stacked);
        assert(!dest->is_stacked);

        switch (src->attrs & TYPE_ATTRS)
        {
            case burm_int_ATTR:
                hop_add_insel(hop, "stwu %H, -4(sp)", src);
                break;

            case burm_long_ATTR:
                hop_add_insel(hop, "stwu %0H, -4(sp)", src);
                hop_add_insel(hop, "stwu %1H, -4(sp)", src);
                break;

            case burm_float_ATTR:
                hop_add_insel(hop, "stfsu %H, -4(sp)", src);
                break;

            case burm_double_ATTR:
                hop_add_insel(hop, "stfdu %H, -8(sp)", src);
                break;

            default:
                goto nomove;
        }

        switch (dest->attrs & TYPE_ATTRS)
        {
            case burm_int_ATTR:
                hop_add_insel(hop, "lwz %H, 0(sp)", dest);
                break;

            case burm_long_ATTR:
                hop_add_insel(hop, "lwz %0H, 4(sp)", dest);
                hop_add_insel(hop, "lwz %1H, 0(sp)", dest);
                break;

            case burm_float_ATTR:
                hop_add_insel(hop, "lfs %H, 0(sp)", dest);
                break;

            case burm_double_ATTR:
                hop_add_insel(hop, "lfd %H, 0(sp)", dest);
                break;

            default:
                goto nomove;
        }

        switch (dest->attrs & TYPE_ATTRS)
        {
            case burm_int_ATTR:
            case burm_float_ATTR:
                hop_add_insel(hop, "addi sp, sp, 4");
                break;

            case burm_double_ATTR:
            case burm_long_ATTR:
                hop_add_insel(hop, "addi sp, sp, 8");
                break;

            default:
                goto nomove;
        }
    }
    else
    {
        uint32_t type = src->attrs & TYPE_ATTRS;

        if (!src->is_stacked && dest->is_stacked)
        {
            switch (type)
            {
                case burm_int_ATTR:
                    hop_add_insel(hop, "stw %H, %S(fp) ! %H", src, dest, dest);
                    break;

                case burm_float_ATTR:
                    hop_add_insel(hop, "stfs %H, %S(fp) ! %H", src, dest, dest);
                    break;

                case burm_long_ATTR:
                    hop_add_insel(hop, "stw %0H, 4+%S(fp) ! %H", src, dest, dest);
                    hop_add_insel(hop, "stw %1H, 0+%S(fp) ! %H", src, dest, dest);
                    break;

                case burm_double_ATTR:
                    hop_add_insel(hop, "stfd %H, %S(fp) ! %H", src, dest, dest);
                    break;

                default:
                    goto nomove;
            }
        }
        else if (src->is_stacked && !dest->is_stacked)
        {
            switch (type)
            {
                case burm_int_ATTR:
                    hop_add_insel(hop, "lwz %H, %S(fp) ! %H", dest, src, src);
                    break;

                case burm_float_ATTR:
                    hop_add_insel(hop, "lfs %H, %S(fp) ! %H", dest, src, src);
                    break;

                case burm_double_ATTR:
                    hop_add_insel(hop, "lfd %H, %S(fp) ! %H", dest, src, src);
                    break;

                default:
                    goto nomove;
            }
        }
        else if (!src->is_stacked && !dest->is_stacked)
        {
            switch (type)
            {
                case burm_int_ATTR:
                    hop_add_insel(hop, "mr %H, %H", dest, src);
                    break;

                case burm_long_ATTR:
                    hop_add_insel(hop, "mr %0H, %0H", dest, src);
                    hop_add_insel(hop, "mr %1H, %1H", dest, src);
                    break;

                case burm_float_ATTR:
                case burm_double_ATTR:
                    hop_add_insel(hop, "fmr %H, %H", dest, src);
                    break;

                default:
                    goto nomove;
            }
        }
        else
            goto nomove;
    }

    return hop;

nomove:
    fatal("cannot move %s to %s", src->id, dest->id);
}

struct hop* platform_swap(struct basicblock* bb, struct hreg* src, struct hreg* dest)
{
    struct hop* hop = new_hop(bb, NULL);

    assert(!src->is_stacked);
    assert(!dest->is_stacked);
    assert((src->attrs & TYPE_ATTRS) == (dest->attrs & TYPE_ATTRS));
    
    switch (src->attrs & TYPE_ATTRS)
    {
        case burm_int_ATTR:
            hop_add_insel(hop, "mr r0, %H", src);
            hop_add_insel(hop, "mr %H, %H", src, dest);
            hop_add_insel(hop, "mr %H, r0", dest);
            break;

        case burm_long_ATTR:
            hop_add_insel(hop, "mr r0, %0H", src);
            hop_add_insel(hop, "mr %0H, %0H", src, dest);
            hop_add_insel(hop, "mr %0H, r0", dest);

            hop_add_insel(hop, "mr r0, %1H", src);
            hop_add_insel(hop, "mr %1H, %1H", src, dest);
            hop_add_insel(hop, "mr %1H, r0", dest);
            break;

        case burm_float_ATTR:
        case burm_double_ATTR:
            hop_add_insel(hop, "fmr f0, %H", src);
            hop_add_insel(hop, "fmr %H, %H", src, dest);
            hop_add_insel(hop, "fmr %H, f0", dest);
            break;
    }

    return hop;
}

/* vim: set sw=4 ts=4 expandtab : */
