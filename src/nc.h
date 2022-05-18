#ifndef ZAKAROUF_NEOCALC_H
#define ZAKAROUF_NEOCALC_H

#include <z_/types/base.h>
#include <z_/types/string.h>

typedef struct nc_State nc_State;

nc_State* nc_State_new(void);
void nc_State_delete(nc_State *s);

void nc_State_setvar(nc_State *s, const char *name, z__f64 val);
void nc_State_setexpr(nc_State *s, const char *name, const z__Str expr_raw, z__u64 in);
z__f64 nc_State_getval(nc_State *s, const char *name);
z__f64 *nc_State_getvar(nc_State *s, const char *name);

z__f64 nc_eval_expr(nc_State *s, const char *expr_name, z__f64 *_pass, z__u64 _passed);
int nc_eval(nc_State *s, z__String *nc_cmd, z__String *res_name);

#endif

