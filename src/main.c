#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tgmath.h>

#include <z_/types/base.h>
#include <z_/types/string.h>
#include <z_/types/arr.h>
#include <z_/types/hashhoyt.h>
#include <z_/imp/sys.h>

#include <readline/readline.h>
#include <readline/history.h>

#define logprint(b, f, fmt, ...) z__log_cl256_full(b, f, fmt "\n" ,##__VA_ARGS__)
#define logfprint(file, b, f, fmt, ...) z__logfile_cl256_full(file, b, f , fmt "\n" ,##__VA_ARGS__)

#define error_print(fmt, ...) logprint(0, 1, fmt ,##__VA_ARGS__)
#define nc_print(color, fmt, ...) z__log_cl256(color, fmt "\n",##__VA_ARGS__)

#if HW_DEBUG == 1
    #define DP(b, f, fmt, ...) logprint(b, f, "DEBUG:: " fmt "\n" ,##__VA_ARGS__)
    #define DEBUG_CHECK(...) __VA_ARGS__
#else 
    #define DP(b, f, fmt, ...)
    #define DEBUG_CHECK(...)
#endif

#define NC_VERSION  "0.1.0"
#define NC_YEAR     "2022-2023"
#define NC_HOSTPAGE "https://github.com/zakarouf/neocalc"

#define NC_INTRODUCTION\
    "Welcome to neocalc (" NC_HOSTPAGE ")\n"\
    "neocalc " NC_VERSION " by zakarouf " NC_YEAR " (/q to exit)"\

typedef z__f64 nc_Val;

typedef z__HashHoyt(nc_Val) nc_VarsTable;
typedef z__Arr(nc_Val) nc_VarList;
struct nc_VarStack {
    nc_VarList vars;
    char op;
};
typedef z__Arr(struct nc_VarStack) nc_VarStackArr;

#define isparen_open(c)  ((c) == '(')
#define isparen_close(c) ((c) == ')')
#define isparen(c)       (isparen_open(c) || isparen_close(c))
#define iswhitespace(c)  ((c) == ' ' || (c) == '\n' || (c) == '\t')

char* skipws(const char *s) {
    while(*s == ' '
    ||*s == '\n'
    ||*s == '\t') s++;
    return (char *)s;
}

char* gettows(const char *s) {
    while((*s != ' '
    &&*s != '\n'
    &&*s != '\t')
    &&*s != '\0') s++;
    return (char *)s;
}

void nc_VarStackArr_new(nc_VarStackArr *vs, z__size size)
{
    z__Arr_new(vs, size);
}

struct nc_VarStack* nc_VarStackArr_push(nc_VarStackArr *vs, char op)
{
    z__Arr_pushInc(vs);
    struct nc_VarStack *vsa = &z__Arr_getTop((*vs));
    vsa->op = op;
    z__Arr_new(&vsa->vars, 8);
    return vsa;
}

struct nc_VarStack* nc_VarStackArr_pop(nc_VarStackArr *vs)
{
    nc_VarList *vl = &z__Arr_getTop((*vs)).vars;
    z__Arr_delete(vl);
    if(vs->lenUsed > 0) vs->lenUsed -= 1;
    return &z__Arr_getTop((*vs));
}

void nc_VarStackArr_delete(nc_VarStackArr *vs)
{
    for (size_t i = 0; i < z__Arr_getUsed(*vs); i++) {
        nc_VarStackArr_pop(vs);
    }
    z__Arr_delete(vs);
}

nc_Val list_op_add(nc_VarList *vl)
{
    nc_Val tmp = z__Arr_getVal((*vl), 0);
    for (size_t i = 1; i < vl->lenUsed; i++) {
        tmp += z__Arr_getVal((*vl), i);
    }
    return tmp;
}

nc_Val list_op_sub(nc_VarList *vl)
{
    nc_Val tmp = z__Arr_getVal((*vl), 0);
    for (size_t i = 1; i < vl->lenUsed; i++) {
        tmp -= z__Arr_getVal((*vl), i);
    }
    return tmp;
}

nc_Val list_op_mul(nc_VarList *vl)
{
    nc_Val tmp = z__Arr_getVal((*vl), 0);
    for (size_t i = 1; i < vl->lenUsed; i++) {
        tmp *= z__Arr_getVal((*vl), i);
    }
    return tmp;
}

nc_Val list_op_div(nc_VarList *vl)
{
    nc_Val tmp = z__Arr_getVal((*vl), 0);
    for (size_t i = 1; i < vl->lenUsed; i++) {
        tmp /= z__Arr_getVal((*vl), i);
    }
    return tmp;
}

nc_Val list_op(struct nc_VarStack *st)
{
    if(st->vars.lenUsed == 0) {
        return 0;
    }
    switch(st->op) {
        break; case '+': return list_op_add(&st->vars);
        break; case '-': return list_op_sub(&st->vars);
        break; case '*': return list_op_mul(&st->vars);
        break; case '/': return list_op_div(&st->vars);
        default: return 0;
    }

}

nc_Val* nc_get_var_reffval(nc_VarsTable *v, const char *var)
{
    nc_Val *reff;
    z__HashHoyt_getreff(v, var, reff);
    return reff;
}

nc_Val nc_get_var_val(nc_VarsTable *v, const char *var)
{
    nc_Val *reff = nc_get_var_reffval(v, var);
    if(reff) return *reff;
    else {
        error_print("Var `%s` does not exist", var);
    }
    return 0;
}

typedef struct {
    nc_Val val;
    char const *name;
} nc_Expr_Result;

nc_Val eval_expr(nc_VarStackArr *stackArr, nc_VarsTable *v, char *expr, nc_Val lastval)
{

    #define next(i, n) { i += n; if(i >= expr.data+expr.lenUsed) { printf("GONE TO FAR"); exit(-1); }  }
    #define next_tok(i, t) { i = strtok_r(NULL, " \t\n", &t); }

    #if 0
      #define ast_stackpush(s) { printf("|S:%u\n", (s)->lenUsed); }
      #define ast_stackpop(s) { printf("|E:%u\n", (s)->lenUsed); }
      #define ast_op(i) {printf(" |Operator: %c\n", i);}
      #define ast_float(i) {printf("    |Float: %lf\n", i);}
    #else
      #define ast_stackpush(s)
      #define ast_stackpop(s)
      #define ast_op(i) 
      #define ast_float(i)
    #endif
    #if 0
      #define val_print(v) {printf("-|Val:%lf\n", v);}
    #else 
      #define val_print(v)
    #endif

    char *t;
    char *i = strtok_r(expr, " \n\t", &t);
    struct nc_VarStack *stack_cursor = &(stackArr)->data[0];

    if(*i != '(') {
        if(*i == '_' || isalpha(*i)) {
            char *var_name = i++;
            while(isalnum(*i)|| *i == '_') i++;
            char tmp = *i;
            *i = 0;
            
            nc_Val *val = nc_get_var_reffval(v, var_name); 
            *i = tmp;
            return val == NULL? lastval: *val;
        } else if(*i == '.' || isdigit(*i)) {
            nc_Val retval = 0;
            sscanf(i, "%lf", &retval);
            return retval;
        } else {
            if (*i != '\0') goto _L_force_stack_push;
        }
    }

    do {
        _L_recheck:
        if(*i == '(') {
            if(*(i+1) == '\0') {
                next_tok(i, t);
            } else {
                i += 1;
            }
            _L_force_stack_push:
            stack_cursor = nc_VarStackArr_push(stackArr, *i);
            ast_stackpush(stackArr);
            ast_op(*i);
        } else if(*i == ')') {
            if(stackArr->lenUsed == 1) {
                ast_stackpop(stackArr);
                nc_Val tmp = list_op(stack_cursor);
                nc_VarStackArr_pop(stackArr);
                return tmp;
            }

            nc_Val tmp = list_op(stack_cursor);
            stack_cursor = nc_VarStackArr_pop(stackArr);
            z__Arr_push(&stack_cursor->vars, tmp);
            ast_stackpop(stackArr);

            i++;
            if(*i != '\0') goto _L_recheck;

        } else if( isdigit(*i) || *i == '.' || *i == '-') {
            nc_Val tmp;
            if(sscanf(i, "%lf", &tmp)) {
                ast_float(tmp);
                z__Arr_push(&stack_cursor->vars, tmp);
                if(*i == '-') i++;
                while(isdigit(*i) || *i == '.') i++;
            }
            if(*i != '\0') {
                goto _L_recheck;
            }
        } else if( ((*i == '_') || isalpha(*i)) ) {
            char *var_name = i++;
            while(isalnum(*i) || *i == '_') i++;
            char tmp = *i;
            *i = 0;
            
            nc_Val *reff = nc_get_var_reffval(v, var_name);
            if(reff) {
                z__Arr_push(&stack_cursor->vars, *reff);
            }
            
            *i = tmp;
            if(*i) goto _L_recheck;
        }
        next_tok(i, t);

    } while(i != NULL);

    z__size length = z__Arr_getUsed((*stackArr));
    if(length > 0) {
        for (size_t j = 1; j < length; j++) {
            lastval = list_op(stack_cursor);
            stack_cursor = nc_VarStackArr_pop(stackArr);
            printf("- %p|\n", stack_cursor);
            z__Arr_push(&stack_cursor->vars, lastval);
        }
        lastval = list_op(stack_cursor);
        stack_cursor = nc_VarStackArr_pop(stackArr);
    }

    return lastval;
}

void print_vars(nc_VarsTable *v)
{
    for (size_t i = 0; i < v->len; i++) {
        if(v->entries[i].key != NULL) {
            nc_print(2, "%--32s|%23lf|", v->entries[i].key, v->entries[i].value);
        }
    }
}

int main(void)
{
    puts(NC_INTRODUCTION);
    nc_VarsTable *v = z__New0(*v, 1);
    z__HashHoyt_new(v);
    z__String var = z__String_newFromStr("stdout", -1);

    nc_VarStackArr stackArr;
    nc_VarStackArr_new(&stackArr, 8);

    nc_Val retval = 0;

    while(true) {
        char *line = readline("> "); if(!*line) goto _L_skip;
        add_history(line);
        char *expr = line;
        if(expr[0] == '/') {
            switch(expr[1]) {
                break; case 'q': goto _L_return;
                break; case 's': {
                    expr = gettows(expr);
                    expr = skipws(expr);
                    if(expr == 0) goto _L_skip;
                    if(*expr == '_' || isalpha(*expr)) {
                        char *end = expr + 1;
                        while (isalnum(*end) || *end == '_') end++;
                        z__String_replaceStr(&var, expr, end - expr);
                        expr = end;
                    } else {
                        error_print("/s: Not a valid variable name");
                    }
                }
                break; case 'l': print_vars(v); goto _L_skip;
            }
        }

        retval = eval_expr(&stackArr, v, expr, retval);
        z__HashHoyt_set(v, var.data, retval);
        z__HashHoyt_set(v, "_last", retval);

        _L_skip:
        nc_print(5, "%s:%lf ", var.data, retval);
        z__FREE(line);
        z__String_replaceStr(&var, "stdout", sizeof("stdout"));
    }

    _L_return: {
        nc_VarStackArr_delete(&stackArr);
        z__HashHoyt_delete(v);
        z__FREE(v);
        return 0;
    }
}

