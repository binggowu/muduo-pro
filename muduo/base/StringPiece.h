// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// A string like object that points into another piece of memory.
// Useful for providing an interface that allows clients to easily
// pass in either a "const char*" or a "string".
//
// Arghh!  I wish C++ literals were automatically of type "string".

#ifndef MUDUO_BASE_STRINGPIECE_H
#define MUDUO_BASE_STRINGPIECE_H

#include <string.h>
#include <iosfwd> // for ostream forward-declaration

#include <muduo/base/Types.h>
#ifndef MUDUO_STD_STRING
#include <string>
#endif

//

namespace muduo
{
    // 用以实现高效的字符串传递, 即不涉及字符串的拷贝. 具体如下:
    //     要求参数可以是const char*, std::string对象,
    //     void foo(const char *x);        // 当参数为std::string, 需要使用.c_str()方法, 不够直观
    //     void foo(const std::string &x); // 当参数为char *时, 涉及到字符串的拷贝, 不够高效.
    //     void foo(const StringPiect &s); // 即直观, 又高效.
    // 高性能服务器, 每一个环节都是很重要的!
    class StringPiece
    {
    private:
        const char *ptr_; // 字符串地址
        int length_;      // 字符串长度(不包括'\0')

    public:
        StringPiece() : ptr_(NULL), length_(0) {}

        // 单参数构造函数没有使用 explicit 修饰, 参数可以是 const char*, std::string 对象.
        // 使用 std::string 时, 使用了.data()方法, 所以不涉及字符串的拷贝.
        StringPiece(const char *str)
            : ptr_(str),
              length_(static_cast<int>(strlen(ptr_))) {}

        StringPiece(const unsigned char *str)
            : ptr_(reinterpret_cast<const char *>(str)),
              length_(static_cast<int>(strlen(ptr_))) {}

        StringPiece(const string &str)
            : ptr_(str.data()),
              length_(static_cast<int>(str.size())) {}

#ifndef MUDUO_STD_STRING
        StringPiece(const std::string &str)
            : ptr_(str.data()), length_(static_cast<int>(str.size()))
        {
        }
#endif
        StringPiece(const char *offset, int len)
            : ptr_(offset), length_(len)
        {
        }

        // data() may return a pointer to a buffer with embedded NULs, and the
        // returned buffer may or may not be null terminated.  Therefore it is
        // typically a mistake to pass data() to a routine that expects a NUL
        // terminated string.  Use "as_string().c_str()" if you really need to do
        // this.  Or better yet, change your routine so it does not rely on NUL
        // termination.
        const char *data() const { return ptr_; }
        int size() const { return length_; }
        bool empty() const { return length_ == 0; }

        void clear()
        {
            ptr_ = NULL;
            length_ = 0;
        }

        void set(const char *buffer, int len)
        {
            ptr_ = buffer;
            length_ = len;
        }

        void set(const char *str)
        {
            ptr_ = str;
            length_ = static_cast<int>(strlen(str));
        }

        void set(const void *buffer, int len)
        {
            ptr_ = reinterpret_cast<const char *>(buffer);
            length_ = len;
        }

        char operator[](int i) const { return ptr_[i]; }

        void remove_prefix(int n)
        {
            ptr_ += n;
            length_ -= n;
        }

        void remove_suffix(int n)
        {
            length_ -= n;
        }

        // "=="比较的是内容, 与"!="不是相反关系.
        bool operator==(const StringPiece &x) const
        {
            return ((length_ == x.length_) &&
                    (memcmp(ptr_, x.ptr_, length_) == 0));
        }

        // "!="比较的地址, 与"=="不是相反关系.
        bool operator!=(const StringPiece &x) const
        {
            return !(*this == x);
        }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                                \
    bool operator cmp(const StringPiece &x) const                                \
    {                                                                            \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
    }
        STRINGPIECE_BINARY_PREDICATE(<, <);
        STRINGPIECE_BINARY_PREDICATE(<=, <);
        STRINGPIECE_BINARY_PREDICATE(>=, >);
        STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE
        // 上面已经调用过</<=/>=/>了, 在预处理已经展开过了, 即代码已经生成了.

        int compare(const StringPiece &x) const
        {
            int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
            if (r == 0)
            {
                if (length_ < x.length_)
                    r = -1;
                else if (length_ > x.length_)
                    r = +1;
            }
            return r;
        }

        string as_string() const
        {
            return string(data(), size());
        }

        void CopyToString(string *target) const
        {
            target->assign(ptr_, length_);
        }

#ifndef MUDUO_STD_STRING
        void CopyToStdString(std::string *target) const
        {
            target->assign(ptr_, length_);
        }
#endif

        // Does "this" start with "x"
        bool starts_with(const StringPiece &x) const
        {
            return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
        }
    };

} // namespace muduo

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

// 在STL中, 为了提供通用操作而又不损失效率, 需要用到一种 traits 编程技巧.
// traits: 通过定义一些结构体/类, 利用模板特化和偏特化, 给类型赋予一些特性, 这些特性因类型不同而异.

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template <>
struct __type_traits<muduo::StringPiece>  // __type_traits: 模板
{
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream &operator<<(std::ostream &o, const muduo::StringPiece &piece);

#endif // MUDUO_BASE_STRINGPIECE_H
