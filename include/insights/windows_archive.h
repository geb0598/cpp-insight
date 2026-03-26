#pragma once

#include <cstdio>
#include <filesystem>
#include <system_error>

#include "insights/archive.h"

namespace insights {

// -------------------------------------------------
// WindowsBinaryWriter
// -------------------------------------------------
class WindowsBinaryWriter : public Archive {
public:
    explicit WindowsBinaryWriter(const std::filesystem::path& path)
        : file_(fopen(path.string().c_str(), "wb"))
    {
        if (!file_) {
            throw std::system_error(errno, std::system_category(),
                                    "insights::WindowsBinaryWriter: failed to open '" + path.string() + "'");
        }
    }

    ~WindowsBinaryWriter() { Close(); }

    bool IsLoading() const override { return false; }

    void Serialize(void* data, size_t size) override {
        if (fwrite(data, 1, size, file_) != size) {
            throw std::system_error(errno, std::system_category(),
                                    "insights::WindowsBinaryWriter: fwrite failed");
        }
    }

    void Close() {
        if (file_) {
            fclose(file_);
            file_ = nullptr;
        }
    }

private:
    FILE* file_ = nullptr;
};

// -------------------------------------------------
// WindowsBinaryReader
// -------------------------------------------------
class WindowsBinaryReader : public Archive {
public:
    explicit WindowsBinaryReader(const std::filesystem::path& path)
        : file_(fopen(path.string().c_str(), "rb"))
    {
        if (!file_) {
            throw std::system_error(errno, std::system_category(),
                                    "insights::WindowsBinaryReader: failed to open '" + path.string() + "'");
        }
    }

    ~WindowsBinaryReader() {
        if (file_) { fclose(file_); }
    }

    bool IsLoading() const override { return true; }

    void Serialize(void* data, size_t size) override {
        if (fread(data, 1, size, file_) != size) {
            throw std::system_error(errno, std::system_category(),
                                    "insights::WindowsBinaryReader: fread failed");
        }
    }

    bool IsEof() const { return feof(file_) != 0; }

private:
    FILE* file_ = nullptr;
};

} // namespace insights
