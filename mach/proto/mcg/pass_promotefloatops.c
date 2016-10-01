#include "mcg.h"

static ARRAYOF(struct ir) pending;
static ARRAYOF(struct ir) promotable;

static void addall(struct ir* ir)
{
    if (array_appendu(&pending, ir))
        return;

    if (ir->left)
        addall(ir->left);
    if (ir->right)
        addall(ir->right);
}

static void collect_irs(struct procedure* proc)
{
    int i;
    
    pending.count = 0;
    promotable.count = 0;
	for (i=0; i<proc->blocks.count; i++)
    {
        struct basicblock* bb = proc->blocks.item[i];
        int j;

        for (j=0; j<bb->irs.count; j++)
            addall(bb->irs.item[j]);
    }
}

static void promote(struct ir* ir)
{
    switch (ir->opcode)
    {
        case IR_CONST:
        case IR_POP:
        case IR_LOAD:
            array_appendu(&promotable, ir);
            break;

        case IR_PHI:
            if (!array_appendu(&promotable, ir))
            {
                if (ir->left)
                    promote(ir->left);
                if (ir->right)
                    promote(ir->right);
            }
            break;
    }
}

static void search_for_promotable_irs(void)
{
    int i;

    for (i=0; i<pending.count; i++)
    {
        struct ir* ir = pending.item[i];

        switch (ir->opcode)
        {
            case IR_ADDF:
            case IR_SUBF:
            case IR_MULF:
            case IR_DIVF:
            case IR_NEGF:
                if (ir->left)
                    promote(ir->left);
                if (ir->right)
                    promote(ir->right);
                break;
        }
    }
}

static void modify_promotable_irs(void)
{
    int i;

    for (i=0; i<promotable.count; i++)
    {
        struct ir* ir = promotable.item[i];
        
        if (ir->opcode != IR_PHI)
            ir->opcode++;
    }
}

void pass_promote_float_ops(struct procedure* proc)
{
	collect_irs(proc);
    search_for_promotable_irs();
    modify_promotable_irs();
}

/* vim: set sw=4 ts=4 expandtab : */
