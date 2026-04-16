#ifndef __M_CONNECTION_H__
#define __M_CONNECTION_H__
#include "common/common.hpp"
#include "common/logger.hpp"
#include "event_loop.hpp"
#include "socket.hpp"
#include "buffer.hpp"

enum class ConnStatus
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};
class Connection; 
using PtrConnection = std::shared_ptr<Connection>;
class Connection : public std::enable_shared_from_this<Connection>
{
private:
    uint64_t _conn_id;
    int _sockfd;
    EventLoop *_loop;
    Socket _socket;
    ConnStatus _status;
    bool _enable_inactive_release;
    Channel _channel;
    Buffer _in_buffer;
    Buffer _out_buffer;
    std::any _context;

    using ConnectedCallback = std::function<void(const PtrConnection &)>;
    using MessageCallback = std::function<void(const PtrConnection &, Buffer *)>;
    using CloseCallback = std::function<void(const PtrConnection &)>;
    using AnyEventCallback = std::function<void(const PtrConnection &)>;

    ConnectedCallback _connected_callback;
    MessageCallback _message_callback;
    CloseCallback _closed_callback;
    AnyEventCallback _event_callback;
    CloseCallback _server_closed_callback;

private:
    void HandleRead()
    {
        char buf[65536];
        ssize_t ret = _socket.NonBlockRecv(buf, sizeof(buf) - 1);
        if (ret < 0)
        {
            return ShutdownInLoop();
        }
        _in_buffer.WriteAndMove(buf, ret);
        if (_in_buffer.ReadAbleSize() > 0)
        {
            return _message_callback(shared_from_this(), &_in_buffer);
        }
    }
    void HandleWrite()
    {
        ssize_t ret = _socket.NonBlockSend(_out_buffer.ReadPosition(), _out_buffer.ReadAbleSize());
        if (ret < 0)
        {
            if (_in_buffer.ReadAbleSize() > 0)
            {
                _message_callback(shared_from_this(), &_in_buffer);
            }
            return Release();
        }
        _out_buffer.MoveReadOffset(ret);
        if (_out_buffer.ReadAbleSize() == 0)
        {
            _channel.DisableWrite();
            if (_status == ConnStatus::DISCONNECTING)
            {
                return Release();
            }
        }
        return;
    }
    void HandleClose()
    {
        if (_in_buffer.ReadAbleSize() > 0)
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }

        return Release();
    }
    void HandleError()
    {
        HandleClose();
    }
    void HandleEvent()
    {
        if (_enable_inactive_release == true)
        {
            _loop->TimerRefresh(_conn_id);
        }
        if (_event_callback)
        {
            _event_callback(shared_from_this());
        }
    }
    void EstablishedInLoop()
    {
        assert(_status == ConnStatus::CONNECTING);
        _status = ConnStatus::CONNECTED;
        _channel.EnableRead();
        if (_connected_callback)
            _connected_callback(shared_from_this());
    }
    void ReleaseInLoop()
    {
        _status = ConnStatus::DISCONNECTED;
        _channel.Remove();
        _socket.Close();
        if (_loop->HasTimer(_conn_id))
            CancelInactiveReleaseInLoop();
        if (_closed_callback)
            _closed_callback(shared_from_this());
        if (_server_closed_callback)
            _server_closed_callback(shared_from_this());
    }

    void SendInLoop(Buffer &buf)
    {
        if (_status == ConnStatus::DISCONNECTED)
        {
            return;
        }
        _out_buffer.WriteBufferAndMove(buf);
        if (_channel.WriteAble() == false)
        {
            _channel.EnableWrite();
        }
    }
    void ShutdownInLoop()
    {
        _status = ConnStatus::DISCONNECTING;
        if (_in_buffer.ReadAbleSize() > 0)
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }
        if (_out_buffer.ReadAbleSize() > 0)
        {
            if (_channel.WriteAble() == false)
            {
                _channel.EnableWrite();
            }
        }
        if (_out_buffer.ReadAbleSize() == 0)
        {
            Release();
        }
    }
    void EnableInactiveReleaseInLoop(const uint32_t &sec)
    {
        _enable_inactive_release = true;
        if (_loop->HasTimer(_conn_id))
        {
            return _loop->TimerRefresh(_conn_id);
        }
        _loop->TimerAdd(_conn_id, sec, std::bind(&Connection::Release, this));
    }
    void CancelInactiveReleaseInLoop()
    {
        _enable_inactive_release = false;
        if (_loop->HasTimer(_conn_id))
        {
            _loop->TimerCancel(_conn_id);
        }
    }
    void UpgradeInLoop(const std::any &context, const ConnectedCallback &conn_cb, const MessageCallback &msg_cb, const CloseCallback &close_cb, const AnyEventCallback &event_cb)
    {
        _context = context;
        _connected_callback = conn_cb;
        _message_callback = msg_cb;
        _closed_callback = close_cb;
        _event_callback = event_cb;
    }

public:
    Connection(EventLoop *loop, const uint64_t &conn_id, const int &sockfd)
        : _conn_id(conn_id), _sockfd(sockfd), _loop(loop), _socket(_sockfd), _status(ConnStatus::CONNECTING), _enable_inactive_release(false), _channel(_loop, _sockfd)
    {
        _channel.SetReadCallback(std::bind(&Connection::HandleRead, this));
        _channel.SetWriteCallback(std::bind(&Connection::HandleWrite, this));
        _channel.SetCloseCallback(std::bind(&Connection::HandleClose, this));
        _channel.SetErrorCallback(std::bind(&Connection::HandleError, this));
        _channel.SetEventCallback(std::bind(&Connection::HandleEvent, this));
    }
    ~Connection()
    {
        DBG_LOG("RELEASE CONNECTION:%p", this);
    }
    int Fd()
    {
        return _sockfd;
    }
    uint64_t Id()
    {
        return _conn_id;
    }
    void SetContext(const std::any &context)
    {
        _context = context;
    }
    bool Connected()
    {
        return _status == ConnStatus::CONNECTED;
    }
    void SetConnectedCallback(const ConnectedCallback &cb)
    {
        _connected_callback = cb;
    }

    void SetMessageCallback(const MessageCallback &cb)
    {
        _message_callback = cb;
    }
    void SetClosedCallback(const CloseCallback &cb)
    {
        _closed_callback = cb;
    }
    void SetSrvClosedCallback(const CloseCallback &cb)
    {
        _server_closed_callback = cb;
    }
    void SetAnyEventCallback(const AnyEventCallback &cb)
    {
        _event_callback = cb;
    }
    void Established()
    {
        _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, this));
    }
    void Send(const char *data, const size_t &len)
    {
        Buffer buf;
        buf.WriteAndMove(data, len);
        _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, std::move(buf)));
    }
    void Shutdown()
    {
        _loop->RunInLoop(std::bind(&Connection::ShutdownInLoop, this));
    }
    void Release()
    {
        _loop->QueueInLoop(std::bind(&Connection::ReleaseInLoop, this));
    }
    void EnableInactiveRelease(const uint32_t &sec)
    {
        _loop->RunInLoop(std::bind(&Connection::EnableInactiveReleaseInLoop, this, sec));
    }
    void CancelInactiveRelease()
    {
        _loop->RunInLoop(std::bind(&Connection::CancelInactiveReleaseInLoop, this));
    }
    void Upgrade(const std::any &context, const ConnectedCallback &conn_cb, const MessageCallback &msg_cb, const CloseCallback &close_cb, const AnyEventCallback &event_cb)
    {
        _loop->AssertInLoop();
        _loop->RunInLoop(std::bind(&Connection::UpgradeInLoop, this, context, conn_cb, msg_cb, close_cb, event_cb));
    }
    std::any *GetContext()
    {
        return &_context;
    }
};

#endif