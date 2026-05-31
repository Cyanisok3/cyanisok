#pragma once
#include "SslContext.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/noncopyable.h>
#include <openssl/ssl.h>
#include <memory>

namespace ssl
{

using MessageCallback = std::function<void(const std::shared_ptr<muduo::net::TcpConnection>&,
                                         muduo::net::Buffer*,
                                         muduo::Timestamp)>;

class SslConnection : muduo::noncopyable
{
public:
    using TcpConnectionPtr = std::shared_ptr<muduo::net::TcpConnection>;
    using BufferPtr = muduo::net::Buffer*;

    SslConnection(const TcpConnectionPtr& conn, SslContext* ctx);
    ~SslConnection();

    void startHandshake();
    void send(const void* data, size_t len);
    void onRead(const TcpConnectionPtr& conn, BufferPtr buf, muduo::Timestamp time);
    bool isHandshakeCompleted() const { return state_ == SSLState::ESTABLISHED; }
    muduo::net::Buffer* getDecryptedBuffer() { return &decryptedBuffer_; }
    static int bioWrite(BIO* bio, const char* data, int len);
    static int bioRead(BIO* bio, char* data, int len);
    static long bioCtrl(BIO* bio, int cmd, long num, void* ptr);
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
private:
    void handleHandshake();
    void onEncrypted(const char* data, size_t len);
    void onDecrypted(const char* data, size_t len);
    SSLError getLastError(int ret);
    void handleError(SSLError error);

private:
    SSL*                ssl_;
    SslContext*         ctx_;
    TcpConnectionPtr    conn_;
    SSLState            state_;
    BIO*                readBio_;
    BIO*                writeBio_;
    muduo::net::Buffer  readBuffer_;
    muduo::net::Buffer  writeBuffer_;
    muduo::net::Buffer  decryptedBuffer_;
    MessageCallback     messageCallback_;
};

} // namespace ssl
