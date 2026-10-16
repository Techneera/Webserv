# AGENTS.md — webserv

Guidance for AI coding agents working in this repository. Assumes no prior knowledge of the project.

## Project Overview

`webserv` is an HTTP/1.1 web server written in **C++98** (ISO/IEC 14882:1998), developed as part of the **42 school curriculum** by rluis-ya, avieira-, jcesar-s. The goal is a single-process, single-threaded, event-driven server using `epoll()` (Linux-only) as the sole I/O multiplexer, supporting:

- Multiple concurrent client connections and multiple listening ports
- Static file serving, directory autoindex, custom error pages
- CGI execution (RFC 3875, e.g. `php-cgi`, `python3`)
- File uploads (`multipart/form-data`)
- NGINX-inspired configuration files (`webserv.conf`) with `server` and `location` blocks

**Current state: early skeleton.** The repository contains a placeholder class (`Template`) and a detailed architecture document, but the actual server is not implemented yet. Treat `documentation/architecture/ARCHITECTURE.md` as the authoritative design spec — implementation decisions should follow it.

## Repository Layout

```
include/            Public headers (e.g. DataModel.hpp, Template.hpp)
src/                Source files (main.cpp, Template.cpp)
documentation/
  architecture/     ARCHITECTURE.md — full high-level design (RFCs, constraints, components)
  references/       useful_links.md — RFCs, Beej's guides, man pages
  TBD/              guidelines.md (currently empty)
README.md           Project intro (mostly placeholder)
```

## Build and Test Commands

**There is currently no Makefile or other build system in the repository** — one must be created. The architecture spec (§15.1) mandates:

- Compiler: `g++` or `clang++` with `-std=c++98` (and typically `-Wall -Wextra -Werror`)
- Build system: a **Makefile** with the standard 42 targets: `all`, `clean`, `fclean`, `re`
- Output executable: `webserv` (run as `./webserv [path/to/webserv.conf]`)
- No external libraries; Linux only (`epoll` required)

**There are no tests yet.** No test framework, test directory, or CI configuration exists. When adding tests, you are establishing the convention — keep it simple and consistent with the 42 curriculum (typically ad-hoc test programs or manual verification with `curl` and browser compatibility checks).

## Architecture and Module Divisions (per the design spec)

The planned components (see `documentation/architecture/ARCHITECTURE.md` §4) are:

- **Server Engine** — owns the single `epoll` fd and the main event loop (`epoll_wait`)
- **Config Loader** — parses/validates the NGINX-style config into `ServerConfig`/`LocationConfig`
- **Connection Manager** — listeners and per-client state machines (recv/send buffers, timestamps, timeouts)
- **HTTP Pipeline** — Request Parser → Router (longest-prefix location match) → Handler Factory (static/CGI/upload/redirect) → Response Builder
- **CGI Manager** — `fork()`/`execve()` child processes, RFC 3875 environment, pipe I/O via epoll, timeout killing
- **Logger** — access (Combined Log Format), error, and debug logs to configurable paths
- **Signal Manager** — `SIGINT`/`SIGTERM` graceful shutdown, `SIGPIPE` ignored, `SIGCHLD` reaping via `waitpid()`

## Mandatory Constraints (from the 42 subject — do not violate)

- **C++98 only**: no `auto`, range-for, lambdas, smart pointers, `std::thread`, or any C++11+ feature
- **Single `epoll()` instance** for all socket and CGI-pipe I/O; never `poll()`/`select()`-style busy waiting
- **All sockets and pipes non-blocking** via `fcntl(..., O_NONBLOCK)`
- **Never `recv`/`send` without epoll readiness notification**; use state machines for partial reads/writes
- **No `errno` checks after read/write** — rely on return values and epoll events only
- **`fork()` only for CGI**; all request handling happens in the main process/thread
- Must stay alive under stress: a failed request must never crash or hang the server
- Must be compatible with standard web browsers (correct HTTP/1.1 semantics per RFC 9110/9112)

## Code Style Guidelines

Match the existing files in `src/` and `include/`:

- Headers use `#ifndef`/`#define` include guards named after the file (`TEMPLATE_HPP`)
- Classes follow the **Orthodox Canonical Form** (default ctor, dtor, copy ctor, copy assignment) — see `Template.hpp`/`Template.cpp`
- Class member variables are prefixed with `_` (e.g. `_buffer`)
- Shared constants live in `include/DataModel.hpp` as `#define`s (e.g. `MAX_EPOLL_EVENTS 2048`, `MAX_BUFFER_SIZE 1024`); plain structs are `typedef`'d with a `_t` suffix
- 4-space indentation, braces on their own line
- Language: **English** for all code, comments, and documentation

## Key References

- `documentation/architecture/ARCHITECTURE.md` — the full design document (1000 lines): read the relevant section before implementing any component
- RFC 9112 (HTTP/1.1 message syntax), RFC 9110 (HTTP semantics), RFC 3875 (CGI), RFC 3986 (URIs), RFC 2046 (multipart)
- Beej's Guide to Network Programming (linked in README.md and `documentation/references/useful_links.md`)

## Security Considerations

- Treat all client input as untrusted: strict validation at every parsing layer (request line, headers, URI decoding, chunked bodies, multipart)
- Prevent path traversal when resolving document roots against the filesystem
- Enforce `client_max_body_size` limits and connection/CGI timeouts
- Ignore `SIGPIPE` / use `MSG_NOSIGNAL` so broken connections cannot terminate the process
- Check file accessibility (`access`, `stat`) before serving; never serve files outside the configured root
