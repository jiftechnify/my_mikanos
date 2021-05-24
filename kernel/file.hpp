#pragma once

#include <cstddef>

class FileDescriptor {
  public:
    virtual ~FileDescriptor() = default;
    virtual size_t Read(void *buf, size_t len) = 0;
    virtual size_t Write(const void *buf, size_t len) = 0;
    virtual size_t Size() const = 0;

    // 内部で管理している読み書きオフセットを変更することなく、バッファにファイルの offset 以降のデータを読み込む
    virtual size_t Load(void* buf, size_t len, size_t offset) = 0;
};

size_t PrintToFD(FileDescriptor& fd, const char* format, ...);
