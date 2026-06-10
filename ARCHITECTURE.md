# High Level Architecture Document (HLA)
## webserv — HTTP/1.1 Server in C++98

---

**Document Control**

| Field | Value |
|-------|-------|
| **Project** | webserv |
| **Language** | C++98 (ISO/IEC 14882:1998) |
| **I/O Multiplexer** | `epoll()` (Linux) |
| **HTTP Version** | HTTP/1.1 (RFC 9112) |
| **Date** | 2026-06-10 |
| **Status** | Draft |

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [System Overview](#2-system-overview)
3. [Architectural Principles & Constraints](#3-architectural-principles--constraints)
4. [Component Architecture](#4-component-architecture)
5. [Data Flow Architecture](#5-data-flow-architecture)
6. [Connection & Session Management](#6-connection--session-management)
7. [HTTP Processing Pipeline](#7-http-processing-pipeline)
8. [Static Content Serving](#8-static-content-serving)
9. [CGI Execution Architecture](#9-cgi-execution-architecture)
10. [File Upload Architecture](#10-file-upload-architecture)
11. [Configuration Architecture](#11-configuration-architecture)
12. [Error Handling & Resilience](#12-error-handling--resilience)
13. [Logging Architecture](#13-logging-architecture)
14. [Signal Handling](#14-signal-handling)
15. [Deployment & Execution](#15-deployment--execution)
16. [Appendices](#16-appendices)

---

## 1. Executive Summary

`webserv` is a single-process, single-threaded, event-driven HTTP/1.1 server implemented in C++98. It uses `epoll()` as its sole I/O event demultiplexer to monitor all network sockets and CGI pipes for readiness. The server supports multiple concurrent connections, static file serving, CGI execution, file uploads, and configurable virtual server blocks. All I/O on sockets and pipes is strictly non-blocking; no read or write operation occurs without prior readiness notification from `epoll()`.

The architecture prioritizes resilience: the server must remain operational under stress, handle partial data gracefully via state machines, enforce timeouts on all operations, and recover cleanly from errors without resource leaks.

---

## 2. System Overview

### 2.1 Execution Model

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              MAIN PROCESS                                    │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────┐   │
│  │  Signal Handler │    │  Config Parser  │    │      Logger             │   │
│  │  (SIGINT,       │    │  (NGINX-inspired│    │  (Access, Error, Debug) │   │
│  │   SIGTERM,      │    │   syntax)       │    │                         │   │
│  │   SIGPIPE)      │    │                 │    │                         │   │
│  └────────┬────────┘    └────────┬────────┘    └────────────┬────────────┘   │
│           │                      │                          │                │
│           ▼                      ▼                          ▼                │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                        EVENT LOOP (epoll_wait)                         │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  │ │
│  │  │  Listener   │  │   Client    │  │   Client    │  │  CGI Pipe      │  │ │
│  │  │  Sockets    │  │  Sockets    │  │  Sockets    │  │  (read/write)  │  │ │
│  │  │  (accept)   │  │  (recv)     │  │  (send)     │  │                │  │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └────────────────┘  │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│           │                      │                          │                │
│           ▼                      ▼                          ▼                │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                     REQUEST/RESPONSE PROCESSOR                          │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  │ │
│  │  │   Parser    │  │   Router    │  │   Handler   │  │   Responder    │  │ │
│  │  │  (HTTP/1.1) │  │  (Location  │  │  (Static/   │  │  (Buffer mgmt) │  │ │
│  │  │             │  │   match)    │  │   CGI/Upload)│  │                │ │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └────────────────┘  │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                         │
│                                    ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                        CGI SUBPROCESSES (fork/execve)                   │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                      │ │
│  │  │  php-cgi    │  │  python3    │  │  other      │                      │ │
│  │  │  (stdin)    │  │  (stdin)    │  │  interpreters                      │ │
│  │  │  (stdout)   │  │  (stdout)   │  │  via shebang)                      │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                      │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Key Characteristics

| Characteristic | Description |
|----------------|-------------|
| **Concurrency Model** | Single-threaded, event-driven (Reactor pattern) |
| **I/O Model** | Non-blocking on all sockets and pipes; `epoll()` for readiness notification |
| **Process Model** | Single main process; `fork()` used exclusively for CGI execution |
| **Protocol** | HTTP/1.1 (RFC 9112) with persistent connection support |
| **Platform** | Linux (epoll-dependent) |
| **Memory Model** | Stack-preferred allocation; static buffers for CGI output |

---

## 3. Architectural Principles & Constraints

### 3.1 Mandatory Constraints (from Subject)

| ID | Constraint | Architectural Impact |
|----|-----------|----------------------|
| C1 | C++98 only | No `std::thread`, `auto`, range-for, smart pointers, lambdas |
| C2 | Single `epoll()` for all I/O | All sockets and pipes registered in one `epoll` instance |
| C3 | Non-blocking always | Every socket/pipe set `O_NONBLOCK` via `fcntl()` |
| C4 | No read/write without epoll readiness | State machines handle partial data; never call recv/send on unready fd |
| C5 | No `errno` checking after read/write | Use return values and epoll events exclusively |
| C6 | `fork()` only for CGI | All request handling in main process |
| C7 | No execve of another web server | Self-contained executable |
| C8 | Compatible with standard browsers | Proper HTTP/1.1 semantics, correct headers |

### 3.2 Design Principles

| Principle | Rationale |
|-----------|-----------|
| **Fail Fast, Fail Safe** | Invalid requests are rejected immediately; errors never crash the server |
| **Resource RAII** | Every allocated resource (fd, memory, process) has deterministic cleanup |
| **Zero-Copy Where Possible** | Response headers constructed in send buffers; file content sent via `sendfile()` if available |
| **Defensive Parsing** | All client input treated as untrusted; strict validation at every layer |
| **Observability** | Comprehensive logging for debugging and audit trails |

---

## 4. Component Architecture

### 4.1 Component Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           webserv COMPONENTS                                │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │   Server    │  │   Config    │  │   Logger    │  │   SignalManager     │ │
│  │   Engine    │  │   Loader    │  │             │  │                     │ │
│  │             │  │             │  │             │  │                     │ │
│  │ - EventLoop │  │ - Parse file│  │ - AccessLog │  │ - SIGINT handler    │ │
│  │ - epoll mgmt│  │ - Validate  │  │ - ErrorLog  │  │ - SIGTERM handler   │ │
│  │ - Lifecycle │  │ - Defaults  │  │ - DebugLog  │  │ - SIGPIPE ignore    │ │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │                │                │                    │            │
│         ▼                ▼                ▼                    ▼            │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                         Connection Manager                              ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  ││
│  │  │   Listener  │  │   Client    │  │   Client    │  │   Client       │  ││
│  │  │   (Port 80) │  │   (Conn 1)  │  │   (Conn 2)  │  │   (Conn N)     │  ││
│  │  │             │  │   State:    │  │   State:    │  │   State:       │  ││
│  │  │ - bind()    │  │   READ_HDR  │  │   WRITE_RSP │  │   PROCESS_CGI  │  ││
│  │  │ - listen()  │  │             │  │             │  │                │  ││
│  │  │ - epoll_ctl │  │ - recv buf  │  │ - send buf  │  │ - cgi pipe fd  │  ││
│  │  │   (EPOLLIN) │  │ - request   │  │ - response  │  │ - cgi pid      │  ││
│  │  └─────────────┘  │ - timestamp │  │ - timestamp │  │ - timeout      │  ││
│  │                   └─────────────┘  └─────────────┘  └────────────────┘  ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                      HTTP Processing Pipeline                           ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  ││
│  │  │   Request   │  │   Router    │  │   Handler   │  │   Response     │  ││
│  │  │   Parser    │  │             │  │   Factory   │  │   Builder      │  ││
│  │  │             │  │             │  │             │  │                │  ││
│  │  │ - StateMach │  │ - Prefix    │  │ - Static    │  │ - StatusLine   │  ││
│  │  │ - URI decode│  │   match     │  │ - CGI       │  │ - Headers      │  ││
│  │  │ - HeaderVal │  │ - Method    │  │ - Upload    │  │ - Body         │  ││
│  │  │ - Body parse│  │   check     │  │ - Redirect  │  │ - Chunked enc  │  ││
│  │  │ - Chunked   │  │ - Config    │  │ - Autoindex │  │                │  ││
│  │  │   decode    │  │   resolve   │  │ - ErrorPage │  │                │  ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └────────────────┘  ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                        CGI Manager                                      ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                      ││
│  │  │   CgiProcess│  │   EnvSetup  │  │   PipeI/O   │                      ││
│  │  │             │  │             │  │             │                      ││
│  │  │ - fork()    │  │ - RFC 3875  │  │ - epoll     │                      ││
│  │  │ - execve()  │  │   variables │  │   monitor   │                      ││
│  │  │ - waitpid() │  │ - PATH_INFO │  │ - static    │                      ││
│  │  │ - timeout   │  │ - QUERY_STR │  │   buffer    │                      ││
│  │  │   (kill)    │  │ - HTTP_*    │  │ - timeout   │                      ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘                      ││
│  └─────────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Component Responsibilities

#### 4.2.1 Server Engine
- Owns the single `epoll` file descriptor
- Runs the main event loop (`epoll_wait`)
- Manages server lifecycle: initialization → event loop → shutdown
- Coordinates component initialization order

#### 4.2.2 Configuration Loader
- Parses the configuration file (NGINX-inspired syntax)
- Validates directive semantics and dependencies
- Populates `ServerConfig` and `LocationConfig` data structures
- Applies default values for unspecified directives

#### 4.2.3 Connection Manager
- Maintains the set of active `Client` objects
- Accepts new connections from listener sockets
- Closes timed-out or errored connections
- Tracks connection states and timestamps

#### 4.2.4 HTTP Processing Pipeline
- **Request Parser**: State-machine driven HTTP/1.1 parser
- **Router**: Matches URI prefix to location blocks; resolves server block by port
- **Handler Factory**: Dispatches to appropriate handler (static, CGI, upload, redirect)
- **Response Builder**: Constructs HTTP/1.1 responses with proper headers

#### 4.2.5 CGI Manager
- Spawns CGI processes via `fork()`/`execve()`
- Sets up CGI environment per RFC 3875
- Manages pipe I/O through the event loop
- Enforces CGI timeout and cleanup

#### 4.2.6 Logger
- Access log: every request/response in Combined Log Format
- Error log: server errors, client errors, CGI failures
- Debug log: detailed operational events (configurable verbosity)
- All logs written to configurable file paths

#### 4.2.7 Signal Manager
- Registers signal handlers during initialization
- `SIGINT`/`SIGTERM`: trigger graceful shutdown
- `SIGPIPE`: ignored (prevent process termination on broken pipe)
- `SIGCHLD`: non-blocking `waitpid()` for CGI zombie prevention

---

## 5. Data Flow Architecture

### 5.1 Request-Response Flow

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  Client  │────▶│  Kernel  │────▶│  epoll   │────▶│  recv()  │────▶│  Request │
│  Browser │     │  TCP/IP  │     │  ready   │     │  (after  │     │  Parser  │
│          │     │  Stack   │     │  event   │     │  epoll)  │     │  (state) │
└──────────┘     └──────────┘     └──────────┘     └──────────┘     └────┬─────┘
                                                                         │
                                                                         ▼
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  Client  │◀────│  Kernel  │◀────│  epoll   │◀────│  send()  │◀────│  Response│
│  Browser │     │  TCP/IP  │     │  ready   │     │  (after  │     │  Buffer  │
│          │     │  Stack   │     │  event   │     │  epoll)  │     │  (state) │
└──────────┘     └──────────┘     └──────────┘     └──────────┘     └────┬─────┘
                                                                         ▲
                                                                         │
                              ┌──────────────────────────────────────────┘
                              │
                    ┌─────────┴──────────┐
                    │   Handler Output   │
                    │  (Static/CGI/Err)  │
                    └────────────────────┘
```

### 5.2 CGI Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              CGI EXECUTION FLOW                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Client Request (POST /script.php)                                         │
│        │                                                                    │
│        ▼                                                                    │
│   ┌─────────────┐                                                           │
│   │   Router    │──▶ Extension .php ──▶ CGI Handler                         │
│   └─────────────┘                                                           │
│        │                                                                    │
│        ▼                                                                    │
│   ┌─────────────┐     fork()     ┌─────────────┐     execve("php-cgi")      │
│   │   pipe()    │───────────────▶│   Child     │────────────────────────▶   │
│   │  (2 pipes:  │                │  Process    │    stdin ◀── request body  │
│   │   stdin,    │                │             │    stdout ──▶ response     │
│   │   stdout)   │◀───────────────│  (php-cgi)  │                            │
│   └─────────────┘   waitpid()    └─────────────┘                            │
│        │                                                                    │
│        ▼                                                                    │
│   ┌─────────────┐                                                           │
│   │  epoll_ctl  │──▶ Monitor parent-side pipe fds for EPOLLIN/EPOLLOUT      │
│   │  (EPOLLIN on│                                                           │
│   │   stdout,   │                                                           │
│   │  EPOLLOUT on│                                                           │
│   │   stdin)    │                                                           │
│   └─────────────┘                                                           │
│        │                                                                    │
│        ▼                                                                    │
│   ┌─────────────┐                                                           │
│   │  Static Buf │──▶ Read stdout into pre-allocated buffer                  │
│   │  (CGI out)  │                                                           │
│   └─────────────┘                                                           │
│        │                                                                    │
│        ▼                                                                    │
│   ┌─────────────┐                                                           │
│   │   Parse     │──▶ Extract headers, determine body length/EOF             │
│   │   CGI Resp  │                                                           │
│   └─────────────┘                                                           │
│        │                                                                    │
│        ▼                                                                    │
│   ┌─────────────┐                                                           │
│   │   Build     │──▶ HTTP/1.1 response to client                            │
│   │   Response  │                                                           │
│   └─────────────┘                                                           │
│                                                                             │
│   TIMEOUT GUARD: If CGI exceeds configured timeout, send SIGKILL,           │
│   return 504 Gateway Timeout to client.                                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Connection & Session Management

### 6.1 Connection Lifecycle State Machine

```
                    ┌─────────────┐
                    │   CLOSED    │
                    │  (initial)  │
                    └──────┬──────┘
                           │ accept()
                           ▼
                    ┌─────────────┐
         ┌─────────│  ACCEPTED   │◀────────────────────────────┐
         │         │  (epoll:    │                             │
         │         │   EPOLLIN)  │                             │
         │         └──────┬──────┘                             │
         │                │ recv() returns data                │
         │                ▼                                    │
         │         ┌─────────────┐                             │
         │         │READING_HDRS │◀──────┐                     │
         │         │  (parse     │       │ partial data        │
         │         │   request   │───────┘                     │
         │         │   line &    │                             │
         │         │   headers)  │                             │
         │         └──────┬──────┘                             │
         │                │ headers complete                   │
         │                ▼                                    │
         │         ┌─────────────┐     Content-Length/         │
         │         │READING_BODY │◀─── chunked body            │
         │         │  (recv body │──── partial data            │
         │         │   data)     │                             │
         │         └──────┬──────┘                             │
         │                │ body complete                      │
         │                ▼                                    │
         │         ┌─────────────┐                             │
         │         │ PROCESSING  │                             │
         │         │  (route &   │                             │
         │         │   handle)   │                             │
         │         └──────┬──────┘                             │
         │                │                                    │
         │       ┌────────┴────────┐                           │
         │       ▼                 ▼                           │
         │ ┌─────────────┐   ┌─────────────┐                   │
         │ │WRITING_RSP  │   │WRITING_CGI  │                   │
         │ │  (send()    │   │  (write to  │                   │
         │ │   response) │   │   CGI stdin)│                   │
         │ └──────┬──────┘   └──────┬──────┘                   │
         │        │                 │                          │
         │        │ send complete   │ write complete           │
         │        ▼                 ▼                          │
         │ ┌─────────────┐   ┌─────────────┐                   │
         │ │   DONE      │   │READING_CGI  │                   │
         │ │  (close or  │   │  (read CGI  │                   │
         │ │   keep-alive│   │   stdout)   │                   │
         │ └──────┬──────┘   └──────┬──────┘                   │
         │        │                 │ read complete            │
         │        │ keep-alive      ▼                          │
         │        │         ┌─────────────┐                    │
         └────────┘         │ PROCESSING  │────────────────────┘
                            │  (CGI resp  │
                            │   parsed)   │
                            └─────────────┘
```

### 6.2 Timeout Management

| Timeout Type | Trigger | Action |
|--------------|---------|--------|
| **Connection Idle** | No activity for N seconds | Close connection, log timeout |
| **Request Read** | Incomplete request after N seconds | Close connection, return 408 |
| **CGI Execution** | CGI process exceeds configured limit | `SIGKILL` to CGI, return 504 |
| **Response Write** | Client not reading for N seconds | Close connection, log timeout |

**Timeout values are configurable per server block.**

### 6.3 Connection Cleanup

On any of the following events, the connection must be closed and all resources freed:
- Client closes connection (`recv` returns 0)
- Unrecoverable socket error
- Request/response timeout
- Protocol error (malformed HTTP)
- Server shutdown signal received

---

## 7. HTTP Processing Pipeline

### 7.1 Request Parsing Requirements

The parser must handle HTTP/1.1 messages per RFC 9112:

```
HTTP-message   = start-line
                 *( field-line CRLF )
                 CRLF
                 [ message-body ]

start-line     = request-line / status-line
request-line   = method SP request-target SP HTTP-version CRLF
```

**Parsing stages (state machine):**

| Stage | Validates | Transitions To |
|-------|-----------|----------------|
| `PARSE_REQUEST_LINE` | Method, URI, HTTP-version | `PARSE_HEADERS` |
| `PARSE_HEADERS` | Field-name, Field-value, header completeness | `PARSE_BODY` or `PROCESSING` |
| `PARSE_BODY` | Content-Length match, chunked decode complete | `PROCESSING` |

**Required parsing behaviors:**
- **Method validation**: Only `GET`, `POST`, `DELETE` are accepted; others return `405`
- **URI decoding**: Percent-encoded characters decoded per RFC 3986
- **Path traversal prevention**: Reject or sanitize `../` sequences
- **Chunked transfer decoding**: De-chunk request body before CGI forwarding
- **Header size limits**: Configurable maximum header size; exceed → `431 Request Header Fields Too Large`

### 7.2 Routing Logic

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ROUTING DECISION                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Input: (server_block, method, URI)                                         │
│                                                                             │
│  Step 1: Server Selection                                                   │
│    - Match by destination port                                              │
│    - If multiple server blocks on same port, use first declared (default)   │
│                                                                             │
│  Step 2: Location Matching                                                  │
│    - Find longest prefix match of URI against location paths                │
│    - Example: URI /kapouet/pouic matches location /kapouet before /         │
│                                                                             │
│  Step 3: Method Validation                                                  │
│    - Check if method is in location's allowed_methods list                  │
│    - If not allowed → return 405 Method Not Allowed                         │
│                                                                             │
│  Step 4: Action Determination                                               │
│    - If `return` directive present → redirect response                      │
│    - If URI has CGI extension → CGI handler                                 │
│    - If method is POST and upload_path set → upload handler                 │
│    - If URI resolves to file → static file handler                          │
│    - If URI resolves to directory → index file or autoindex                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.3 Response Generation

**HTTP/1.1 Response Format (RFC 9112):**

```
HTTP/1.1 200 OK

Content-Type: text/html

Content-Length: 1234

Connection: keep-alive



[message-body]
```

**Response construction rules:**
- `Connection` header: `keep-alive` if client requested it and server supports it; `close` otherwise
- `Content-Length`: Required for static responses; if CGI returns no Content-Length, use EOF detection
- `Transfer-Encoding: chunked`: Used for streaming responses where length is unknown
- `Date`: Required per RFC 9112
- `Server`: Optional identification header

---

## 8. Static Content Serving

### 8.1 File Resolution

```
URI: /kapouet/pouic/toto/pouet
Location: /kapouet with root /tmp/www

Resolved path: /tmp/www/pouic/toto/pouet
```

**Security checks:**
1. Resolved path must be within the location's root directory (prevent traversal)
2. File must be readable (`access(R_OK)`)
3. Directory must be executable (`access(X_OK)`) for path traversal

### 8.2 Directory Handling

| Condition | Action |
|-----------|--------|
| URI is directory AND index file exists AND autoindex off | Serve index file |
| URI is directory AND autoindex on | Generate directory listing HTML |
| URI is directory AND autoindex off AND no index | Return 403 Forbidden |

### 8.3 MIME Type Detection

File extension to MIME type mapping. Minimum required types:

| Extension | MIME Type |
|-----------|-----------|
| `.html`, `.htm` | `text/html` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.jpg`, `.jpeg` | `image/jpeg` |
| `.png` | `image/png` |
| `.gif` | `image/gif` |
| `.txt` | `text/plain` |
| `.pdf` | `application/pdf` |
| Default | `application/octet-stream` |

---

## 9. CGI Execution Architecture

### 9.1 CGI Trigger Conditions

CGI execution is triggered when:
1. Request URI maps to a file
2. File extension matches a configured `cgi_extension`
3. The CGI interpreter path (`cgi_path`) is configured and executable

### 9.2 CGI Environment Variables (RFC 3875)

| Variable | Source |
|----------|--------|
| `AUTH_TYPE` | From request `Authorization` header |
| `CONTENT_LENGTH` | From request `Content-Length` header |
| `CONTENT_TYPE` | From request `Content-Type` header |
| `GATEWAY_INTERFACE` | Fixed: `CGI/1.1` |
| `PATH_INFO` | URI path after script name |
| `PATH_TRANSLATED` | `PATH_INFO` resolved against document root |
| `QUERY_STRING` | URI query component (after `?`) |
| `REMOTE_ADDR` | Client IP address |
| `REQUEST_METHOD` | HTTP method |
| `SCRIPT_NAME` | URI path to the CGI script |
| `SERVER_NAME` | Server name from config |
| `SERVER_PORT` | Port number |
| `SERVER_PROTOCOL` | Fixed: `HTTP/1.1` |
| `SERVER_SOFTWARE` | Server identification string |
| `HTTP_*` | All request headers mapped to CGI variables |

### 9.3 CGI Process Lifecycle

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CGI PROCESS LIFECYCLE                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. PREPARATION                                                             │
│     - Create two pipes: [server→CGI stdin] and [CGI stdout→server]          │
│     - Set parent-side pipe fds to non-blocking (O_NONBLOCK)                 │
│     - Register both pipes with epoll (stdin: EPOLLOUT, stdout: EPOLLIN)     │
│                                                                             │
│  2. FORK                                                                    │
│     - Child process:                                                        │
│       * Close unused pipe ends                                              │
│       * dup2() stdin pipe to STDIN_FILENO                                   │
│       * dup2() stdout pipe to STDOUT_FILENO                                 │
│       * chdir() to CGI script directory                                     │
│       * Set all RFC 3875 environment variables                              │
│       * execve() the CGI interpreter with script as argument                │
│     - Parent process:                                                       │
│       * Close child-side pipe ends                                          │
│       * Store child PID and pipe fds in Client state                        │
│       * Start CGI timeout timer                                             │
│                                                                             │
│  3. I/O PHASE (via epoll)                                                   │
│     - If request body exists and stdin pipe is writable:                    │
│       * Write body chunks to CGI stdin (non-blocking)                       │
│       * If chunked request, server has already de-chunked the body          │
│     - If stdout pipe is readable:                                           │
│       * Read CGI output into static buffer                                  │
│       * Continue until EOF (pipe closed by CGI exit)                        │
│                                                                             │
│  4. COMPLETION                                                              │
│     - Detect CGI termination (pipe EOF + waitpid() returns)                 │
│     - Parse CGI output: extract headers, determine body                     │
│     - If no Content-Length from CGI, body ends at EOF                       │
│     - Build HTTP/1.1 response from CGI output                               │
│     - Clean up: close pipes, remove from epoll, free resources              │
│                                                                             │
│  5. TIMEOUT HANDLING                                                        │
│     - If CGI exceeds configured timeout:                                    │
│       * Send SIGKILL to child process                                       │
│       * Close pipes, clean up                                               │
│       * Return 504 Gateway Timeout to client                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.4 CGI Response Parsing

CGI output may be:
- **Document response**: Headers + body → wrap in HTTP/1.1 response
- **Local redirect**: `Location: /local/path` + no body → generate 302 redirect
- **Client redirect**: `Location: http://external` + no body → generate 302 redirect
- **Client redirect with document**: `Location:` header + body → pass through

---

## 10. File Upload Architecture

### 10.1 Upload Trigger Conditions

Upload handling is triggered when:
1. Request method is `POST`
2. Request URI matches a location with `upload_path` configured
3. `Content-Type` is `multipart/form-data` (browser uploads) or raw body

### 10.2 Multipart Parsing

```
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary

------WebKitFormBoundary

Content-Disposition: form-data; name="file"; filename="original.txt"

Content-Type: text/plain



[file content bytes]


------WebKitFormBoundary--

```

**Processing:**
1. Extract boundary string from `Content-Type` header
2. Parse parts sequentially
3. For file parts (`filename` present):
   - Generate unique filename: `{timestamp}_{uuid}.{ext}` or similar
   - Write to `upload_path` directory
   - Sanitize original filename (strip path components)
4. For non-file parts: optionally store as form data

### 10.3 Upload Constraints

| Constraint | Behavior |
|------------|----------|
| **Route-level size limit** | Configurable per location; exceeded → `413 Payload Too Large` |
| **Disk space** | Write failure → `500 Internal Server Error` |
| **Path traversal** | Reject filenames containing `..` or absolute paths |
| **Duplicate handling** | Unique naming prevents overwrites |

---

## 11. Configuration Architecture

### 11.1 Configuration Structure

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        CONFIGURATION HIERARCHY                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  webserv.conf                                                               │
│  ├── server {                    # Server Block 1                           │
│  │   ├── listen 127.0.0.1:8080;  # IP:port to bind                          │
│  │   ├── server_name example.com;# Server identifier                        │
│  │   ├── client_max_body_size 10M;# Global body size limit                  │
│  │   ├── error_page 404 /errors/404.html;# Custom error page                │
│  │   ├── error_page 500 502 /errors/50x.html;# Multiple codes               │
│  │   ├── access_log /var/log/webserv/access.log;# Access log path           │
│  │   ├── error_log /var/log/webserv/error.log;# Error log path              │
│  │   ├── debug_log /var/log/webserv/debug.log;# Debug log path              │
│  │   ├── timeout 30s;            # Connection timeout                       │
│  │   ├── cgi_timeout 10s;       # CGI timeout                               │
│  │   │                                                                      │
│  │   ├── location / {            # Location Block (root)                    │
│  │   │   ├── methods GET POST;   # Allowed HTTP methods                     │
│  │   │   ├── root /var/www/html; # Document root                            │
│  │   │   ├── index index.html;   # Default index file                       │
│  │   │   ├── autoindex off;     # Directory listing                         │
│  │   │   └── upload_path /var/www/uploads;# Upload directory                │
│  │   │                                                                      │
│  │   ├── location /kapouet {      # Location Block (prefix /kapouet)        │
│  │   │   ├── methods GET;        # Only GET allowed                         │
│  │   │   ├── root /tmp/www;      # Filesystem root for this route           │
│  │   │   ├── autoindex on;      # Enable directory listing                  │
│  │   │   ├── return 301 /new/path;# HTTP redirect                           │
│  │   │   └── client_max_body_size 5M;# Override global limit                │
│  │   │                                                                      │
│  │   └── location ~ \.(php|py)$ { # CGI Location (extension match)          │
│  │       ├── methods GET POST;   # CGI methods                              │
│  │       ├── cgi_extension .php;# Trigger extension                         │
│  │       ├── cgi_path /usr/bin/php-cgi;# Interpreter path                   │
│  │       └── root /var/www/cgi; # CGI scripts root                          │
│  │                                                                          │
│  └── server {                    # Server Block 2 (different port)          │
│      ├── listen 127.0.0.1:9090;                                             │
│      └── ...                                                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 11.2 Configuration Directives Reference

#### Server-Level Directives

| Directive | Required | Default | Description |
|-----------|----------|---------|-------------|
| `listen` | Yes | — | `IP:port` to bind. Multiple `listen` per server allowed. |
| `server_name` | No | `""` | Server identifier (informational, no virtual host routing) |
| `client_max_body_size` | No | `1M` | Maximum request body size |
| `error_page` | No | Generated | Map status codes to custom error page paths |
| `access_log` | No | `stdout` or disabled | Path to access log file |
| `error_log` | No | `stderr` or disabled | Path to error log file |
| `debug_log` | No | disabled | Path to debug log file |
| `timeout` | No | `60s` | Connection idle timeout |
| `cgi_timeout` | No | `30s` | CGI process timeout |

#### Location-Level Directives

| Directive | Required | Default | Description |
|-----------|----------|---------|-------------|
| `location` | Yes | — | URI prefix to match |
| `methods` | No | `GET POST DELETE` | Allowed HTTP methods |
| `root` | No | Inherited or server root | Filesystem root for this location |
| `index` | No | `index.html` | Default file for directory requests |
| `autoindex` | No | `off` | Enable/disable directory listing |
| `return` | No | — | HTTP redirect (`return code url;`) |
| `upload_path` | No | — | Directory for file uploads |
| `client_max_body_size` | No | Inherited from server | Override global body size limit |
| `cgi_extension` | No | — | File extension triggering CGI |
| `cgi_path` | No | — | Path to CGI interpreter executable |

### 11.3 Configuration Inheritance

- Location blocks inherit server-level values unless explicitly overridden
- `root` directive: if not specified at location level, uses server-level `root` or defaults to system-dependent path
- `client_max_body_size`: location-level overrides server-level
- `error_page`: location-level overrides server-level for that location

### 11.4 Default Server Behavior

When multiple server blocks define the same port, the **first declared server block** is the default. All requests to that port are handled by the matching server block (port-based selection only; no `Host` header virtual hosting).

---

## 12. Error Handling & Resilience

### 12.1 Error Page Strategy

| Scenario | Behavior |
|----------|----------|
| Custom error page configured and readable | Serve custom page with correct status code |
| Custom error page missing or unreadable | Generate minimal HTML dynamically |
| No custom error page configured | Generate minimal HTML dynamically |

**Generated error page format:**
```html
<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body>
</html>
```

### 12.2 HTTP Status Code Accuracy

| Status Code | Trigger Condition |
|-------------|-------------------|
| `200 OK` | Successful GET/POST/DELETE |
| `201 Created` | Successful file upload |
| `204 No Content` | Successful DELETE with no body |
| `301 Moved Permanently` | Permanent redirect (`return 301`) |
| `302 Found` | Temporary redirect (`return 302`) |
| `400 Bad Request` | Malformed HTTP request |
| `403 Forbidden` | Access denied, directory listing disabled |
| `404 Not Found` | Resource does not exist |
| `405 Method Not Allowed` | Method not in location's allowed list |
| `408 Request Timeout` | Incomplete request within timeout window |
| `413 Payload Too Large` | Body exceeds `client_max_body_size` |
| `431 Request Header Fields Too Large` | Headers exceed maximum size |
| `500 Internal Server Error` | Server internal error (memory, disk) |
| `502 Bad Gateway` | CGI process error (invalid output) |
| `504 Gateway Timeout` | CGI process exceeds timeout |

### 12.3 Resilience Mechanisms

| Mechanism | Purpose |
|-----------|---------|
| **Connection timeouts** | Prevent resource exhaustion from idle clients |
| **Request size limits** | Prevent memory exhaustion from oversized requests |
| **CGI timeouts** | Prevent indefinite hangs from buggy scripts |
| **Non-blocking I/O** | Prevent single slow client from blocking all others |
| **Graceful degradation** | Continue serving other clients when one connection fails |
| **Zombie prevention** | `SIGCHLD` handler with non-blocking `waitpid()` |
| **FD limits** | Monitor and handle `EMFILE`/`ENFILE` on accept |

---

## 13. Logging Architecture

### 13.1 Log Types

| Log Type | Format | Content |
|----------|--------|---------|
| **Access Log** | Combined Log Format | Client IP, timestamp, method, URI, status, bytes sent, referer, user-agent |
| **Error Log** | Custom timestamped | Server errors, client errors, CGI failures, configuration errors |
| **Debug Log** | Custom timestamped | Connection events, epoll events, state transitions, parser decisions |

### 13.2 Combined Log Format (Access Log)

```
%h %l %u %t "%r" %>s %b "%{Referer}i" "%{User-agent}i"
```

Example:
```
127.0.0.1 - - [10/Jun/2026:14:32:01 +0000] "GET /index.html HTTP/1.1" 200 1234 "-" "Mozilla/5.0"
```

### 13.3 Error Log Format

```
[YYYY-MM-DD HH:MM:SS] [LEVEL] [COMPONENT] Message
```

Example:
```
[2026-06-10 14:32:01] [ERROR] [CGI] CGI process 12345 timed out after 10s
[2026-06-10 14:32:02] [WARN]  [Client] Connection from 192.168.1.1 closed: timeout
```

### 13.4 Debug Log Format

```
[YYYY-MM-DD HH:MM:SS] [DEBUG] [COMPONENT:FUNCTION] Detailed message
```

Example:
```
[2026-06-10 14:32:01] [DEBUG] [Epoll:wait] 3 fds ready: fd4(EPOLLIN), fd7(EPOLLOUT), fd9(EPOLLIN)
[2026-06-10 14:32:01] [DEBUG] [Parser:request] Method=GET, URI=/kapouet/pouic, Version=HTTP/1.1
```

### 13.5 Log Configuration

- All log paths are configurable per server block
- If log file cannot be opened, fall back to stderr (error) or stdout (access)
- Debug logging is disabled by default; enabled by configuring `debug_log`
- Logs must be flushed periodically to prevent data loss on crash

---

## 14. Signal Handling

### 14.1 Signal Registration

| Signal | Action | Rationale |
|--------|--------|-----------|
| `SIGINT` | Graceful shutdown | User interrupt (Ctrl+C) |
| `SIGTERM` | Graceful shutdown | Process termination request |
| `SIGPIPE` | Ignore | Prevent termination on broken pipe (non-blocking I/O) |
| `SIGCHLD` | Non-blocking `waitpid()` | Reap CGI zombie processes |

### 14.2 Graceful Shutdown Sequence

```
1. Set global shutdown flag
2. Stop accepting new connections (close listener sockets)
3. Wait for active connections to complete or timeout
4. Terminate running CGI processes
5. Flush and close log files
6. Free all allocated resources
7. Exit with status 0
```

### 14.3 SIGPIPE Handling

`SIGPIPE` is ignored at startup via `signal(SIGPIPE, SIG_IGN)`. Additionally, sockets are created with `SO_NOSIGPIPE` where supported, or `send()`/`recv()` use `MSG_NOSIGNAL` flag to prevent `SIGPIPE` generation on broken connections.

---

## 15. Deployment & Execution

### 15.1 Build Requirements

| Requirement | Specification |
|-------------|---------------|
| **Language** | C++98 |
| **Compiler** | g++ or clang++ with `-std=c++98` |
| **Build System** | Makefile with targets: `all`, `clean`, `fclean`, `re` |
| **Executable** | `webserv` |
| **Dependencies** | None (no external libraries) |
| **Platform** | Linux (epoll required) |

### 15.2 Execution

```bash
# With configuration file
./webserv /path/to/webserv.conf

# Default configuration path (if no argument provided)
./webserv
# Searches for: ./webserv.conf, /etc/webserv/webserv.conf, etc.
```

### 15.3 Runtime Requirements

| Requirement | Specification |
|-------------|---------------|
| **User privileges** | Standard user (no root required if binding to ports > 1024) |
| **CGI interpreters** | `php-cgi`, `python3`, etc. must be installed and executable |
| **File permissions** | Read access to document root; write access to upload paths and log directories |
| **File descriptors** | Sufficient ulimit for expected concurrent connections |

---

## 16. Appendices

### Appendix A: RFC Reference Mapping

| RFC | Title | Relevance |
|-----|-------|-----------|
| RFC 9112 | HTTP/1.1: Message Syntax and Routing | HTTP/1.1 message format, persistent connections, chunked encoding |
| RFC 9110 | HTTP Semantics | Method semantics, status code definitions, header field semantics |
| RFC 3875 | The Common Gateway Interface (CGI) Version 1.1 | CGI environment variables, input/output conventions |
| RFC 3986 | Uniform Resource Identifier (URI): Generic Syntax | URI encoding, percent-encoding, path resolution |
| RFC 2046 | Multipurpose Internet Mail Extensions (MIME) Part Two | `multipart/form-data` parsing for file uploads |
| RFC 6266 | Content-Disposition Header Field | File upload handling hints |

### Appendix B: External Function Usage

| Function | Usage Context |
|----------|---------------|
| `socket`, `bind`, `listen`, `accept` | Listener socket creation |
| `connect` | Outbound connections (if needed) |
| `setsockopt` | Socket options (SO_REUSEADDR, SO_NOSIGPIPE) |
| `getsockname` | Retrieve local socket address |
| `getaddrinfo`, `freeaddrinfo` | Address resolution |
| `getprotobyname` | Protocol lookup |
| `send`, `recv` | Socket I/O (only after epoll readiness) |
| `htons`, `htonl`, `ntohs`, `ntohl` | Byte order conversion |
| `fcntl` | Set non-blocking mode, file descriptor flags |
| `epoll_create`, `epoll_ctl`, `epoll_wait` | I/O event multiplexing |
| `pipe` | CGI inter-process communication |
| `dup`, `dup2` | Redirect stdin/stdout for CGI |
| `fork` | CGI process creation ONLY |
| `execve` | Execute CGI interpreter |
| `waitpid` | Reap CGI child processes |
| `kill` | Terminate timed-out CGI processes |
| `signal` | Register signal handlers |
| `chdir` | Change CGI working directory |
| `open`, `close`, `read`, `write` | File operations (disk files exempt from epoll) |
| `stat` | File metadata (size, permissions, type) |
| `access` | File accessibility checks |
| `opendir`, `readdir`, `closedir` | Directory listing generation |
| `strerror` | Error message generation (not for errno after read/write) |
| `gai_strerror` | getaddrinfo error messages |
| `errno` | General error checking (NOT after read/write on sockets/pipes) |

### Appendix C: Glossary

| Term | Definition |
|------|------------|
| **epoll** | Linux I/O event notification mechanism |
| **Reactor Pattern** | Event-driven design pattern: demultiplex events to handlers |
| **Non-blocking I/O** | Operations return immediately without waiting for completion |
| **CGI** | Common Gateway Interface: protocol for web servers to execute external programs |
| **State Machine** | Computation model where behavior depends on current state and inputs |
| **RAII** | Resource Acquisition Is Initialization: bind resource lifecycle to object lifetime |
| **Keep-Alive** | HTTP/1.1 persistent connection feature |
| **Chunked Transfer Encoding** | HTTP body transmission without prior Content-Length |
| **MIME Type** | Media type identifier for file content classification |

---

*End of Document*
