# C++多网络协议高并发服务器

## 项目简介

这是一个基于C++17开发的高性能、多协议网络服务器框架，支持TCP协议，可用于构建Echo服务器、HTTP服务器等多种网络应用。

- **高性能**：采用事件驱动、非阻塞I/O、线程池等技术，实现高并发处理能力
- **模块化设计**：支持单头文件形式和多头文件形式，满足不同场景的使用需求
- **多协议支持**：内置Echo和HTTP协议实现，可扩展性强
- **稳定性**：长时间压测无抖动、无内存泄漏、无连接异常

## 技术栈

- **开发语言**：C++17
- **设计框架**：采用One Thread One Loop式主从Reactor模型
- **网络模型**：事件驱动、Reactor模式
- **I/O多路复用**：epoll
- **线程模型**：主线程+工作线程池
- **内存管理**：智能指针
- **构建工具**：Makefile

## 性能测试

### 测试环境

- 腾讯云 2 核 4G 轻量应用服务器
- 本机回环压测

### 测试工具

```bash
wrk -t2 -c500 -d60s http://localhost:8080/
```

### 测试结果

- **稳定 QPS**：1.12w+
- **平均延迟**：41ms
- **瓶颈分析**：服务瓶颈为 CPU 计算能力，已达当前硬件性能上限
- **稳定性**：长时间压测无抖动、无内存泄漏、无连接异常

## 目录结构

```
├── include/              # 头文件目录
│   ├── server.hpp        # 单头文件形式的服务器实现
│   ├── server/           # 多头文件形式的服务器实现
│   │   ├── server.hpp    # 多头文件形式的主头文件
│   │   ├── common/       # 公共工具和宏定义
│   │   ├── buffer.hpp    # 缓冲区实现
│   │   ├── socket.hpp    # 套接字封装
│   │   ├── channel.hpp   # 事件通道
│   │   ├── poller.hpp    # 事件轮询器
│   │   ├── timer.hpp     # 定时器
│   │   ├── event_loop.hpp # 事件循环
│   │   ├── thread.hpp    # 线程池
│   │   ├── connection.hpp # 连接管理
│   │   ├── acceptor.hpp  # 连接接收器
│   │   ├── tcp_server.hpp # TCP服务器
│   │   └── network.hpp   # 网络初始化
│   └── protocol/         # 协议实现
│       ├── echo.hpp      # Echo协议
│       └── http.hpp      # HTTP协议
├── examples/             # 示例代码
│   ├── single_header/    # 单头文件形式示例
│   │   ├── echo/         # Echo服务器示例
│   │   └── http/         # HTTP服务器示例
│   └── multi_header/     # 多头文件形式示例
│       ├── echo/         # Echo服务器示例
│       └── http/         # HTTP服务器示例
├── README.md             # 项目说明
└── .gitignore            # Git忽略文件
```

## 快速开始

### 编译示例

#### 单头文件形式

```bash
# Echo服务器
cd examples/single_header/echo
make

# HTTP服务器
cd examples/single_header/http
make
```

#### 多头文件形式

```bash
# Echo服务器
cd examples/multi_header/echo
make

# HTTP服务器
cd examples/multi_header/http
make
```

### 运行示例

#### Echo服务器

```bash
./main
# 然后使用telnet或nc连接到8084端口
```

#### HTTP服务器

```bash
./main
# 然后在浏览器中访问 http://localhost:8080/
```

## 核心特性

### 1. 事件驱动架构

- 基于Reactor模式，使用epoll进行事件监听
- 非阻塞I/O，提高并发处理能力
- 事件循环机制，高效处理网络事件

### 2. 线程池设计

- 主线程负责接受连接
- 工作线程池处理I/O事件
- 负载均衡，提高系统吞吐量

### 3. 内存管理

- 智能指针自动管理内存
- 缓冲区复用，减少内存分配
- 无内存泄漏，长时间运行稳定

### 4. 多协议支持

- 内置Echo协议实现
- 内置HTTP协议实现，支持静态文件服务和路由
- 可扩展其他协议

### 5. 两种使用形式

- **单头文件形式**：简单易用，只需包含一个头文件
- **多头文件形式**：结构清晰，便于维护和扩展

## 使用示例

### Echo服务器

```cpp
#include "server.hpp" // 单头文件形式
// 或 #include "server/server.hpp" // 多头文件形式

class EchoServer {
private:
    void OnConn(const PtrConnection &conn) {
        DBG_LOG("NEW CONNECTION:%p", conn.get());
    }
    void OnMsg(const PtrConnection &conn, Buffer *buf) {
        conn->Send(buf->ReadPosition(), buf->ReadAbleSize());
        buf->MoveReadOffset(buf->ReadAbleSize());
    }
    void OnClose(const PtrConnection &conn) {
        DBG_LOG("CLOSE CONNECTION:%p", conn.get());
    }

public:
    EchoServer(const int port) : _server(port) {
        _server.SetThreadCount(2);
        _server.EnableInactiveRelease(3);
        _server.SetConnectedCallback(std::bind(&EchoServer::OnConn, this, std::placeholders::_1));
        _server.SetMessageCallback(std::bind(&EchoServer::OnMsg, this, std::placeholders::_1, std::placeholders::_2));
        _server.SetClosedCallback(std::bind(&EchoServer::OnClose, this, std::placeholders::_1));
    }
    void Start() {
        _server.Start();
    }

private:
    TcpServer _server;
};

int main() {
    EchoServer server(8084);
    server.Start();
    return 0;
}
```

### HTTP服务器

```cpp
#include "server.hpp" // 单头文件形式
// 或 #include "server/server.hpp" // 多头文件形式
#include <regex>
#include <fstream>

// HTTP服务器实现...

int main() {
    HttpServer server(8080);
    server.SetThreadCount(2);
    server.SetBaseDir("./wwwroot");
    server.Get("/", [](const HttpRequest &req, HttpResponse *rsp) {
        rsp->SetContent("<h1>Hello, HTTP Server!</h1>", "text/html");
    });
    server.Get("/hello", [](const HttpRequest &req, HttpResponse *rsp) {
        std::string name = req.GetParam("name");
        if (name.empty()) {
            name = "World";
        }
        rsp->SetContent("<h1>Hello, " + name + "!</h1>", "text/html");
    });
    server.Listen();
    return 0;
}
```

## 项目亮点

1. **高性能**：采用事件驱动和非阻塞I/O，QPS达到1.12w+，已达硬件性能上限
2. **稳定性**：长时间压测无抖动、无内存泄漏、无连接异常
3. **灵活性**：支持单头文件和多头文件两种使用形式，满足不同场景需求
4. **可扩展性**：模块化设计，易于添加新协议和功能
5. **安全性**：内置信号处理，防止SIGPIPE等信号导致的崩溃

## 应用场景

- **Web服务器**：基于HTTP协议实现静态文件服务和API接口
- **游戏服务器**：处理大量并发连接
- **实时通信**：如聊天服务器、推送服务
- **代理服务器**：转发和处理网络请求

## 贡献指南

1. Fork 本仓库
2. 新建 Feat\_xxx 分支
3. 提交代码
4. 新建 Pull Request

## 许可证

本项目采用 MIT 许可证，详情请查看 LICENSE 文件。
