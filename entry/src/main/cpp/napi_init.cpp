#include "napi/native_api.h"
#include "core.h"

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_value const cases[] = {nullptr, exports};
    Core::GetInstance()->SetEnv(env);
    return cases[static_cast<size_t>(Core::InitApp(env, exports))];
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
