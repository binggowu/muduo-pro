#include <muduo/base/Exception.h>
#include <execinfo.h> // backtrace, backtrace_symbols
#include <stdlib.h>

using namespace muduo;

Exception::Exception(const char *msg) : message_(msg)
{
    fillStackTrace();
}

Exception::Exception(const string &msg) : message_(msg)
{
    fillStackTrace();
}

Exception::~Exception() throw() // 不允许抛出任何异常, 即函数是异常安全的.
{
}

const char *Exception::what() const throw()
{
    return message_.c_str();
}

const char *Exception::stackTrace() const throw()
{
    return stack_.c_str();
}

// 填充函数调用栈到stack中.
void Exception::fillStackTrace()
{
    const int len = 200;
    void *buffer[len];                                   // 用于保存各个栈帧的地址
    int nptrs = ::backtrace(buffer, len);                // 栈回溯, nptrs是实际的栈帧的数量
    char **strings = ::backtrace_symbols(buffer, nptrs); // 根据地址, 转成相应的函数名称
    if (strings)
    {
        for (int i = 0; i < nptrs; ++i)
        {
            // TODO demangle funcion name with abi::__cxa_demangle, 本机测试, g++已经实现了该功能.
            stack_.append(strings[i]);
            stack_.push_back('\n'); // 追加单个字符
        }
        free(strings); // man 3 backtrace_symbols, 看到 DESCRIPTION 第2段:
        // This array is malloc(3)ed by backtrace_symbols(), and must be freed by the caller.
    }
}
