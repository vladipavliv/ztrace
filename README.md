# ZeroTrace

Lightweight cross-platform C++ telemetry library based on shared memory. Designed for low-latency and high-throughput applications where telemetry should be cheap and independent from the monitored process.

### Features

- **C++20** - Modern C++ API
- **Cross-platform** - Platform-independent API
- **Shared-memory IPC** - Direct inter-process communication without sockets
- **Lock-free variables** - Atomic values directly in shared memory
- **SPSC ring buffers** - Lock-free single-producer/single-consumer event streams
- **Persistent telemetry** - Monitoring processes survive application restarts
- **No serialization** - Trivially copyable data stored directly in shared memory
- **Header-only API** - Simple integration

### Requirements

- **C++20**
- **CMake 3.20+**
- **Boost 1.83+** - Used for cross-platform shared memory

### Usage

See the [`examples`](examples) directory for usage examples.

### License

MIT