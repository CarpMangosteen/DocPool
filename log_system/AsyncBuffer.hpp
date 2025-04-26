#pragma once

#include <vector>

#include "Util.hpp"
#include <cassert>

extern mylog::Util::JsonData *g_conf_data;

namespace mylog
{
    class Buffer
    {
    protected:
        std::vector<char> buffer_;
        size_t write_pos_;
        size_t read_pos_;

    protected:
        void ToBeEnough(size_t len)
        {
            int buffersize = buffer_.size();
            if (len >= WriteableSize())
            {
                if (buffer_.size() < g_conf_data->threshold)
                {
                    buffer_.resize(2 * buffer_.size() + buffersize);
                }
                else
                {
                    buffer_.resize(g_conf_data->linear_growth + buffersize);
                }
            }
        }

    public:
        Buffer() : write_pos_(0), read_pos_(0)
        {
            buffer_.resize(g_conf_data->buffer_size);
        }

        void Push(const char *data, size_t len)
        {
            ToBeEnough(len);
            std::copy(data, data + len, &buffer_[write_pos_]);
            write_pos_ += len;
        }

        char *ReadBegin(int len)
        {
            assert(len <= ReadableSize());
            return &buffer_[read_pos_];
        }

        bool IsEmpty()
        {
            return write_pos_ == read_pos_;
        }

        void Swap(Buffer &buf)
        {
            buffer_.swap(buf.buffer_);
            std::swap(read_pos_, buf.read_pos_);
            std::swap(write_pos_, buf.write_pos_);
        }

        size_t WriteableSize()
        {
            return buffer_.size() - write_pos_;
        }

        size_t ReadableSize()
        {
            return write_pos_ - read_pos_;
        }

        const char *Begin()
        {
            return &buffer_[read_pos_];
        }

        void MoveWritePos(int len)
        {
            assert(len <= WriteableSize());
            write_pos_ += len;
        }

        void MovReadPos(int len)
        {
            assert(len <= ReadableSize());
            read_pos_ += len;
        }

        void Reset()
        {
            write_pos_ = 0;
            read_pos_ = 0;
        }
    };
}//namespace mylog