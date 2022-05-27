/* Extern */
#include <stdio.h>
#include <z_/imp/tgprint.h>
#include <z_/types/base_util.h>
#include <z_/types/contof.h>
#include <z_/types/hashhoyt.h>
#define NC_DEVELOPMENT
#include "nc.h"

/**
 * Print Current Stack Data
 */
void nc_printall_data(nc_State *s)
{
    if(s->logfp == NULL) return;
    nc_floatArr v = s->stacks.v;
    fputs("[ " z__ansi_fmt((cl256_fg, 4)) , s->logfp);
    z__Arr_foreach(i, v){
        fprintf(s->logfp, "%.g ", *i);
    }
    fputs( z__ansi_fmt((plain)) "]\n", s->logfp);
}

/**
 */
void nc_printall_expr(nc_State *s)
{
    if(s->logfp == NULL) return;
    z__HashHoyt_foreach(e, &s->exprs) {
        fprintf(s->logfp, "(%s %s)\n", e->key, e->value.expr.data);
    }
}
/**
 */
void nc_printall_var(nc_State *s)
{
    if(s->logfp == NULL) return;
    z__HashHoyt_foreach(e, &s->vars) {
        fprintf(s->logfp, "(%s %f)\n", e->key,  e->value);
    }
}

/**
 */
void nc_printall_builtin(nc_State *s)
{
    if(s->logfp == NULL) return;
    z__HashHoyt_foreach(e, &s->fns) {
        fprintf(s->logfp, "(%s)\n", e->key);
    }
}

/**
 */
static inline
z__u64 _skip_whitespace(z__String const str, z__u64 idx)
{
    if(idx >= str.lenUsed) return str.lenUsed;
    register z__char *ch = str.data + idx;
    while(*ch == ' '
        ||*ch == '\n'
        ||*ch == '\t') ch++;
    return ch - (str.data);
}

/**
 * Evaluate the minimum requried the number of arguments to be passed when calling the said expression
 */
static
void nc_Expr_set_in(nc_Expr *expr)
{
    z__u32 tok = 0; z__u32 prevtok = 0;

    while(expr->expr.data[tok] != 0) {
        tok = z__String_tok(expr->expr, prevtok, z__Str("#", 1));
        z__u64 _in = atoll(expr->expr.data + tok) + 1;
        if(_in > expr->in) {
            expr->in = _in;
        }
        prevtok = tok;
    }
}

/**
 * Create a new expression
 */
static
nc_Expr nc_Expr_new(const z__Str expr_raw)
{
    nc_Expr expr = {
        .expr = z__String_newFromStr(expr_raw.data, expr_raw.len)
    };
    nc_Expr_set_in(&expr);

    return expr;
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
void nc_Stacks_pop_retpoint(nc_Stacks *st, nc_float res)
{
    st->v.lenUsed = z__Arr_getTop(st->retpoints).ret;
    z__Arr_pop_nocheck(&st->retpoints);
    nc_assert(st->retpoints.lenUsed < st->retpoints.len
            , z__contof(st, nc_State, stacks)->logfp
            , "Irregular Popping of retpoints %d/%d", st->retpoints.lenUsed, st->retpoints.len);
    z__Arr_push(&st->v, res);
}

static
z__u64 nc_Stacks_retdiff(nc_Stacks *st)
{
    return z__Arr_getUsed(st->v) - z__Arr_getTop(st->retpoints).ret;
}

static
void nc_Stacks_push_val(nc_Stacks *st, nc_float val)
{
    z__Arr_push(&st->v, val);
}

nc_State* nc_State_new()
{
    nc_State *s = z__MALLOC(sizeof(*s));
    nc_Stacks_new(&s->stacks);
    z__HashHoyt_new(&s->vars);
    z__HashHoyt_new(&s->exprs);
    z__HashHoyt_new(&s->fns);
    z__Arr_new(&s->estates, 32);
    s->logfp = stdout;

    return s;
}

void nc_State_setlogfile(nc_State *s, FILE *logfp)
{
    s->logfp = logfp;
}

void nc_State_delete(nc_State *s)
{
    nc_Stacks_delete(&s->stacks);
    z__HashHoyt_delete(&s->vars);
    z__HashHoyt_delete(&s->fns);

    z__HashHoyt_foreach(i, &s->exprs) {
        nc_Expr_delete(&i->value);
    }
    z__HashHoyt_delete(&s->exprs);

    z__Arr_delete(&s->estates);
    
    z__FREE(s);
}

void nc_State_setfn(nc_State *s, const char *name, nc_builtin_Fn fn)
{
    z__HashHoyt_set(&s->fns, name, fn);
}

void nc_State_setvar(nc_State *s, const char *name, nc_float val)
{
    z__HashHoyt_set(&s->vars, name, val);
}

nc_float *nc_State_getvar(nc_State *s, const char *name)
{
    nc_float *v = NULL;
    z__HashHoyt_getreff(&s->vars, name, v);
    return v;
}

nc_float nc_State_getval(nc_State *s, const char *name)
{
    nc_float *v = NULL;
    z__HashHoyt_getreff(&s->vars, name, v);
    if(v == NULL) {
        nc_pwarn(s->logfp, "'%s' var does not exist", name);
        return 0;
    }
    return *v;
}

nc_Expr const * nc_State_getexpr(nc_State *s, const char *name)
{
    nc_Expr *expr = NULL;
    z__HashHoyt_getreff(&s->exprs, name, expr);
    if(expr == NULL) {
        nc_perr(s->logfp, "'%s' symbol not found", name);
    }
    return expr;
}

long nc_State_getexpr_id(nc_State *s, const char *name)
{
    long index;
    z__HashHoyt_getidx(&s->exprs, name, &index);
    if(index == -1) {
        nc_perr(s->logfp, "'%s' symbol id not found", name);
        return -1;
    }
    return index;
}

void nc_State_setexpr(nc_State *s, const char *name, const z__Str expr_raw)
{
    nc_Expr *expr;
    z__HashHoyt_getreff(&s->exprs, name, expr);

    if(expr) {
        z__String_replaceStr(&expr->expr, expr_raw.data, expr_raw.len);
        nc_Expr_set_in(expr);
    } else {
        nc_Expr expr_new = nc_Expr_new(expr_raw);
        z__HashHoyt_set(&s->exprs, name, expr_new);
    }
}

static
void nc_State_push_estate(nc_State *s, z__u64 tok, z__u64 prevtok, z__u64 passed,nc_Expr const *expr)
{
    z__Arr_push(&s->estates, (nc_ExprState){
            .tok = tok,
            .prevtok = prevtok,
            .expr = expr,
            .passed = passed,
        });
}

static
void nc_State_pop_estate(nc_State *s)
{
    z__Arr_pop_nocheck(&s->estates);
}

static inline nc_float nc_State_call_fns(nc_State *s, const char *name, char *rest_expr)
{
    nc_builtin_Fn *fn;
    z__HashHoyt_getreff(&s->fns, name, fn);
    if(fn == NULL) {
        nc_perr(s->logfp, "Built-in function `%s` does not exist", name);
        return 0;
        return 0.0;
    }
    return (*fn)(s, rest_expr);
}

static
nc_float nc_list_op(nc_State *state, z__Str op)
{
    nc_Stacks *s = &state->stacks;
    nc_float *start = &z__Arr_getVal(s->v, z__Arr_getTop(s->retpoints).ret);
    nc_float *end = &z__Arr_getTopEmpty(s->v);

    nc_float result = *start;
    start++;

    #define op_action_sin(op, ch)\
        break; case ch:\
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
            if(nc_isdigit(op.data[0]) || op.data[0] == '.') {
                return atof(op.data);
            } else {
                nc_pwarn(
                    state->logfp
                    , "Built-in, Unknown Operator -> \"%s\"", op.data);
                return 0;
            }
        }
    }

    return result;
}

nc_float nc_eval_expr(nc_State *s, long expr_id, nc_float *_pass, z__u64 _passed)
{
    /**
     * Basic parsing macros wrapper to make stuff easier
     */
    #define tok(w) {\
        tok = z__String_tok(expr->expr, tok, z__Str(w, sizeof(w)));\
    }
    #define tokskip(w) {\
        tok = z__String_tokskip(expr->expr, tok, z__Str(w, sizeof(w)));\
    }
    #define tokskip_whitespace() {\
        tok = _skip_whitespace(expr->expr, tok);\
    }
    #define tokset(w) {\
        tok = w;\
    }
    #define toknext(w) tokset(tok + (w))
    #define get(idx) (expr->expr.data[idx])
    #define exprend() (expr->expr.lenUsed <= tok)

    /**
     * State loading and saving
     */
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

    /**
     * For debug purpose
     */
    #if NC_AST
        #define ast_(...) __VA_ARGS__
        #define ast_print(fmt, ...) nc_print(s->logfp, "ast >> " fmt "\n",##__VA_ARGS__)
        #define ast_retpush(type, sz)\
                ast_print("|ret push => %d" , s->stacks.retpoints.lenUsed);

        #define ast_retpop() ast_print("ret pop => %d, brac %llu", s->stacks.retpoints.lenUsed, brac)
        #define ast_est_pop() ast_print("")
        #define ast_est_push(d, v) ast_print("# EST PUSH: %s, %llu", d, v);
        #define ast_data_print(v) nc_printall_data(v)
        #define ast_tgprint(...) z__tgfprint(s->logfp, __VA_ARGS__)
    #else
        #define ast_(...) 
        #define ast_print(...) 
        #define ast_retpush(...) 
        #define ast_retpop(...)
        #define ast_est_pop() 
        #define ast_est_push(...) 
        #define ast_data_print(...) 
        #define ast_tgprint(...) 
 
    #endif

    /**
     * Expressions are evaluated on a single layer without
     * recursive calls or state based function calls.
     * The state is saved and loaded on the run,
     * therefore making nc_eval_expr() independent
     */
    nc_Expr const *expr = &s->exprs.entries[expr_id].value;

    nc_State_push_estate(s, 0, 0, _passed, expr);
    for (size_t i = 0; i < _passed; i++) {
        nc_Stacks_push_val(&s->stacks, _pass[i]);
    }

    z__u64 register tok;
    z__u64 prevtok;
    nc_float res = 0;
    z__u64 brac = 0;

    _L_restart:;
    expr_state_load();

    if(nc_iswhitespace(get(0))) {
        tokskip_whitespace();
    }

    while(!exprend()) {
        ast_tgprint("==>>", (const char *)&get(tok));
        if(get(tok) == '(') {
            brac ++;

            /* Move past the bracket */
            toknext(1);

            /* Skip any whitespaces, if any */
            tokskip_whitespace();

            /* Its a method name, grab it */
            z__char *p = &get(tok);
            prevtok = tok;
            while(!nc_iswhitespace(get(tok))) toknext(1);

            nc_Stacks_push_retpoint(&s->stacks, z__Str(p, tok - prevtok));
            ast_retpush(p, tok - prevtok);

        } else if(get(tok) == ')') {
            /**
             * An expression is evaluated only when closing paren is found
             */

            toknext(1);
            brac --;
            
            /* Evaluate which type of method to use on the data */
            /* Store the method name */
            z__Str op = z__Arr_getTop(s->stacks.retpoints).app;

            /* Call an defined expression */
            if(op.data[0] == '@') {
                expr_state_save();
                z__char ch = op.data[op.len];
                op.data[op.len] = 0;
                z__u64 retdiff = nc_Stacks_retdiff(&s->stacks);
                const nc_Expr *expr_new = nc_State_getexpr(s, op.data+1);

                nc_eval_assert(expr_new
                    , goto _L_at_skip;
                    , s->logfp
                    , "Expression does not exist %s", op.data);

                nc_State_push_estate(s
                        , 0, 0
                        , retdiff
                        , expr_new);

                
                nc_eval_assert(retdiff >= expr_new->in,
                        nc_State_pop_estate(s); goto _L_at_skip;,
                        s->logfp,
                        "Passed In val is less than required in %s:\n"
                        "Passed: %llu\n"
                        "Required: %llu\n"
                        "Called From:\n"
                        "   %s", op.data, retdiff, expr_new->in, expr->expr.data+1);

                ast_est_push(op.data, nc_Stacks_retdiff(&s->stacks));

                _L_at_skip:
                op.data[op.len] = ch;
                goto _L_restart;

            /* Call a built-in function */
            } else if(op.data[0] == '$') {
                z__char ch = op.data[op.len];
                op.data[op.len] = 0;
                res = nc_State_call_fns(s, op.data+1, op.data + op.len + 1);
                op.data[op.len] = ch;
            } else {
                res = nc_list_op(s, op);
            }

            /* Pop all the data in the current expression stack and push the result */
            nc_Stacks_pop_retpoint(&s->stacks, res);
            ast_data_print(s);
            ast_retpop();

            /* If exiting a defined expression then pop current state; and
             * load the previous state,
             * while pushing the result of the current state at top of the previous state */
            if(s->stacks.retpoints.lenUsed) {
                if(!brac) {
                    ast_tgprint("Expr POP:", s->estates.data[s->estates.lenUsed-1].expr->expr);
                    nc_State_pop_estate(s);
                    res = z__Arr_getTop(s->stacks.v);
                    nc_Stacks_pop_retpoint(&s->stacks, res);
                    ast_data_print(s);
                    goto _L_restart;
                }
            }

        /* Push a value of a argument passed to the data stack */
        } else if(get(tok) == '#') {
            toknext(1);
            z__u32 retidx = s->stacks.retpoints.lenUsed - brac - 1;
            _passed = z__Arr_getVal(
                      s->stacks.retpoints
                    , retidx).ret + atoi(&get(tok));
           
            while(nc_isdigit(get(tok))) toknext(1);

            ast_tgprint("-> passed :", _passed, "bracs", brac, "retidx", retidx);
            nc_Stacks_push_val(&s->stacks, s->stacks.v.data[_passed]);
            ast_data_print(s);

        /* Push a value of a variable to the data stack */
        } else if(nc_isidentbegin(get(tok))) {
            z__u64 tmp = tok;
            while(nc_isident(get(tok))) toknext(1);
            z__char ch = get(tok);
            get(tok) = 0;
            nc_Stacks_push_val(&s->stacks, nc_State_getval(s, &get(tmp)));
            ast_data_print(s);
            get(tok) = ch;

        /* Push a literal value to the data stack */
        } else {
            nc_Stacks_push_val(&s->stacks, atof(&get(tok)));
            ast_data_print(s);
            toknext(1);
            while(nc_isdigit(get(tok)) || get(tok) == '.') toknext(1);
        }

        /* Skip whitespace */
        if(nc_iswhitespace(get(tok)))
            tokskip_whitespace();
    }
   
    /* Rewind Stack to be 0 */
    s->stacks.v.lenUsed = 0;
    s->stacks.retpoints.lenUsed = 0;
    s->estates.lenUsed = 0;

    /* Return whatever the result is */
    return res;

    #undef tok
    #undef tokset
    #undef tokskip
    #undef tokskip_whitespace
    #undef get
}

nc_float nc_runfile(nc_State *s, const char *name)
{
    z__String file = z__String_newFromFile(name);
    if(!file.data) {
        nc_perr(s->logfp, "Cannot Load File %s", name);
        return 0;
    }
    z__Arr(z__Vector(char, *start, *end)) cmds;
    z__Arr_new(&cmds, 32);
    z__u32 idx = 0;
    z__i32 brac = 0;
    #define get() file.data[idx]
    while(get() != 0 || brac < 0) {
        if(get() == '(') {
            if(!brac) {
                z__Arr_pushInc(&cmds);
                z__Arr_getTop(cmds).start = &get();
            }
            brac ++;
        } else if (get() == ')') {
            brac --;
            if(!brac) {
                z__Arr_getTop(cmds).end = &get();
            }
        }
        idx ++;
    }

    nc_float res = 0;
    z__Arr_foreach(i, cmds) {
        z__String cmd = z__String_bind(i->start, i->end - i->start+1);
        nc_eval(s, &cmd, &res);
    }
    z__Arr_delete(&cmds);
    z__String_delete(&file);

    return res;

    #undef get
}

int nc_eval(nc_State *s, z__String *cmd, nc_float *res)
{
    /**
     * Parsing utility
     */
    z__String nc_cmd = *cmd;
    #define tok(w) {\
        tok = z__String_tok(\
                nc_cmd, tok, z__Str(w, sizeof(w)));\
    }
    #define tokskip(w) {\
        tok = z__String_tokskip(\
                nc_cmd, tok, z__Str(w, sizeof(w)));\
    }
    #define tokskip_whitespace() {\
        tok = _skip_whitespace(nc_cmd, tok);\
    }
    #define tokset(w) {\
        tok = w;\
    }
    #define toknext(w) tokset(tok + (w))
    #define get(idx) (nc_cmd.data[idx])
   
    z__u64 tok = 0;

    tokskip_whitespace();

    /* Define a variable or an expression */
    if(nc_isparen_open(get(tok))) {
        if(get(tok+1) == 's'
        && get(tok+2) == 'e'
        && get(tok+3) == 't'
        && nc_iswhitespace(get(tok+4))) {
            toknext(5);
            tokskip_whitespace();

            /* Define a expression */
            if(get(tok) == '@') {
                toknext(1);
                char *exprn = &get(tok);
                while(!nc_iswhitespace(get(tok))) toknext(1);
                get(tok) = 0;
                
                toknext(1);
                if(get(tok) != '(') tok("(");

                z__u64 start = tok;
                z__u64 brac = 1;
                while(brac) {
                    if(get(tok) == '(') brac ++;
                    else if(get(tok) == ')') brac --;
                    else if(get(tok) == 0) {
                        nc_perr(s->logfp,
                            "Setting an expression '%s',\n"
                            "in `%s`", exprn, &get(start));
                        return -1;
                    }
                    toknext(1);
                }

                nc_State_setexpr(s, exprn, z__Str(&get(start), tok - start - 1));

            /* Define a variable */
            } else if (nc_isidentbegin(get(tok))) {
                char const *name = &get(tok);
                while(nc_isident(get(tok))) {
                    toknext(1);
                }
                get(tok) = 0;
                toknext(1);
                tokskip_whitespace();
                
                /* Its Float literal */
                if(nc_isfloat(get(tok))) {
                    nc_State_setvar(s, name, atof(&get(tok)));

                /* Its an expression, 
                 * we evaluate the expression and store the result as is */
                } else if(get(tok) == '(') {

                    /* Fetch the expression */
                    char *exprn = &get(tok);
                    toknext(1);
                    z__u64 brac = 1;
                    while(brac) {
                        if(get(tok) == '(') brac ++;
                        else if(get(tok) == ')') brac --;
                        else if(get(tok) == 0) {
                            nc_perr(s->logfp,
                                "For evaluating '%s',\n  trailing brackets in expression %s"
                                , name, exprn);
                            return -2;
                        } 
                        toknext(1);
                    }

                    /* Set the expression */
                    nc_State_setexpr(s, "__main__", z__Str(exprn, &get(tok) - exprn));
                   
                    /* Check for reapeated calls */
                    z__i32 repeat = 1;
                    toknext(1);
                    tokskip_whitespace();
                    if(nc_isidentbegin(get(tok))) {
                        z__char *ident = &get(tok);
                        while(nc_isident(get(tok))) toknext(1);
                        z__char ch = get(tok);
                        get(tok) = 0;
                        repeat = nc_State_getval(s, ident);
                        get(tok) = ch;
                    } else if(nc_isdigit(get(tok))) {
                        repeat = atoi(&get(tok));
                    }

                    /* Evaluate and store */
                    nc_float *v = nc_State_getvar(s, name);
                    if(!v) {
                        nc_State_setvar(s, name, 0);
                        v = nc_State_getvar(s, name);
                    }
                    long main_id = nc_State_getexpr_id(s, "__main__");
                    for(;repeat > 0; repeat--) {
                        *v = nc_eval_expr(s, main_id, 0, 0);
                    }

                } else if(nc_isidentbegin(get(tok))) {
                    char const *vname = &get(tok);
                    tok(")");
                    get(tok-1) = 0;
                    nc_State_setvar(s, name, nc_State_getval(s, vname));
                } else {
                    return -3;
                }

            /* set but what? no */
            } else {
                nc_perr(s->logfp, 
                    "Invalid name\n"
                    "  (set %s\n"
                    "       ^~~~~~~~", &get(tok));
                return -2;
            }
        
        /* Load a file */
        } else if(get(tok + 1) == 'l'
               && get(tok + 2) == 'o'
               && get(tok + 3) == 'a'
               && get(tok + 4) == 'd'
               && nc_iswhitespace(get(tok + 5))) {
            toknext(6);
            tokskip_whitespace();

            char *name = &get(tok);
            tok("\t\n)");
            toknext(-1);
            get(tok) = 0;

            nc_runfile(s, name);

        /* Whatever is passed starts with a '(' so as a last resort, evaluate it as an expression */
        } else {
            nc_State_setexpr(s, "__main__", z__Str(nc_cmd.data, nc_cmd.lenUsed));
            *res = nc_eval_expr(s, nc_State_getexpr_id(s, "__main__"), 0, 0);
        }

    /* We got a number */
    } else if(nc_isfloat(get(tok))) {
        *res = atof(&get(tok));

    /* We got a variable */
    } else if(nc_isidentbegin(get(tok))) {
        *res = nc_State_getval(s, &get(tok));
    }
    
    return 0;
}
