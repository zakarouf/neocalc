#include <math.h>
#include <z_/prep/map.h>
#define NC_DEVELOPMENT
#include "nc.h"

/* Mafs */
#include <tgmath.h>

/* Name_space */
#define fn(name) zpp__CAT(ncm_, name)
/* Function Define Template */
#define defn(name) static z__f64 fn(name) (nc_State *s, char *rest_expr)

/**
 * To lessen the pain of writing same function defination a dozen times.
 * We use some handy macros to just generate code for us.
 */

/* For Functions that takes single argument */
#define defn_s(name, met)\
    defn(name) {\
        return met(z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret));\
    }
#define defn_s_unwrap(x) defn_s x
#define defn_smap(...) zpp__Args_map(defn_s_unwrap, __VA_ARGS__)

/* For Functions that takes multiple arguments */
#define defn_m(name, met, ...)\
    defn(name) {\
        nc_float *x = &z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret);\
        return met(__VA_ARGS__);\
    }
#define defn_m_unwrap(x) defn_m x
#define defn_mmap(...) zpp__Args_map(defn_m_unwrap, __VA_ARGS__)


#pragma GCC diagnostic ignored "-Wunused-parameter"
defn_smap((sin, sin)
       , (cos, cos)
       , (tan, tan)

       , (asin, asin)
       , (acos, acos)
       , (atan, atan)

       , (exp, exp)
       , (exp2, exp2)

       , (log, log)
       , (log10, log10)
       , (log2, log2))

defn_mmap((mod, fmod, x[0], x[1])
        , (pow, pow, x[0], x[1])
        , (atan2, atan2, x[0], x[1]))


void nc_State_defmath(nc_State *s) {
    #define f(x) nc_State_setfn(s, #x, fn(x));
    #define set(...) zpp__Args_map(f, __VA_ARGS__)
    set(sin, cos, tan, asin, acos, atan,
        exp, exp2, log, log10, log2);

    set(mod, pow, atan2);
}