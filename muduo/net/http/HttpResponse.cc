#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/Buffer.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void HttpResponse::appendToBuffer(Buffer *output) const
{
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_); // 添加响应头
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if (closeConnection_)
    {
        // 如果是短连接, 不需要告诉浏览器Content-Length, 浏览器也能正确处理. 短连接不存在 粘包 问题.
        output->append("Connection: close\r\n");
    }
    else
    {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size()); // 实体长度
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    // header列表
    for (auto it = headers_.begin(); it != headers_.end(); ++it)
    {
        output->append(it->first);
        output->append(": ");
        output->append(it->second);
        output->append("\r\n");
    }

    output->append("\r\n"); // header与body之间的空行
    output->append(body_);
}
