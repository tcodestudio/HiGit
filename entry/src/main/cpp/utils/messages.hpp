//
// Created on 2024/10/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef TEST_MESSAGES_CPP_H
#define TEST_MESSAGES_CPP_H
#include <cstdint>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <sstream>


namespace Messages {

inline napi_value NewResultMessage(napi_env env, bool success, const std::string &message,
                                   const std::string &data = "") {
    napi_value object = nullptr;
    napi_create_object(env, &object);

    napi_value successName = nullptr;
    napi_create_string_utf8(env, "success", NAPI_AUTO_LENGTH, &successName);
    napi_value successValue = nullptr;
    napi_create_int32(env, success ? 1 : 0, &successValue);
    napi_set_property(env, object, successName, successValue);

    napi_value messageName = nullptr;
    napi_create_string_utf8(env, "message", NAPI_AUTO_LENGTH, &messageName);
    napi_value messageValue = nullptr;
    napi_create_string_utf8(env, message.c_str(), NAPI_AUTO_LENGTH, &messageValue);
    napi_set_property(env, object, messageName, messageValue);

    napi_value dataName = nullptr;
    napi_create_string_utf8(env, "data", NAPI_AUTO_LENGTH, &dataName);
    napi_value dataValue = nullptr;
    napi_create_string_utf8(env, data.c_str(), NAPI_AUTO_LENGTH, &dataValue);
    napi_set_property(env, object, dataName, dataValue);

    return object;
}


} // namespace Messages

#endif // TEST_MESSAGES_CPP_H