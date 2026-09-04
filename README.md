# ztrace

Lightweight cross-platform C++ telemetry library based on shared memory. Designed for low-latency and high-throughput applications where telemetry should be cheap and independent from the monitored process.

### Features

- **Zero-copy IPC** - Direct shared-memory access, no sockets, no serialization overhead
- **Connect by name** - Variables and streams connect via name across processes
- **Survive restarts** - Stop, rebuild, and restart either side
- **Lock-free streams** - Single-producer/single-consumer ring buffers with atomic operations
- **Header-only** - Drop in and go

### Requirements

- **C++20**
- **CMake 3.20+**
- **Boost 1.83+**

### Usage

ztrace has no setup ceremony and no required startup order. Create a `Variable` in one process, create a `Viewer` in another - they instantly connect. Restart the producer and the consumer keeps reading without a hiccup. Same for `Stream`: push on one side, `drain` on the other.

```cpp
// producer.cpp
ztrace::Variable<double> fps("fps", 60.0);
fps.update(144.0);

// consumer.cpp
ztrace::Viewer<double> fps("fps");
double current = fps.read(); // 144.0
```

For more examples see [`examples`](examples) directory.

### License

MIT