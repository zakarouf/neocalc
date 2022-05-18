/* Essential */
#include <ctype.h>
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
#include "nc.h"

/**/
#define NC_VERSION  "0.2.0"
#define NC_YEAR     "2022-2023"
#define NC_HOSTPAGE "https://github.com/zakarouf/neocalc"

#define NC_INTRODUCTION\
    "Welcome to neocalc (" NC_HOSTPAGE ")\n"\
    "neocalc " NC_VERSION " by zakarouf " NC_YEAR " (/q to exit)"\

#define NC_REPL_PROMT "> "

static
void repl(nc_State *s)
{
    puts(NC_INTRODUCTION);
    z__String res_var = z__String_newFromStr("x", 1);
    while(true) {
        char *_e = readline(NC_REPL_PROMT);
        if(_e) {
            add_history(_e);
            z__String line = z__String_bind(_e, strlen(_e));
            /**/
            if(line.data[0] == '/') {
                switch (line.data[1]) {
                    break; case 'q': goto _L_return;
                    break; case 'v': {
                        char const *name = z__str_skipget_nonws(line.data + 2, line.lenUsed-2);
                        z__String_replaceStr(&res_var, name, (line.data + line.lenUsed) - name);
                    }
                    default: break;
                }
            } else {
                nc_eval(s, &line, &res_var);
                z__fprint(stdout,
                    z__ansi_fmt((cl256_fg, 6)) "%s" z__ansi_fmt((plain))
                    " is "
                    z__ansi_fmt((cl256_fg, 2)) "%f" z__ansi_fmt((plain)) "\n"
                        , res_var.data, nc_State_getval(s, res_var.data));
            }
            free(_e);
        } else {
            fputs("\n", stdout);
        }
    }

    _L_return:
    z__String_delete(&res_var);
}

#define cstr(s) z__Str(s, sizeof(s)-1)
int main (int argc, char *argv[])
{   
    nc_State *state = nc_State_new();
    repl(state);
    nc_State_delete(state);
    return 0;
}

