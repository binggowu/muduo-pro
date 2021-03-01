#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <sstream>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI; // POLLPRI: 紧急数据
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

void Channel::tie(const boost::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// update() -> EventLoop::updateChannel() -> EPollPoller::updateChannel()
void Channel::update()
{
    loop_->updateChannel(this);
}

// 从 loop_中移除该 channel
// 调用这个函数之间确保调用disbaleAll()
void Channel::remove()
{
    assert(isNoneEvent());

    loop_->removeChannel(this);
}

// Channel的核心函数, 根据 revents_ 的值分别调用不同的用户回调(read/write/error/close).
// 参数receiveTime: 事件发生的时间
void Channel::handleEvent(Timestamp receiveTime)
{
    boost::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock(); // 如果tie_所指对象存在, weak_ptr 提升 share_ptr; 如果不存在, 返回nullptr.
        if (guard)           // tie_所指对象存在, 即TcpConnection存在.
        {
            handleEventWithGuard(receiveTime); // 连接关闭是可读事件
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据 revents_ 的值分别调用不同的用户回调(read/write/error/close).
// 参数receiveTime: 事件发生的时间
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    eventHandling_ = true;

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) // 客户端主动关闭连接
    {
        if (logHup_)
        {
            LOG_WARN << "Channel::handle_event() POLLHUP";
        }
        if (closeCallback_) // 服务方也关闭连接
            closeCallback_();
    }

    if (revents_ & POLLNVAL) // POLLNVAL: fd没有打开, 或者不是合法的
    {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
            errorCallback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) // POLLRDHUP: 对方调用close()/shutdown()
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }

    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }

    eventHandling_ = false;
}

string Channel::reventsToString() const
{
    std::ostringstream oss;
    oss << fd_ << ": ";
    if (revents_ & POLLIN)
        oss << "IN ";
    if (revents_ & POLLPRI)
        oss << "PRI ";
    if (revents_ & POLLOUT)
        oss << "OUT ";
    if (revents_ & POLLHUP)
        oss << "HUP ";
    if (revents_ & POLLRDHUP)
        oss << "RDHUP ";
    if (revents_ & POLLERR)
        oss << "ERR ";
    if (revents_ & POLLNVAL)
        oss << "NVAL ";

    return oss.str().c_str();
}
