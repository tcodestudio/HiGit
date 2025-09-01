//
// Created on 2024/10/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef TEST_UTILS_CPP_H
#define TEST_UTILS_CPP_H
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <core.h>
#include <cstdint>
#include <fstream>
#include <hilog/log.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <optional>
#include <repo_manager.h>
#include <sstream>


namespace Utils {

constexpr const char *FILES_DIR = "/data/storage/el2/base/haps/entry/files";

[[nodiscard]] inline bool CheckXComponentResult(int32_t result, char const *from, char const *message) noexcept {
    if (result == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        return true;
    }

    static std::unordered_map<int32_t, char const *> const mapper{
        {OH_NATIVEXCOMPONENT_RESULT_FAILED, "OH_NATIVEXCOMPONENT_RESULT_FAILED"},
        {OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER, "OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER"}};

    if (auto const findResult = mapper.find(result); findResult != mapper.cend()) {
        OH_LOG_ERROR(LOG_APP, "%{public}s - %{public}s. Error %{public}s", from, message, findResult->second);
        return false;
    }

    OH_LOG_ERROR(LOG_APP, "%{public}s - %{public}s. Unknown error code %{public}d", from, message, result);
    return false;
}

[[nodiscard]] inline std::string getFilesPath(const std::string &path) {
    if (!path.empty() && path.front() == '/') {
        return std::string(FILES_DIR) + path;
    }
    return std::string(FILES_DIR) + "/" + path;
}

inline std::string getFileExtension(const std::string &path) {
    std::filesystem::path pathObj(path);
    if (pathObj.has_extension()) {
        return pathObj.extension().string();
    }
    return "";
}

inline std::string replaceFileExtension(const std::string &path, const std::string &newExtension) {
    std::filesystem::path pathObj(path);
    if (pathObj.has_extension()) {
        pathObj.replace_extension(newExtension);
    } else {
        pathObj += newExtension;
    }

    return pathObj.string();
}

[[nodiscard]] inline std::ifstream::pos_type getFileSize(const char *filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
};

constexpr float degToRad(float degrees) { return degrees * (M_PI / 180.0f); }

// 检查API接过
[[nodiscard]] inline bool checkNAPIResult(napi_status result, napi_env env, char const *from,
                                          char const *message) noexcept {
    if (result == napi_ok) {
        return true;
    }

    napi_extended_error_info const *info;
    napi_get_last_error_info(env, &info);

    OH_LOG_ERROR(LOG_APP, "From %{public}s %{public}s JS message %{public}s Engine error code %{public}u", from,
                 message, info->error_message, info->engine_error_code);

    return false;
}

// 检查是否是数字
[[nodiscard]] inline bool isNumber(napi_env env, napi_value v, char const *errorMessage, char const *from) noexcept {
    napi_valuetype type;

    // 调用 napi_typeof 获取 v 的类型
    if (!checkNAPIResult(napi_typeof(env, v, &type), env, from, errorMessage)) {
        return false;
    }

    // 检查是否是数字类型
    if (type == napi_number) {
        return true;
    }

    // 记录日志并返回 false
    OH_LOG_ERROR(LOG_APP, "%{public}s - %{public}s", from, errorMessage);
    return false;
}

[[nodiscard]] inline bool isString(napi_env env, napi_value v, char const *errorMessage, char const *from) noexcept {
    napi_valuetype type;

    // 调用 napi_typeof 获取 v 的类型
    if (!checkNAPIResult(napi_typeof(env, v, &type), env, from, errorMessage)) {
        return false;
    }

    // 检查是否是数字类型
    if (type == napi_string) {
        return true;
    }

    // 记录日志并返回 false
    OH_LOG_ERROR(LOG_APP, "%{public}s - %{public}s", from, errorMessage);
    return false;
}

[[nodiscard]] inline bool isBoolean(napi_env env, napi_value v, char const *errorMessage, char const *from) noexcept {
    napi_valuetype type;

    // 调用 napi_typeof 获取 v 的类型
    if (!checkNAPIResult(napi_typeof(env, v, &type), env, from, errorMessage)) {
        return false;
    }

    // 检查是否是数字类型
    if (type == napi_boolean) {
        return true;
    }

    // 记录日志并返回 false
    OH_LOG_ERROR(LOG_APP, "%{public}s - %{public}s", from, errorMessage);
    return false;
}

// 提取数字
[[nodiscard]] inline std::optional<int32_t> extractInteger(napi_env env, napi_value v, char const *errorMessage,
                                                           char const *from) noexcept {
    if (!isNumber(env, v, errorMessage, from)) {
        return std::nullopt;
    }

    int32_t value;

    if (checkNAPIResult(napi_get_value_int32(env, v, &value), env, from, errorMessage)) {
        return value;
    }

    return std::nullopt;
}

[[nodiscard]] inline std::optional<double> extractDouble(napi_env env, napi_value v, char const *errorMessage,
                                                         char const *from) noexcept {
    if (!isNumber(env, v, errorMessage, from)) {
        return std::nullopt;
    }

    double value;

    if (checkNAPIResult(napi_get_value_double(env, v, &value), env, from, errorMessage)) {
        return value;
    }

    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string> extractString(napi_env env, napi_value v, char const *errorMessage,
                                                              char const *from) noexcept {
    if (!isString(env, v, errorMessage, from)) {
        return std::nullopt;
    }

    // 获取字符串长度
    size_t strLength;
    if (!checkNAPIResult(napi_get_value_string_utf8(env, v, nullptr, 0, &strLength), env, from, errorMessage)) {
        return std::nullopt;
    }

    char *buf = new char[strLength + 1];
    std::memset(buf, 0, strLength + 1);
    size_t result = 0;
    if (checkNAPIResult(napi_get_value_string_utf8(env, v, buf, strLength + 1, &result), env, from, errorMessage)) {
        std::string data(buf);
        delete[] buf;
        return data;
    }
    delete[] buf;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<bool> extractBoolean(napi_env env, napi_value v, char const *errorMessage,
                                                        char const *from) noexcept {

    if (!isBoolean(env, v, errorMessage, from)) {
        return std::nullopt;
    }

    bool value;

    if (checkNAPIResult(napi_get_value_bool(env, v, &value), env, from, errorMessage)) {
        return value;
    }

    return std::nullopt;
}

// 提取参数,并且检查是否符合长度
[[nodiscard]] inline bool extractParameters(napi_env env, napi_callback_info info, const size_t expected, size_t *argc,
                                            napi_value *argv, const char *from) {
    bool const result = Utils::checkNAPIResult(napi_get_cb_info(env, info, argc, argv, nullptr, nullptr), env, from,
                                               "Can't extract arguments");
    if (!result) {
        return false;
    }

    if (*argc != expected) {
        OH_LOG_ERROR(LOG_APP, "%{public}s - Parameters expected: %{public}zu, got: %{public}zu", from, expected, *argc);
        return false;
    }

    return true;
}

[[nodiscard]] inline RepoManager *FindRepoManager(napi_env env, napi_callback_info info, const char *from) {
    constexpr size_t expectedParams = 1U;
    constexpr size_t repoURLIdx = 0U;

    size_t argc = expectedParams;

    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const repoManager = Core::GetInstance()->FindRepoManager(repoURL.value());
    if (repoManager == nullptr) {
        OH_LOG_ERROR(LOG_APP, "RepoManager not found for url: %{public}s", repoURL.value().c_str());
        return nullptr;
    }

    return repoManager;
}


} // namespace Utils

#endif // TEST_UTILS_CPP_H
