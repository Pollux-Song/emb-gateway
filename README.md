# emb-gateway · 端云一体数据采集系统（MVP）

设备端采集 → 断网缓存 → 恢复补传 → 云端落库查询。
C + Linux 实现，环形缓冲 + CRC8 帧编码 + MQTT。

## 架构

```
┌─────────────┐     ┌──────────┐     ┌─────────────┐
│  端侧设备    │────▶│  MQTT    │────▶│  云侧订阅    │
│  (gateway)  │     │  broker  │     │  (Python)   │
└──────┬──────┘     └──────────┘     └──────┬──────┘
       │ 断网时                                 │
       ▼                                       ▼
┌─────────────┐                         ┌──────────┐
│  环形缓冲区   │                         │  SQLite   │
│  64条×11字节 │                         │  落库     │
└─────────────┘                         └──────────┘
```

## 核心设计决策

### 1. 为什么用环形缓冲，不用普通数组？
- **O(1) 读写**：head/tail 指针移动，不用 memmove
- **无动态内存分配**：嵌入式环境堆碎片是大敌，静态数组最稳
- **天然覆盖旧数据**：满了自动丢最旧，不需要额外逻辑

### 2. 为什么 payload 手工序列化，不直接 memcpy 结构体？
- **内存对齐**：结构体里有 padding，直接 memcpy 会带入垃圾字节
- **大小端**：不同 CPU 字节序不同，手工序列化保证跨平台一致

### 3. 为什么溢出策略是"丢最旧"，不是"丢最新"？
- 传感器数据是时序流，旧数据价值低，新数据有价值
- 丢最旧 = 保留最近的状态，符合现场排障需求
- 如果要做落盘，应该用环形文件，不是 SQLite（写入寿命）

### 4. 为什么发布成功才出队？
- "发布成功"才意味着 broker 确认收到
- 没出队 = 数据还在本地，断网恢复后自动补传
- 服务端用 (device, seq) 幂等去重，保证不重复

## 关键数字

| 指标 | 值 |
|---|---|
| 缓冲区容量 | 64 条 × 11 字节 ≈ 704 字节 |
| 采样间隔 | 200ms（每秒 5 条） |
| 断网 30 秒缓冲 | 约 150 条（自动丢最旧） |
| 丢包率 | 0（发布成功才出队） |
| 帧格式 | magic(2) + len(1) + type(1) + seq(2) + payload + CRC8 |

## 技术栈

- **端侧**：C99、Linux 系统编程、MQTT（libmosquitto）
- **云侧**：Python、paho-mqtt、SQLite、FastAPI
- **测试**：自研断言框架，`make test` 22 项全绿

## 快速开始

```bash
# 跑测试
make test

# 模拟模式（不需要 MQTT broker）
make sim
./gateway_sim 20

# 真实 MQTT
sudo apt install mosquitto mosquitto-clients libmosquitto-dev
sudo systemctl start mosquitto
make && ./gateway 0

# 另开终端看消息
mosquitto_sub -h localhost -t 'dev/+/data' -v
```

## 项目结构

```
src/
├── ringbuf.c/h    ← 环形缓冲区（我写的）
├── protocol.c/h   ← 帧编码 + CRC8（我写的）
├── sensor.c/h     ← 模拟传感器源
└── gateway.c      ← 主循环，MQTT 发布
tests/
├── test_ringbuf.c ← 环形缓冲测试
└── test_protocol.c ← 协议测试
cloud/
├── subscriber.py  ← 订阅 + 落库
└── api.py         ← HTTP 查询接口
```

## 面试追问预案

- **怎么保证不重复？** 帧序号 + 服务端 (device, seq) 幂等去重
- **CRC8 能检出什么？** 单比特、部分双比特；不能纠错
- **断网怎么检测？** MQTT 连接状态 + 发布失败计数
- **为什么 MVP 用 SQLite？** 先跑通链路；数据量起来换 TDengine
