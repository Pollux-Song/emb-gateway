# 端云一体数据采集系统（MVP）

> 目标：一个能在面试里讲 10 分钟、能演示 30 秒、每个设计都能被追问三层的项目。
> 不是功能最多的项目，是取舍讲得最清楚的项目。

---

## 一、功能冻结线（先画死，别扩张）

### 必须有（做到这四条就算"完整"）

1. 端侧每 200ms 产生一条数据（带时间戳、温度、电压、状态）
2. 数据编码成带 CRC 的帧，压入环形缓冲区
3. broker 在线时发布，离线时继续缓存；恢复后按顺序补发
4. 云侧订阅、落库、提供按设备和时间查询的 HTTP 接口

### 有余力再做（加分，不是必须）

5. 串口作为真实数据源（现在是模拟源）
6. 一个轮询展示的网页看板
7. 交叉编译到 i.MX6ULL 实机运行

### 明确不做

用户系统、鉴权、前端框架、Docker 编排、时序数据库、OTA、多租户、K8s。

**这条线以上每加一个功能，你的进度就晚一天。面试官不会因为你多了个登录页而加分。**

---

## 二、目录结构

```
emb-gateway/
├── Makefile
├── README.md
├── src/
│   ├── ringbuf.h / ringbuf.c      ← 你要写（环形缓冲区）
│   ├── protocol.h / protocol.c    ← 你要写（组包、CRC8、解包）
│   ├── sensor.h / sensor.c        ← 已给（模拟传感器 + 显式序列化）
│   └── gateway.c                  ← 已给（主循环，MQTT 发布）
├── tests/
│   ├── test_ringbuf.c             ← 已给
│   └── test_protocol.c            ← 已给
└── cloud/
    ├── subscriber.py              ← 已给（订阅 + 解析 + 落库）
    ├── api.py                     ← 已给（查询接口）
    └── requirements.txt
```

**你只需要写两个文件：`ringbuf.c` 和 `protocol.c`。** 其余都能直接跑。这是刻意的——把时间花在面试会考的地方。

---

## 三、快速开始

### 1. 先在 PC 上跑通（不需要板子、不需要 MQTT）

```
make sim
./gateway_sim 20
```

第一次会看到没有输出，因为环形缓冲和协议还是空的。先 `make test`，让测试从红变绿。

### 2. 跑测试

```
make test
```

### 3. 接真实 MQTT

```
sudo apt install mosquitto mosquitto-clients libmosquitto-dev
sudo systemctl start mosquitto
make
./gateway 0
```

另开一个终端看消息：

```
mosquitto_sub -h localhost -t 'dev/+/data' -v
```

### 4. 启动云侧

```
cd cloud
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python subscriber.py          # 终端 1
uvicorn api:app --reload      # 终端 2
curl 'http://127.0.0.1:8000/data?device=dev-001'
```

### 5. 演示断网续传（面试时最值钱的一段）

1. 启动 `gateway`，让它正常上报
2. `sudo systemctl stop mosquitto`
3. 等 30 秒，端侧会打印缓冲区在增长
4. `sudo systemctl start mosquitto`
5. 观察：缓冲区被清空，云端数据完整，丢包率为 0

**把这一段录成 30 秒视频。**

---

## 四、四天构建顺序

| 天 | 任务 | 验收 |
|---|---|---|
| D1 | 写 `ringbuf.c`，让 `make test` 的 ringbuf 部分全绿 | 溢出时丢最旧、计数正确 |
| D2 | 写 `protocol.c`：CRC8、组包、解包 | 篡改一个字节后解包失败 |
| D3 | 装 mosquitto，跑通端到云，云侧落库 | `mosquitto_sub` 能看到消息 |
| D4 | 断网续传实验 + README + 架构图 + 30 秒视频 | 丢包率 0 |

---

## 五、必须拿到的五个数字（简历和面试都靠它们）

1. **正常链路端到端延迟**：从采样到写入数据库多少 ms
2. **断网 30 秒后的补传耗时**
3. **缓冲区容量**：多少条、多大内存
4. **溢出策略**：丢最旧 / 落盘，为什么这么选
5. **丢包率**：理论 0，实际测出来多少

没有数字的项目经历，在面试官眼里等于"跟着教程做过"。

---

## 六、3 分钟讲法（直接背结构，不要背稿）

1. **做什么**：设备端采集数据，断网不丢，恢复自动补传，云端可查询。
2. **怎么实现**：C + Linux，串口/模拟源 → 帧编码（CRC8）→ 环形缓冲 → MQTT → Python 订阅 → SQLite → HTTP 查询。
3. **难点**：断网续传。普通数组满就覆盖会丢数据，改成环形缓冲 + "发布成功才出队"，保证不丢；用帧序号和 CRC 保证不重复、不脏数据。
4. **结果**：断网 30 秒恢复后补传完整，丢包率 0，端到端延迟 X ms。

---

## 七、面试高频追问（提前想好）

- 为什么用环形缓冲而不是普通数组？→ O(1)、无内存分配、天然覆盖旧数据
- 缓冲区满了怎么办？→ 丢最旧并计数，或落盘；说明取舍
- 怎么保证不重复？→ 帧序号 + 服务端按 (device, seq) 去重
- CRC8 能检出什么？→ 单比特、部分双比特；不能纠错
- 为什么 payload 要手工序列化，不直接 `memcpy` 结构体？→ 内存对齐会带入 padding，还受大小端影响
- 为什么 MVP 用 SQLite？→ 先跑通链路；数据量起来换 TDengine，理由讲清楚就行
- 断网怎么检测？→ 连接状态 + 发布失败计数

---

## 八、常见坑

- `rb_push` 满了要**先移动 tail 再写**，顺序错了会读到脏数据
- CRC 的计算范围要和 Python 侧完全一致，否则永远对不上
- `mosquitto_publish` 返回成功不等于送达，真实场景要 QoS 1 + 确认
- 时间戳用 `uint32_t` 存秒级 uptime，别用 `time_t` 直接塞进 4 字节
