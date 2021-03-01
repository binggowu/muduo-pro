#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include <muduo/base/copyable.h>

#include <muduo/net/http/HttpRequest.h>

namespace muduo
{
    namespace net
    {

        // 协议解析类, 没有处理body.
        class HttpContext : public muduo::copyable
        {
        public:
            enum HttpRequestParseState // 请求解析状态
            {
                kExpectRequestLine, // 正处于解析请求行
                kExpectHeaders,     // 正处于解析请求头
                kExpectBody,        // 正处于解析body
                kGotAll,            // 解析完毕
            };

            HttpContext()
                : state_(kExpectRequestLine)
            {
            }

            // default copy-ctor, dtor and assignment are fine

            bool expectRequestLine() const
            {
                return state_ == kExpectRequestLine;
            }

            bool expectHeaders() const
            {
                return state_ == kExpectHeaders;
            }

            bool expectBody() const
            {
                return state_ == kExpectBody;
            }

            bool gotAll() const
            {
                return state_ == kGotAll;
            }

            void receiveRequestLine()
            {
                state_ = kExpectHeaders;
            }

            void receiveHeaders()
            {
                state_ = kGotAll;
            } // FIXME

            // 重置HttpContext状态
            void reset()
            {
                state_ = kExpectRequestLine;
                HttpRequest dummy;
                request_.swap(dummy);
            }

            const HttpRequest &request() const
            {
                return request_;
            }

            HttpRequest &request()
            {
                return request_;
            }

        private:
            HttpRequestParseState state_; // 请求解析状态
            HttpRequest request_;         // http请求
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_HTTP_HTTPCONTEXT_H
