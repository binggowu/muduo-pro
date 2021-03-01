#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class InetAddress;

        ///
        /// Wrapper of socket file descriptor.
        ///
        /// It closes the sockfd when desctructs.(RAII)
        /// It's thread safe, all operations are delagated to OS.
        class Socket : boost::noncopyable
        {
        public:
            // Socket不负责创建fd(但负责close), 而是接受一个已经创建好的fd.
            explicit Socket(int sockfd)
                : sockfd_(sockfd)
            {
            }

            // Socket(Socket&&) // move constructor in C++11

            ~Socket();

            int fd() const { return sockfd_; }

            void bindAddress(const InetAddress &localaddr);
            void listen();
            int accept(InetAddress *peeraddr);

            void shutdownWrite();

            ///
            /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
            ///
            // Nagle算法: 频繁发送小数据包, 它会做一些延迟, 等收集足够量的数据包之后一起发送, 可以一定程度上避免 网络拥塞, 但造成了延迟. 
            // TCP_NODELAY选项可以禁用Nagle算法: 只有有数据包就发送, 这对于编写低延迟的网络服务很重要.
            void setTcpNoDelay(bool on);

            ///
            /// Enable/disable SO_REUSEADDR
            ///
            void setReuseAddr(bool on);

            ///
            /// Enable/disable SO_KEEPALIVE
            ///
            // TCP keepalive是指定期探测连接是否存在, 如果应用层有心跳的话, 这个选项不是必需要设置的.
            void setKeepAlive(bool on);

        private:
            const int sockfd_;
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_SOCKET_H
