/* Essential */
#include <ctype.h>
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

int nc_eval(
    nc_State *s
  , z__String *nc_cmd
  , z__String *res_var_name
);


#define cstr(s) z__Str(s, sizeof(s)-1)
int main (int argc, char *argv[])
{   
    nc_State *state = nc_State_new();
//    repl(state);
    nc_State_setexpr(state, "inc", cstr("(+ #1 1)"), 1);
    nc_State_setexpr(state, "__main__", cstr("(+ 1 (@inc 1))"), 0);

    nc_eval_expr(state, "__main__", 0, 0);

    nc_State_delete(state);
    return 0;
}

