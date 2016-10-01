#ifndef MCG_H
#define MCG_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "em_arith.h"
#include "em_label.h"
#include "em.h"
#include "em_comp.h"
#include "em_pseu.h"
#include "em_mnem.h"
#include "em_flag.h"
#include "em_ptyp.h"
#include "array.h"
#include "pmap.h"
#include "diagnostics.h"
#include "astring.h"
#include "ir.h"
#include "mcgg.h"
#include "hop.h"

extern char em_pseu[][4];
extern char em_mnem[][4];
extern char em_flag[];

enum {
	SECTION_UNKNOWN = 0,
	SECTION_ROM,
	SECTION_DATA,
	SECTION_BSS,
	SECTION_TEXT
};

struct symbol
{
	const char* name;
	int section;
    struct procedure* proc;
	bool is_defined : 1;
	bool is_exported : 1;
	bool is_proc : 1;
};

enum
{
    PARAM_NONE,
    PARAM_IVALUE,
    PARAM_LVALUE,
    PARAM_BVALUE,
};

struct em
{
    int opcode;
    int paramtype;
    union {
        arith ivalue;
        struct {
            const char* label;
            arith offset;
        } lvalue;
        struct {
            struct basicblock* left;
            struct basicblock* right;
        } bvalue;
    } u;
};
    
struct procedure
{
    const char* name;
    struct basicblock* root_bb;
    size_t nlocals;
    ARRAYOF(struct basicblock) blocks;
};

struct basicblock
{
    const char* name;
    ARRAYOF(struct em) ems;
    ARRAYOF(struct ir) irs;
    ARRAYOF(struct hop) hops;
    bool is_fake : 1;
    bool is_root : 1;
    bool is_terminated : 1;
};

extern const char* aprintf(const char* fmt, ...);
extern void tracef(char k, const char* fmt, ...);
extern bool tracing(char k);

extern void parse_em(void);

extern void symbol_init(void);
extern bool symbol_exists(const char* name);
extern struct symbol* symbol_get(const char* name);
extern void symbol_declare(const char* name, bool is_exported, bool is_proc);

typedef bool symbol_walker_t(struct symbol* symbol, void* user);
extern struct symbol* symbol_walk(symbol_walker_t* walker, void* user);

extern void data_label(const char* name);
extern void data_int(arith data, size_t size, bool is_ro);
extern void data_block(const uint8_t* data, size_t size, bool is_ro);
extern void data_offset(const char* label, arith offset, bool is_ro);
extern void data_bss(arith size, int init);

extern void bb_init(void);
extern struct basicblock* bb_get(const char* name);
extern void bb_alias(struct basicblock* block, const char* name);
extern void bb_print(char k, struct basicblock* block);

extern void tb_filestart(void);
extern void tb_fileend(void);
extern void tb_procedure(struct procedure* proc);
extern void tb_regvar(arith offset, int size, int type, int priority);

extern void pass_convert_stack_ops(struct procedure* proc);
extern void pass_remove_dead_blocks(struct procedure* proc);
extern void pass_eliminate_trivial_blocks(struct procedure* proc);
extern void pass_instruction_selector(struct procedure* proc);
extern void pass_promote_float_ops(struct procedure* proc);

extern void procedure_compile(struct procedure* proc);

#endif

/* vim: set sw=4 ts=4 expandtab : */