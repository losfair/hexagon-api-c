#ifndef _HX_IMPORTS_H_
#define _HX_IMPORTS_H_

#define HX_USE_PLATFORM_LIB

#ifdef HX_USE_PLATFORM_LIB
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct sHxOrtExecutor {
    char _;
};

typedef struct sHxOrtExecutor* HxOrtExecutor;

struct sHxOrtFrame {
    char _;
};

typedef struct sHxOrtFrame* HxOrtFrame;

struct sHxOrtExecutorImpl {
    char _;
};

typedef struct sHxOrtExecutorImpl* HxOrtExecutorImpl;

struct sHxOrtFunction {
    char _;
};

typedef struct sHxOrtFunction* HxOrtFunction;

struct rHxOrtValue {
    char _[16];
};

typedef struct rHxOrtValue HxOrtValue;

struct sHxOrtObjectProxy {
    char _;
};

typedef struct sHxOrtObjectProxy* HxOrtObjectProxy;

typedef void (*LocalHxOrtNativeFunction)(HxOrtValue *, HxOrtExecutorImpl, void *);
typedef void (*LocalHxOrtNativeFunctionDestructor)(void *);

typedef int (*HxOrtObjectProxy_OnCall)(HxOrtValue *place, void *data, unsigned int n_args, const HxOrtValue *args);
typedef int (*HxOrtObjectProxy_OnGetField)(HxOrtValue *place, void *data, const char *field_name);

unsigned int hexagon_ort_get_value_size();

void hexagon_glue_destroy_cstring(char *s);

HxOrtExecutor hexagon_ort_executor_create();
void hexagon_ort_executor_destroy(HxOrtExecutor e);
HxOrtExecutorImpl hexagon_ort_executor_get_impl(HxOrtExecutor e);
int hexagon_ort_executor_impl_attach_function(
    HxOrtExecutorImpl e,
    const char *key,
    HxOrtFunction f
);
int hexagon_ort_executor_impl_run_callable(
    HxOrtExecutorImpl e,
    const char *key
);
void hexagon_ort_function_destroy(
    HxOrtFunction f
);
void hexagon_ort_function_enable_optimization(
    HxOrtFunction f
);
HxOrtFunction hexagon_ort_function_load_native(
    LocalHxOrtNativeFunction cb,
    LocalHxOrtNativeFunctionDestructor dtor, // nullable
    void *user_data
);
HxOrtFunction hexagon_ort_function_load_virtual(
    const char *encoding,
    const unsigned char *code,
    unsigned int len
);
void hexagon_ort_executor_impl_get_static_object(
    HxOrtValue *place,
    HxOrtExecutorImpl e,
    const char *key
);
void hexagon_ort_executor_impl_invoke(
    HxOrtValue *place,
    HxOrtExecutorImpl e,
    const HxOrtValue *target,
    const HxOrtValue *this_env,
    const HxOrtValue *args,
    unsigned int n_args
);
void hexagon_ort_executor_pin_object_proxy(
    HxOrtValue *place,
    HxOrtExecutorImpl e,
    HxOrtObjectProxy p
);
void hexagon_ort_value_create_from_null(HxOrtValue *place);
void hexagon_ort_value_create_from_bool(HxOrtValue *place, unsigned int v);
void hexagon_ort_value_create_from_i64(HxOrtValue *place, long long v);
void hexagon_ort_value_create_from_f64(HxOrtValue *place, double v);
int hexagon_ort_value_read_i64(long long *place, const HxOrtValue *v);
int hexagon_ort_value_read_f64(double *place, const HxOrtValue *v);
int hexagon_ort_value_read_null(const HxOrtValue *v);
int hexagon_ort_value_read_bool(int *place, const HxOrtValue *v);
char * hexagon_ort_value_read_string(const HxOrtValue *v, HxOrtExecutorImpl e);
HxOrtObjectProxy hexagon_ort_object_proxy_create(void *data);
void hexagon_ort_object_proxy_destroy(HxOrtObjectProxy p);
void hexagon_ort_object_proxy_set_on_call(HxOrtObjectProxy p, HxOrtObjectProxy_OnCall f);
void hexagon_ort_object_proxy_set_on_get_field(HxOrtObjectProxy p, HxOrtObjectProxy_OnGetField f);

static void __hx_platform_abort(const char *msg) {
#ifdef HX_USE_PLATFORM_LIB
    fprintf(stderr, "%s\n", msg);
    abort();
#else
    volatile int *p = (int *)(~(unsigned long)0);
    *p;
#endif

    while(1) {}
}

// Check struct sizes to ensure correct calling conventions
static void __attribute__((constructor)) __hx_check_cc() {
    if(hexagon_ort_get_value_size() != sizeof(HxOrtValue)) {
        __hx_platform_abort("Value size mismatch");
    }
}

#ifdef __cplusplus
}
#endif

#endif
