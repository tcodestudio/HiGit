//
// Created on 2024/10/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#ifndef TEST_RAW_H
#define TEST_RAW_H
#include <hilog/log.h>
#include <memory>
#include <rawfile/raw_file_manager.h>

struct FileData {
    std::unique_ptr<uint8_t[]> data;
    long length;
};

class Raw {
public:
    Raw(NativeResourceManager *nativeResourceManager) : nativeResourceManager_(nativeResourceManager){};
    ~Raw() { OH_ResourceManager_ReleaseNativeResourceManager(nativeResourceManager_); };

    FileData readAll(const char *filename) {
        RawFile *rawFile = OH_ResourceManager_OpenRawFile(nativeResourceManager_, filename);
        if (rawFile == nullptr) {
            OH_LOG_ERROR(LOG_APP, "Raw OH_ResourceManager_OpenRawFile error %{public}s", filename);
            return {nullptr, 0}; // 返回空的 FileData 结构
        }

        long len = OH_ResourceManager_GetRawFileSize(rawFile);
        if (len <= 0) {
            OH_ResourceManager_CloseRawFile(rawFile);
            return {nullptr, 0}; // 文件长度无效，返回空的 FileData 结构
        }

        std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(len);

        int res = OH_ResourceManager_ReadRawFile(rawFile, data.get(), len);
        if (res < 0) {
            OH_LOG_ERROR(LOG_APP, "Raw OH_ResourceManager_ReadRawFile error");
            OH_ResourceManager_CloseRawFile(rawFile);
            return {nullptr, 0}; // 读取失败，返回空的 FileData 结构
        }

        OH_ResourceManager_CloseRawFile(rawFile);

        // 返回包含数据和长度的 FileData 结构
        return {std::move(data), len};
    }

private:
    NativeResourceManager *nativeResourceManager_;
};
#endif // TEST_RAW_H