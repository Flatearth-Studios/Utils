
# Game Dev Utility Headers

This repository contains two foundational headers I commonly use when starting a new C++ game or engine project. They're designed to speed up development with cleaner syntax, modern C++ idioms, and minimal friction.

```bash
src/
├── Defines.hpp   // Core type aliases and shorthand for containers
└── Logger.hpp    // Async logger with modern formatting and file support
```

## Defines.hpp

A small utility header to standardize common types and reduce boilerplate.

### Features
- Type aliases for clarity and brevity:

```cpp
using string = std::string;
using uint32 = uint32_t;
using float32 = float;
```

- `fstring<T...>` for safe `std::format_string` use.

- Shorthand for containers and smart pointers:

```cpp
uptr<T>       → std::unique_ptr<T>
umap<K, V>    → std::unordered_map<K, V>
uset<T>       → std::unordered_set<T>
```

## Logger.hpp

A header-only, asynchronous logging system designed for real-time applications like games.

Highlights

🔹 Built with C++23 features: `std::format`, `std::source_locationi`, `std::print`

🔹 Asynchronous logging via thread-safe queue and worker thread

🔹 Color-coded terminal output by log level (Info, Warn, Error, etc.)

🔹 Optional file logging via `ENABLE_FILE_LOGGING(true)`

🔹 Source-aware: logs show file, line, and function name

🔹 Clean macro interface: `LOG_INFO`, `LOG_ERROR`, `FLOG_WARN`, etc.

Example Usage:

```cpp
LOG_INFO("Player connected: {}", playerId);
LOG_WARN("FPS dropped to {}", currentFps);
FLOG_ERROR("Unable to save file: {}", path);
```

## Requirements

- C++23 or newer
- Terminal with ANSI color support (for pretty logs)

## Integration

Just drop both headers into your project and include them:

```cpp
#include "Defines.hpp"
#include "Logger.hpp"
```

No linking, no setup — perfect for prototyping or building a lightweight game engine from scratch.

## About

These headers must be seen as a toolkit used across multiple game dev projects to standardize types and handle clean, performant logging without extra dependencies.
