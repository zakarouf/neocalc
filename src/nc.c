/* Standard Library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tgmath.h>

/* Basic Collection Types */
#include <z_/types/base.h>
#include <z_/types/string.h>
#include <z_/types/arr.h>
#include <z_/types/hashhoyt.h>

/* Printing */
#include <z_/imp/tgprint.h>
#include <z_/imp/print.h>

/* String Attributes (bold, color, etc...) for ansi terminal */
#include <z_/imp/ansi.h>

/* Assertion */
#include <z_/prep/nm/assert.h>

/* Extern */
#include "nc.h"

/* Utility Functions */
#define logprint(b, f, fmt, ...) z__fprint_cl256(stdout, b, f, fmt "\n" ,##__VA_ARGS__)
#define logfprint(file, b, f, fmt, ...) z__fprint_cl256(file, b, f , fmt "\n" ,##__VA_ARGS__)

#define nc_print(fp, color, fmt, ...) z__fprint_cl256f(fp, color, fmt "\n",##__VA_ARGS__)
#define nc_perr(fp, fmt, ...)\
    z__fprint(fp\
        , z__ansi_fmt((bold), (cl256_fg, 1))\
          "Error:" z__ansi_fmt((plain)) " "\
            fmt "\n", __VA_ARGS__);

#define nc_pwarn(fp, fmt, ...)\
    z__fprint(fp\
        , z__ansi_fmt((bold), (cl256_fg, 5))\
          "Warning:" z__ansi_fmt((plain)) " "\
            fmt "\n", __VA_ARGS__);

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
#define isfloat(c)       (isdigit(c) || (c) == '.' || (c) == '-')

/* Assert Implementation */
#define nc__PRIV__exception(fmt, ext_met, ...)\
    ({ nc_perr(stdout, fmt,##__VA_ARGS__) ; ext_met; })

#define nc_assert(exp, fmt, ...) zpp__assert_construct(exp, nc__PRIV__exception, fmt, exit(-1), __VA_ARGS__)
#define nc_eval_assert(exp, ext_met, fmt, ...) zpp__assert_construct(exp, nc__PRIV__exception, fmt, ext_met, __VA_ARGS__)

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
    z__u64 tok, prevtok, passed;
    z__i64 brac;
    z__f64 result;
    const nc_Expr *expr;
} nc_ExprState;
typedef z__Arr(nc_ExprState) nc_ExprStates;

/**
 * Variable Table
 */
typedef z__HashHoyt(z__f64) nc_VarTable;

/**
 * Expression Table
 */
typedef z__HashHoyt(nc_Expr) nc_ExprTable;

/**
 * Return Points
 */
typedef struct {
    z__Str app;
    z__size ret;
} nc_RetPoint;

/**
 * Single Layered Stack System
 */
typedef struct {
    z__f64Arr v;
    z__Arr( nc_RetPoint ) retpoints;
} nc_Stacks;

/**
 * Entire state of neocalc
 */
typedef struct nc_State {
    nc_Stacks stacks;
    nc_VarTable vars;
    nc_ExprTable exprs;
    nc_ExprStates estates;

    z__f64 *_var_cursor;
} nc_State;

/**
 * Print Current Stack Data
 */
void nc_data_print(nc_State *s)
{
    z__f64Arr v = s->stacks.v;
    fputs("[ " z__ansi_fmt((cl256_fg, 4)) , stdout);
    z__Arr_foreach(i, v){
        fprintf(stdout, "%f ", *i);
    }
    fputs( z__ansi_fmt((plain)) "]\n", stdout);
}

static
nc_Expr nc_Expr_new(const z__Str expr_raw, z__u64 in)
{
    return (nc_Expr){
        .in = in,
        .expr = z__String_newFromStr(expr_raw.data, expr_raw.len)
    };
}

static
void nc_Expr_delete(nc_Expr *e)
{
    z__String_delete(&e->expr);
    memset(e, 0, sizeof(*e));
}

static
void nc_Stacks_new(nc_Stacks *st)
{
    z__Arr_new(&st->v, 128);
    z__Arr_new(&st->retpoints, 32);
}

static
void nc_Stacks_delete(nc_Stacks *st)
{
    z__Arr_delete(&st->v);
    z__Arr_delete(&st->retpoints);
}

static
void nc_Stacks_push_retpoint(nc_Stacks *st, z__Str app)
{
    z__Arr_push(&st->retpoints, (nc_RetPoint){ .ret = st->v.lenUsed, .app = app});
}

static
void nc_Stacks_pop_retpoint(nc_Stacks *st, z__f64 res)
{
    st->v.lenUsed = z__Arr_getTop(st->retpoints).ret;
    z__Arr_pop_nocheck(&st->retpoints);
    nc_assert(st->retpoints.lenUsed < st->retpoints.len
            , "Irregular Popping of retpoints %d/%d", st->retpoints.lenUsed, st->retpoints.len);
    z__Arr_push(&st->v, res);
}

static
z__u64 nc_Stacks_retdiff(nc_Stacks *st)
{
    return z__Arr_getUsed(st->v) - z__Arr_getTop(st->retpoints).ret;
}

static
void nc_Stacks_push_val(nc_Stacks *st, z__f64 val)
{
    z__Arr_push(&st->v, val);
}

nc_State* nc_State_new(void)
{
    nc_State *s = z__MALLOC(sizeof(*s));
    nc_Stacks_new(&s->stacks);
    z__HashHoyt_new(&s->vars);
    z__HashHoyt_new(&s->exprs);
    z__Arr_new(&s->estates, 32);

    nc_State_setvar(s, "__last", 0);
    nc_State_setexpr(s, "__last", z__Str("0", 1), 0);
    return s;
}

void nc_State_delete(nc_State *s)
{
    nc_Stacks_delete(&s->stacks);
    z__HashHoyt_delete(&s->vars);

    z__HashHoyt_foreach(i, &s->exprs) {
        nc_Expr_delete(&i->value);
    }
    z__HashHoyt_delete(&s->exprs);
    z__Arr_delete(&s->estates);
    z__FREE(s);
}

void nc_State_setvar(nc_State *s, const char *name, z__f64 val)
{
    z__HashHoyt_set(&s->vars, name, val);
}

z__f64 *nc_State_getvar(nc_State *s, const char *name)
{
    z__f64 *v = NULL;
    z__HashHoyt_getreff(&s->vars, name, v);
    return v;
}

z__f64 nc_State_getval(nc_State *s, const char *name)
{
    z__f64 *v = NULL;
    z__HashHoyt_getreff(&s->vars, name, v);
    if(v == NULL) {
        nc_pwarn(stdout, "'%s' var does not exist", name);
        return 0;
    }
    return *v;
}

nc_Expr const * nc_State_getexpr(nc_State *s, const char *name)
{
    nc_Expr *expr = NULL;
    z__HashHoyt_getreff(&s->exprs, name, expr);
    if(expr == NULL) {
        nc_perr(stdout, "'%s' symbol not found", name);
    }
    return expr;
}

void nc_State_setexpr(nc_State *s, const char *name, const z__Str expr_raw, z__u64 in)
{
    nc_Expr *expr;
    z__HashHoyt_getreff(&s->exprs, name, expr);
    if(expr) {
        z__String_replaceStr(&expr->expr, expr_raw.data, expr_raw.len);
        expr->in = in;
        return;
    }
    z__HashHoyt_set(&s->exprs, name, nc_Expr_new(expr_raw, in));
}

static
void nc_State_push_estate(nc_State *s, z__u64 tok, z__u64 prevtok, z__u64 result, z__u64 passed,nc_Expr const *expr)
{
    z__Arr_push(&s->estates, (nc_ExprState){
            .tok = tok,
            .prevtok = prevtok,
            .result = result,
            .expr = expr,
            .passed = passed,
        });
}

static
void nc_State_pop_estate(nc_State *s)
{
    z__Arr_pop_nocheck(&s->estates);
}

static
z__f64 nc_Stacks_list_op_action_(nc_Stacks *s, z__Str op)
{
    z__f64 *start = &z__Arr_getVal(s->v, z__Arr_getTop(s->retpoints).ret);
    z__f64 *end = &z__Arr_getTopEmpty(s->v);

    z__f64 result = *start;
#define op_action_sin(op, ch)\
    break; case ch:\
        start ++;\
        while(start < end) {\
            result zpp__CAT(op, =) *start;\
            start ++;\
        }

    switch (op.data[0]) {
        op_action_sin(+, '+');
        op_action_sin(-, '-');
        op_action_sin(*, '*');
        op_action_sin(/, '/');
        break; default: {
            if(isdigit(op.data[0]) || op.data[0] == '.') {
                return atof(op.data);
            } else {
                nc_print(stdout, 1, "# NON A NUMBER -> \"%s\"", op.data);
                return 0;
            }
        }
    }

    return result;
}

z__f64 nc_eval_expr(nc_State *s, const char *expr_name, z__f64 *_pass, z__u64 _passed)
{
    #define tok(w) {\
        prevtok = tok; tok = z__String_tok(expr->expr, prevtok, z__Str(w, sizeof(w)));\
    }
    #define tokskip(w) {\
        prevtok = tok; tok = z__String_tokskip(expr->expr, prevtok, z__Str(w, sizeof(w)));\
    }
    #define tokset(w) {\
        prevtok = tok; tok = w;\
    }
    #define toknext(w) tokset(tok + w)
    #define get(idx) (expr->expr.data[idx])
    #define exprend() (expr->expr.lenUsed <= tok)

    #define expr_state_save(){\
        z__Arr_getTop(s->estates).tok = tok;\
        z__Arr_getTop(s->estates).prevtok = prevtok;\
        z__Arr_getTop(s->estates).brac = brac;\
        z__Arr_getTop(s->estates).expr = expr;\
    }
    #define expr_state_load(){\
        tok = z__Arr_getTop(s->estates).tok;\
        prevtok = z__Arr_getTop(s->estates).prevtok;\
        brac = z__Arr_getTop(s->estates).brac;\
        expr = z__Arr_getTop(s->estates).expr;\
    }

    #ifdef NC_AST
        #define ast_print(fmt, ...) nc_print(2, "ast >> " fmt "\n",##__VA_ARGS__)
        #define ast_retpush(type, sz) fwrite(type, sz, 1, stdout); ast_print("|ret push => %d" , s->stacks.retpoints.lenUsed);
        #define ast_retpop() ast_print("ret pop => %d, brac %llu", s->stacks.retpoints.lenUsed, brac)
        #define ast_est_pop() ast_print("")
        #define ast_est_push(d, v) ast_print("# EST PUSH: %s, %llu", d, v);
        #define ast_data_print(v) nc_data_print(v)
        #define ast_tgprint(...) z__tprint(__VA_ARGS__)
    #else
        #define ast_print(...) 
        #define ast_retpush(...) 
        #define ast_retpop(...)
        #define ast_est_pop() 
        #define ast_est_push(...) 
        #define ast_data_print(...) 
        #define ast_tgprint(...) 
 
    #endif

    nc_State_push_estate(s, 0, 0, 0, _passed, nc_State_getexpr(s, expr_name));
    for (size_t i = 0; i < _passed; i++) {
        nc_Stacks_push_val(&s->stacks, _pass[i]);
    }

    z__u64 tok, prevtok;
    nc_Expr const *expr;
    z__f64 res = 0;
    z__u64 brac = 0;

    _L_restart:;
    expr_state_load();

    if(iswhitespace(get(0))) {
        tokskip(" \n\t");
    }

    while(!exprend()) {
        ast_tgprint("==>>", (const char *)&get(tok));
        if(get(tok) == '(') {
            brac ++;
            toknext(1);
            tokskip(" \n\t");
            z__char *p = &get(tok);
            tok(" \n\t");
            nc_Stacks_push_retpoint(&s->stacks, z__Str(p, tok - prevtok - 1));
            ast_retpush(p, tok - prevtok -1);
        } else if(get(tok) == ')') {
            toknext(1);
            brac --;
            //_L_force_stack_pop:; // unused

            z__Str op = z__Arr_getTop(s->stacks.retpoints).app;
            if(op.data[0] == '@') {
                expr_state_save();
                z__char ch = op.data[op.len];
                op.data[op.len] = 0;
                z__u64 retdiff = nc_Stacks_retdiff(&s->stacks);
                const nc_Expr *expr_new = nc_State_getexpr(s, op.data+1);

                nc_eval_assert(expr_new, goto _L_at_skip;, "Expression does not exist %s", op.data);

                nc_State_push_estate(s
                        , 0, 0, 0
                        , retdiff
                        , expr_new);

                
                nc_eval_assert(retdiff >= expr_new->in,
                        nc_State_pop_estate(s); goto _L_at_skip;,
                        "Passed In val is less than required in %s:\n"
                        "Passed: %llu\n"
                        "Required: %llu\n"
                        "Called From:\n%s", op.data, retdiff, expr_new->in, expr->expr.data);

                ast_est_push(op.data, nc_Stacks_retdiff(&s->stacks));

                _L_at_skip:
                op.data[op.len] = ch;
                goto _L_restart;

            } else {
                res = nc_Stacks_list_op_action_(&s->stacks, op);
            }
            nc_Stacks_pop_retpoint(&s->stacks, res);
            ast_data_print(s);
            ast_retpop();
            if(s->stacks.retpoints.lenUsed) {
                if(!brac) {
                    ast_tgprint("Expr POP:", s->estates.data[s->estates.lenUsed-1].expr->expr);
                    nc_State_pop_estate(s);
                    z__u32 retdiff = nc_Stacks_retdiff(&s->stacks);
                    res = s->stacks.v.data[s->stacks.v.lenUsed - retdiff + 1];
                    nc_Stacks_pop_retpoint(&s->stacks, res);
                    ast_data_print(s);
                    goto _L_restart;
                }
            } 
        } else if(isdigit(get(tok)) || get(tok) == '.' || get(tok) == '-') {
            nc_Stacks_push_val(&s->stacks, atof(&get(tok)));
            ast_data_print(s);
            while(get(tok) == '.' || isdigit(get(tok))) toknext(1);
        } else if(isidentbegin(get(tok))) {
            z__u64 tmp = tok;
            while(isident(get(tok))) toknext(1);
            z__char ch = get(tok);
            get(tok) = 0;
            nc_Stacks_push_val(&s->stacks, nc_State_getval(s, &get(tmp)));
            ast_data_print(s);
            get(tok) = ch;
        } else if(get(tok) == '#') {
            toknext(1);
            while(isident(get(tok))) toknext(1);
            z__char ch = get(tok);
            get(tok) = 0;
            z__u32 idx = atoi(&get(tok));
            _passed =  s->stacks.v.lenUsed - z__Arr_getTop(s->estates).passed + idx;
            ast_tgprint(_passed);
            nc_Stacks_push_val(&s->stacks, s->stacks.v.data[_passed]);
            ast_data_print(s);
            get(tok) = ch;
        } else if(iswhitespace(get(tok))) {
            tok(" \t\n");
        } else {
            toknext(1);
        }

    }
   
    s->stacks.v.lenUsed = 0;
    s->stacks.retpoints.lenUsed = 0;
    return z__Arr_getVal(s->stacks.v, 0);

    #undef tok
    #undef tokskip
    #undef get
}

int nc_eval(nc_State *s, z__String *nc_cmd, z__String *res_name)
{
    #define tok(w) {\
        prevtok = tok; tok = z__String_tok(\
                *nc_cmd, prevtok, z__Str(w, sizeof(w)));\
    }
    #define tokskip(w) {\
        prevtok = tok; tok = z__String_tokskip(\
                *nc_cmd, prevtok, z__Str(w, sizeof(w)));\
    }
    #define tokset(w) {\
        prevtok = tok; tok = w;\
    }
    #define toknext(w) tokset(tok + w)
    #define get(idx) (nc_cmd->data[idx])
   
    z__u64 tok = 0, prevtok = 0;

    tokskip(" \t\n");

    if(isparen_open(get(tok))) {
        if(get(tok+1) == 's'
        && get(tok+2) == 'e'
        && get(tok+3) == 't'
        && iswhitespace(get(tok+4))) {
            toknext(5);
            tokskip(" \n\t");

            if(get(tok) == '@') {
                toknext(1);
                char *exprn = &get(tok);
                while(!iswhitespace(get(tok))) toknext(1);
                get(tok) = 0;
                
                toknext(1);
                tokskip(" \n\t");

                z__u64 in = atof(&get(tok));
                if(get(tok) == '(') tok("(");
                toknext(-1);

                z__u64 start = tok;
                z__u64 brac = 1;
                while(brac) {
                    if(get(tok) == '(') brac ++;
                    else if(get(tok) == ')') brac --;
                    else if(get(tok) == 0) return -1;
                    toknext(1);
                }

                nc_State_setexpr(s, exprn, z__Str(&get(start), tok - start), in);
            } else if (isident(get(tok))) {
                char const *name = &get(tok);
                while(isident(get(tok))) {
                    toknext(1);
                }
                get(tok) = 0;
                toknext(1);
                tokskip(" \n\t");
                
                if(isfloat(get(tok))) {
                    nc_State_setvar(s, name, atof(&get(tok)));
                } else if(get(tok) == '(') {
                    char *exprn = &get(tok);
                    z__u64 brac = 1;
                    while(brac) {
                        if(get(tok) == '(') brac ++;
                        else if(get(tok) == ')') brac --;
                        else if(get(tok) == 0) return -1;
                        toknext(1);
                    }
                    nc_State_setexpr(s, "__main__", z__Str(exprn, &get(tok) - exprn-1), 0);
                    nc_State_setvar(s, name, nc_eval_expr(s, "__main__", 0, 0));
                } else if(isidentbegin(get(tok))) {
                    char const *vname = &get(tok);
                    tok(")");
                    get(tok-1) = 0;
                    nc_State_setvar(s, name, nc_State_getval(s, vname));
                } else {
                    return -3;
                }

            } else {
                return -2;
            }
        } else {
            nc_State_setexpr(s, "__main__", z__Str(nc_cmd->data, nc_cmd->lenUsed), 0);
            nc_State_setvar(s, res_name->data, nc_eval_expr(s, "__main__", 0, 0));
        }
    } else if(isfloat(get(tok))) {
        nc_State_setvar(s, res_name->data, atof(&get(tok)));
    } else if(isidentbegin(get(tok))) {
        nc_State_setvar(s, res_name->data, nc_State_getval(s, &get(tok)));
    }

    return 0;
}

