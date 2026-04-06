# 🚀 HyperMuduo Project Guide (GEMINI.MD)

HyperMuduo 是一个基于 **C++20** 标准实现的高性能 Linux 多线程网络库，深受陈硕老师《Linux 多线程服务端编程》及 Muduo 库的启发。本项目旨在通过现代 C++ 特性重新实现核心网络组件，并探索异步 I/O、Protobuf 编解码及类型擦除分发模型。

---

## 🎯 项目核心目标 (Core Goals)
- **高性能**: 采用 Non-blocking I/O + IO Multiplexing (Epoll) 模型，追求零拷贝（Zero-copy）思维。
- **现代化**: 全面拥抱 **C++20**（std::span, std::jthread, std::stop_token, 概念约束等）。
- **解耦设计**:
    - **Stateless Codec**: 编解码器为纯静态工具类，不持有连接状态。
    - **Logic Handler**: 独立包装类，负责粘合网络层与编解码逻辑。
    - **Type-Erasure Dispatcher**: 自动实现具体消息类型的分发，消除手动 `down-cast`。

---

## 🏗 协议布局 (Protocol Layout)

封包格式严格遵循以下线性空间分布（总长度 $L$ 指后续字节总数）：

| 偏移量 | 长度 | 字节序 | 内容 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| `0` | `4B` | Big-Endian | **Total Length ($L$)** | HEADER_LEN (不含自身) |
| `4` | `4B` | Big-Endian | **Name Length** | NAME_LEN_SIZE (校验起点) |
| `8` | `Var` | - | **Type Name** | 消息类型全称 (含 Package) |
| `...` | `Var` | - | **Protobuf Data** | 二进制序列化负载 |
| `L+4-4`| `4B` | Big-Endian | **Checksum** | 对 NameLen 之后区域的 Adler32 |

---

## 🛠 当前进度与架构 (Current Status)

### 1. 基础组件 (Base)
- [x] **`Buffer`**: 基于 `vector<char>` 的自动扩容缓冲区，支持 `peek/append/retrieve`。
- [x] **`Reflection`**: 匿名命名空间辅助函数 `createMessage`，实现根据类型名动态创建对象。

### 2. 网络核心 (Net)
- [x] **`Codec::Encode`**: **已完成**。优化版实现：单次内存申请 + `AppendToString` 原地序列化 + 完整字节序转换。
- [x] **`Codec::Decode`**: **待开发**。需处理：五级过滤（长度/安全/反射）、半包检测、读指针回退。
- [ ] **`Dispatcher`**: **待开发**。核心逻辑：利用模板派生类还原具体 `T` 类型。

### 3. 事件驱动 (Reactor - 规划中)
- [ ] **`EventLoop / Channel / Poller`**: 反应堆核心组件。

---

## 📝 Gemini CLI 协作准则 (AI Interaction Rules)

**当 Gemini CLI 在本项目中辅助工作时，必须严格执行以下逻辑：**

1.  **逻辑先行，严禁直接给代码**:
    * 除非明确授权（如“请给出参考实现”），否则**禁止直接输出完整 C++ 代码块**。
    * 优先提供 **逻辑路径 (Logic Paths)**、**架构图解** 和 **伪代码 (Pseudocode)**。
2.  **边界与异常扫描**:
    * 在讨论 `Decode` 时，必须主动提出“半包”、“粘包”及“恶意超大包”的防御策略。
    * 分析指针偏移时，需明确指出 `Buffer` 扩容导致的指针失效风险。
3.  **现代化审查 (C++20)**:
    * 优先推荐 C++20 标准库替代旧版方案（如使用 `std::string_view` 替代 `const string&` 传参）。

---

## 📅 近期路线图 (Roadmap)
- [ ] 实现 `Codec::Decode`：精准控制 `Buffer` 读指针，处理 `DecodeState` 状态流转。
- [ ] 实现 `ProtobufDispatcher`：通过模板擦除类型，提供优雅的业务回调注册接口。
- [ ] 接入 `TcpConnection`：开始处理真实的 Socket 数据流读写。

---
