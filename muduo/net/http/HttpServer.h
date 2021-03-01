#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include <muduo/net/TcpServer.h>
#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class HttpRequest;
        class HttpResponse;

        /// A simple embeddable HTTP server designed for report status of a program.
        /// It is not a fully HTTP 1.1 compliant server, but provides minimum features
        /// that can communicate with HttpClient and Web browser.
        /// It is synchronous, just like Java Servlet.
        class HttpServer : boost::noncopyable
        {
        public:
            typedef boost::function<void(const HttpRequest &, HttpResponse *)> HttpCallback;

            HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &name);

            ~HttpServer(); // force out-line dtor, for scoped_ptr members.

            /// Not thread safe, callback be registered before calling start().
            void setHttpCallback(const HttpCallback &cb)
            {
                httpCallback_ = cb;
            }

            // 支持多线程
            void setThreadNum(int numThreads)
            {
                server_.setThreadNum(numThreads);
            }

            void start();

        private:
            // 在onMessage()中调用onRequest(), 在onRequest()中调用httpCallback_()

            void onConnection(const TcpConnectionPtr &conn);
            void onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime);
            void onRequest(const TcpConnectionPtr &, const HttpRequest &); // 处理http请求

            TcpServer server_;
            HttpCallback httpCallback_; // 在处理http请求(调用onRequest)的过程中回调此函数, 对请求进行具体的处理.
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_HTTP_HTTPSERVER_H
