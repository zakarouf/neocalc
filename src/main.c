/* Essential */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <z_/imp/ansi.h>
#include <z_/imp/argparse.h>
#include <z_/imp/print.h>

#include <stdlib.h> // Allocation

/* For REPL */
#include <readline/readline.h>
#include <readline/history.h>
#include <z_/imp/tgprint.h>
#include <z_/types/string.h>

/* Neocalc */
#define NC_IMPLEMENTATION
#include "nc.h"

/**/
#define NC_VERSION  "0.2.1"
#define NC_YEAR     "2022-2023"
#define NC_HOSTPAGE "https://github.com/zakarouf/neocalc"

#define NC_INTRODUCTION\
    "Welcome to neocalc (" NC_HOSTPAGE ")\n"\
    "neocalc " NC_VERSION " by zakarouf, (`/q` to exit or `/h` for help)"\

#define NC_HELP\
    z__ansi_fmt((bold), (underline))\
    "About:\n"\
    z__ansi_fmt((plain))\
    "Neocalc is a small calculator that uses reverse polish notation for its expression\n"\
    "Example:\n"\
    "   (+ 1 2 3)\n"\
    "\n"\
    z__ansi_fmt((bold), (underline))\
    "Syntax:\n"\
    z__ansi_fmt((plain))\
    " " z__ansi_fmt((bold), (underline))\
    "1" z__ansi_fmt((plain)) ". (set <var> <value>) for setting a variable\n "\
    z__ansi_fmt((bold), (underline))\
    "2" z__ansi_fmt((plain)) ". (set @<expr-name> <expr>) for setting a expression\n "\
    z__ansi_fmt((bold), (underline))\
    "3" z__ansi_fmt((plain)) ". (@<expr-name> <arg-0> <arg-1> ... <arg-n>) for calling an expression\n "\
    z__ansi_fmt((bold), (underline))\
    "4" z__ansi_fmt((plain)) ". ($<builtin-name> <arg-0> <arg-1> ... <arg-n>) for calling built-in functions\n"\
    "Example:\n"\
    "   (set x 10)\n"\
    "   (set @sqr (* #0 #0))\n"\
    "       NOTE: `#[0-9]+` is used to getting argument passed by there index\n"\
    "       NOTE: Expression can be nested, so (set @f (/ (@sqr #0) 2)) is a valid\n"\
    "\n"\
    "   (@sqr 3)        => 9\n"\
    "   (@sqr (- x 8))  => 4\n"\
    "\n"\
    z__ansi_fmt((bold), (underline))\
    "REPL:\n"\
    z__ansi_fmt((plain))\
    " /q            Quit\n"\
    " /h            Help\n"\
    " /v            Change Result Variable\n"\
    " /a            List all variables declared\n"\
    " /b            List all built-in functions\n"\
    " /e            List all defined expressions\n"\

#define NC_REPL_PROMT "> "

static
void repl(nc_State *s)
{
    puts(NC_INTRODUCTION);
    z__String res_var = z__String_newFromStr("_", 1);
    nc_State_setvar(s, res_var.data, 0);
    while(true) {
        char *_e = readline(NC_REPL_PROMT);
        if(_e) {
            add_history(_e);
            z__String line = z__String_bind(_e, strlen(_e));

            /* REPL Command */
            if(line.data[0] == '/') {
                switch (line.data[1]) {
                    break; case 'q':{
                        free(_e);
                        goto _L_return;
                    }
                    break; case 'v': {
                        char const *name = z__str_skipget_nonws(line.data + 2, line.lenUsed-2);
                        z__String_replaceStr(&res_var, name, (line.data + line.lenUsed) - name);
                    }
                    break; case 'l': {
                        char const *name = z__str_skipget_nonws(line.data + 2, line.lenUsed-2);
                        nc_runfile(s, name);
                    }
                    break; case 'b': {
                        fputs(z__ansi_fmt((bold), (underline), (cl256_fg, 2))
                                "Built-in Functions:"
                              z__ansi_fmt((plain)) "\n", stdout);
                        nc_printall_builtin(s);
                    }
                    break; case 'e': {
                        fputs(z__ansi_fmt((bold), (underline), (cl256_fg, 2))
                                "Defined Expressions:"
                              z__ansi_fmt((plain)) "\n", stdout);
                        nc_printall_expr(s);
                    }
                    break; case 'a': {
                        fputs(z__ansi_fmt((bold), (underline), (cl256_fg, 2))
                                "Declared Variables:"
                              z__ansi_fmt((plain)) "\n", stdout);
                        nc_printall_var(s);
                    }
                    break; case 'h': {
                        z__fprint(stdout, "%s", NC_HELP);
                    }
                    default: break;
                }
            } else {
                nc_float *res = nc_State_getvar(s, res_var.data);
                if(res) {
                    nc_eval(s, &line, res);
                    z__fprint(stdout,
                        z__ansi_fmt((cl256_fg, 6)) "%s" z__ansi_fmt((plain))
                        " is "
                        z__ansi_fmt((cl256_fg, 2)) "%f" z__ansi_fmt((plain)) "\n"
                            , res_var.data, *res);
                } else {
                    z__fprint(stdout,
                        z__ansi_fmt((bold))
                        "Var `%s` is not set"
                        z__ansi_fmt((plain)) "\n", res_var.data);

                    z__String_replaceStr(&res_var, "_", 1);
                }
            }
            free(_e);
        } else {
            fputs("\n", stdout);
        }
    }

    _L_return:
    z__String_delete(&res_var);
}

int main (z__i32 argc, char const *argv[])
{   
    nc_State *state = nc_State_new();
    extern void nc_State_defmath(nc_State *s);
    nc_State_defmath(state);

    int do_repl = 0, hide_out = 0;

    /* Initialize repl if no arguments are given */
    if(argc < 2) {
        do_repl = 1;
        hide_out = 1;
    }
    z__String cmd = z__String_new(256);
    nc_float res = 0;

    #define cstr(s) (s, sizeof(s)-1)
    z__argp_start(argv, 1, argc) {
        z__argp_ifarg_custom(cstr("--repl")) {
            hide_out = 1;
            do_repl = 1;
        }
        z__argp_elifarg_custom(cstr("-h")) {
            fputs(NC_HELP "\n", stdout);
            goto _L_return;
        } else {
            z__String_replaceStr(&cmd, z__argp_get(), strlen(z__argp_get()));
            nc_eval(state, &cmd, &res);
        }
    }

    if(!hide_out) {
        z__fprint(stdout, "%f\n", res);
    }

    _L_return:
    z__String_delete(&cmd);
    if(do_repl) repl(state);
    nc_State_delete(state);

    return EXIT_SUCCESS;
}

