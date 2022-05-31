#ifndef ZAKAROUF_NEOCALC_MATH_H
#define ZAKAROUF_NEOCALC_MATH_H

#if defined(NC_MATH_IMPLEMENTATION) && !defined(NC_DEVELOPMENT)
#define NC_DEVELOPMENT
#endif

#include "nc.h"
void nc_State_defmath(nc_State *s);

#ifdef NC_MATH_IMPLEMENTATION
/* Mafs */
#include <tgmath.h>

/* Macro stuff */
#include <z_/prep/map.h>
#include <z_/prep/args.h>

/* Name_space */
#define fn(name) zpp__CAT(ncm_, name)
/* Function Define Template */
#define defn(name) static z__f64 fn(name) (nc_State *s, char *rest_expr)
/* Get start */
#define _start_ z__Arr_getVal(s->stacks.v, z__Arr_getTop(s->stacks.retpoints).ret)
#define _end_ z__Arr_getTopEmpty(s->stacks.v)

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
defn_smap(
    (sin, sin)
  , (cos, cos)
  , (tan, tan)

  , (asin, asin)
  , (acos, acos)
  , (atan, atan)

  , (sinh, sinh)
  , (cosh, cosh)
  , (tanh, tanh)

  , (asinh, asinh)
  , (acosh, acosh)
  , (atanh, atanh)

  , (exp, exp)
  , (exp2, exp2)
  , (expm1, expm1)

  , (log, log)
  , (log10, log10)
  , (log2, log2)
  , (log1p, log1p)
  , (logb, logb)

  , (sqrt, sqrt)
  , (cbrt, cbrt)
);

defn_mmap(
    (mod, fmod, x[0], x[1])
  , (pow, pow, x[0], x[1])
  , (atan2, atan2, x[0], x[1])
);

defn(max) {
    nc_float *start = &_start_;
    nc_float *end = &_end_;

    nc_float res = *start;
    start += 1;

    while(start < end) {
        res = res > *start ? res : *start;
        start += 1;
    }
    return res;
}

defn(min) {
    nc_float *start = &_start_;
    nc_float *end = &_end_;

    nc_float res = *start;
    start += 1;

    while(start < end) {
        res = res < *start ? res : *start;
        start += 1;
    }
    return res;
}

void nc_State_defmath(nc_State *s)
{
    #define f(x) nc_State_setfn(s, #x, fn(x));
    #define set(...) zpp__Args_map(f, __VA_ARGS__)

    /* Single */
    set(
        sin, cos, tan
      , sinh, cosh, tanh
      , asin, acos, atan
      , asinh, acosh, atanh
      , exp, exp2, expm1
      , log, log10, log2, log1p, logb
      , sqrt, cbrt
    );

    /* Two */
    set(mod, pow, atan2);

    /* ... */
    set(max, min);
}

#endif // NC_MATH_IMPLEMENTATION

#endif

