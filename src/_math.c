#include <math.h>
#include <z_/prep/map.h>
#define NC_DEVELOPMENT
#include "nc.h"

/* Mafs */
#include <tgmath.h>

#define fn(name) zpp__CAT(ncm_, name)
#define defn(name) static z__f64 fn(name) (nc_State *s, char *rest_expr)

#pragma GCC diagnostic ignored "-Wunused-parameter"

defn(sin) {
    return sin(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(cos) {
    return cos(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(tan) {
    return tan(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(exp) {
    return exp(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(exp2) {
    return exp2(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(pow) {
    z__f64 *x = &z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret);
    return pow(x[0], x[1]);
}

defn(log) {
    return log(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(log10) {
    return log10(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(mod) {
    z__f64 *x = &z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret);
    return fmod(x[0], x[1]);
}

defn(asin) {
    return asin(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(acos) {
    return acos(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(atan) {
    return atan(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));
}

defn(atan2) {
    z__f64 *x = &z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret);
    return atan2(x[0], x[1]);
}


void nc_State_defmath(nc_State *s) {
    #define f(x) nc_State_setfn(s, #x, fn(x));
    #define set(...) zpp__Args_map(f, __VA_ARGS__)
    set(sin, cos, tan, asin, acos, atan, atan2,
        exp, mod, exp2, log, log10, pow);
}
