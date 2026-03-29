#include <vector>
#include <stdint.h>
#include <cassert>
#include <string>
#include <cstring>
#include <iostream>

#define BUFFER_DEFAULT_SIZE 1024
class Buffer
{
private:
    uint64_t _read_idx;
    uint64_t _write_idx;
    std::vector<char> _buffer;

private:
    char *Begin() { return &*_buffer.begin(); }

public:
    Buffer() : _read_idx(0), _write_idx(0), _buffer(BUFFER_DEFAULT_SIZE) {}
    char *WritePosition() { return Begin() + _write_idx; }
    char *ReadPosition() { return Begin() + _read_idx; }
    uint64_t TailIdleSize() const { return _buffer.size() - _write_idx; }
    uint64_t HeadIdleSize() const { return _read_idx; }
    uint64_t ReadAbleSize() const { return _write_idx - _read_idx; }
    void MoveReadOffset(const uint64_t &len)
    {
        assert(len <= ReadAbleSize());
        _read_idx += len;
    }
    void MoveWriteOffset(const uint64_t &len)
    {
        assert(len <= TailIdleSize());
        _write_idx += len;
    }
    void EnsureWriteSpace(const uint64_t &len)
    {
        if (TailIdleSize() >= len)
            return;
        if (TailIdleSize() + HeadIdleSize() >= len)
        {
            uint64_t rsz = ReadAbleSize();
            std::copy(ReadPosition(), ReadPosition() + rsz, Begin());
            _write_idx = rsz;
            _read_idx = 0;
            return;
        }
        else
        {
            _buffer.resize(_write_idx + len);
            return;
        }
    }
    void Write(const void *data, const uint64_t &len)
    {
        EnsureWriteSpace(len);
        const char *d = (const char *)data;
        std::copy(d, d + len, WritePosition());
    }
    void WriteString(const std::string &data)
    {
        Write(data.c_str(), data.size());
    }
    void WriteBuffer(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
    }
    void WriteAndMove(const void *data, const uint64_t &len)
    {
        Write(data, len);
        MoveWriteOffset(len);
    }
    void WriteStringAndMove(const std::string &data)
    {
        Write(&data[0], data.size());
        MoveWriteOffset(data.size());
    }
    void WriteBufferAndMove(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
        MoveWriteOffset(data.ReadAbleSize());
    }
    void Read(void *buf, const uint64_t &len)
    {
        assert(len <= ReadAbleSize());
        std::copy(ReadPosition(), ReadPosition() + len, (char *)buf);
    }
    std::string ReadAsString(const uint64_t &len)
    {
        assert(len <= ReadAbleSize());
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }
    void ReadAndMove(void *buf, const uint64_t &len)
    {
        Read(buf, len);
        MoveReadOffset(len);
    }
    std::string ReadAsStringAndMove(const uint64_t &len)
    {
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        MoveReadOffset(len);
        return str;
    }
    char *FindCRLF()
    {
        void *ret = memchr(ReadPosition(), '\n', ReadAbleSize());
        return (char *)ret;
    }

    std::string GetLine()
    {
        char *pos = FindCRLF();
        if (pos == NULL)
        {
            return "";
        }
        return ReadAsString(pos - ReadPosition() + 1);
    }

    std::string GetLineAndMove()
    {
        std::string str = GetLine();
        MoveReadOffset(str.size());
        return str;
    }
    void Clear()
    {
        _write_idx = 0;
        _read_idx = 0;
    }
};