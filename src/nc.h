#ifndef ZAKAROUF_NEOCALC_H
#define ZAKAROUF_NEOCALC_H

#include <z_/types/string.h>

typedef struct nc_State nc_State;
typedef double nc_float;

nc_State* nc_State_new(void);
void nc_State_delete(nc_State *s);

void nc_State_setvar(nc_State *s, const char *name, nc_float val);
void nc_State_setexpr(nc_State *s, const char *name, const z__Str expr_raw);
nc_float nc_State_getval(nc_State *s, const char *name);
nc_float *nc_State_getvar(nc_State *s, const char *name);
long nc_State_getexpr_id(nc_State *s, const char *name);

nc_float nc_eval_expr(nc_State *s, long expr_id, nc_float *_pass, z__u64 _passed);
int nc_eval(nc_State *s, z__String *nc_cmd, nc_float *res);
nc_float nc_runfile(nc_State *s, const char *name);

void nc_printall_data(nc_State *s);
void nc_printall_var(nc_State *s);
void nc_printall_expr(nc_State *s);
void nc_printall_builtin(nc_State *s);

#ifdef NC_USE_LOG
#include <stdio.h>
void nc_State_setlogfile(FILE *logfp);
#endif

#ifdef NC_DEVELOPMENT
/* Standard Library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Basic Collection Types */
#include <z_/types/base.h>
#include <z_/types/fnptr.h>
#include <z_/types/string.h>
#include <z_/types/arr.h>
#include <z_/types/hashhoyt.h>
#include <z_/types/contof.h>

/* Printing */
#include <z_/imp/tgprint.h>
#include <z_/imp/print.h>

/* String Attributes (bold, color, etc...) for ansi terminal */
#include <z_/imp/ansi.h>

/* Assertion */
#include <z_/prep/nm/assert.h>

/**
 */
typedef z__Arr(nc_float) nc_floatArr;

/**
 * Expression Type
 * Stores the Expression data; and
 * the number of minimum passed values required
 */
typedef struct nc_Expr {
    z__u64 in;
    z__String expr;
} nc_Expr;

/**
 * State of the expression evaluated;
 */
typedef struct {
    z__u64 tok, prevtok, passed, repeat;
    z__i64 brac;
    const nc_Expr *expr;
} nc_ExprState;
typedef z__Arr(nc_ExprState) nc_ExprStates;

/**
 * Variable Table
 */
typedef z__HashHoyt(nc_float) nc_VarTable;

/**
 * Expression Table
 */
typedef z__HashHoyt(nc_Expr) nc_ExprTable;

/**
 */
typedef nc_float (* nc_builtin_Fn)(nc_State *s, char *rest_expr);
typedef z__HashHoyt(nc_builtin_Fn) nc_builtin_FnTable;

/**
 * Return Points
 */
typedef struct {
    /* Store the method name */
    z__Str app;

    /* Return point */
    z__size ret;
} nc_RetPoint;

/**
 * Single Layered Stack System
 */
typedef struct {
    nc_floatArr v;
    z__Arr( nc_RetPoint ) retpoints;
} nc_Stacks;

/**
 * Entire state of neocalc
 */
typedef struct nc_State {
    /**
     * All the stack data and retpoints and apply methods
     */
    nc_Stacks stacks;

    /**
     * Hash set forming all the defined variables
     */
    nc_VarTable vars;

    /**
     * Hash set containing all defined expression
     */
    nc_ExprTable exprs;

    /**
     * Array Storing the expression state,
     * in order to load and save states between calling a expression
     */
    nc_ExprStates estates;

    /**
     */
    nc_builtin_FnTable fns;

    /**
     * Log File
     * All the log output is send to this file stream
     * If set to `NULL`, no logging is done
     * Default is set to `stdout`
     */
    FILE *logfp;
} nc_State;

/* Utility Functions */
#define nc_print(fp, fmt, ...) (fp?z__fprint(fp, fmt "\n",##__VA_ARGS__):((void)0))

#define nc_perr(fp, fmt, ...)\
    nc_print(fp\
        , z__ansi_fmt((bold), (cl256_fg, 1))\
          "Error:" z__ansi_fmt((plain)) " "\
            fmt, __VA_ARGS__);

#define nc_pwarn(fp, fmt, ...)\
    nc_print(fp\
        , z__ansi_fmt((bold), (cl256_fg, 5))\
          "Warning:" z__ansi_fmt((plain)) " "\
            fmt, __VA_ARGS__);

#if DEBUG == 1
    #define DP(b, f, fmt, ...) logprint(b, f, "DEBUG:: " fmt "\n" ,##__VA_ARGS__)
    #define DEBUG_CHECK(...) __VA_ARGS__
#else 
    #define DP(b, f, fmt, ...)
    #define DEBUG_CHECK(...)
#endif

#define isparen_open(c)  ((c) == '(')
#define isparen_close(c) ((c) == ')')
#define isparen(c)       (isparen_open(c) || isparen_close(c))
#define iswhitespace(c)  ((c) == ' ' || (c) == '\n' || (c) == '\t')
#define isidentbegin(c)  ((c) == '_' || isalpha(c))
#define isident(c)       ((c) == '_' || isalnum(c))
#undef isdigit
#define isdigit(c)       ((c) >= '0' && (c) <= '9')
#define isfloat(c)       (isdigit(c) || (c) == '.' || (c) == '-')

/* Assert Implementation */
#define nc__PRIV__exception(fp, fmt, ext_met, ...)\
    ({ nc_perr(fp, fmt,##__VA_ARGS__) ; ext_met; })

#define nc_assert(exp, fp, fmt, ...) zpp__assert_construct(exp, nc__PRIV__exception, fp, fmt, exit(-1), __VA_ARGS__)
#define nc_eval_assert(exp, ext_met, fp, fmt, ...) zpp__assert_construct(exp, nc__PRIV__exception, fp, fmt, ext_met, __VA_ARGS__)

void nc_State_setfn(nc_State *s, const char *name, nc_builtin_Fn fn);

#endif

#endif

