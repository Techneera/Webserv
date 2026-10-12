# Implementation Roadmap

> **Project:** webserv — HTTP/1.1 Server in C++98
> **Reference:** `documentation/architecture/ARCHITECTURE.md` (v0.1-spec, frozen)
> **Status:** Active

This document is the single source of truth for **what to build next**. The
architecture document defines *what* the system is; this roadmap defines *the
order in which we build it*.

---

## Golden Rules

1. **A phase is not done until its acceptance tests pass.** No moving on with
   known failures "to fix later".
2. **Commit working code only.** `make` must compile with **zero warnings**
   (`-Wall -Werror -Wextra -std=c++98`) before every commit.
3. **One component at a time.** Do not write the HTTP parser while the
   connection manager is still unstable.
4. **Respect the subject constraints at all times** (see
   `documentation/TBD/guidelines.md` §2): no `errno` after read/write, all I/O
   non-blocking and gated by epoll, `fork()` only for CGI.

---

## Phase 0: Foundations ✅ (Complete)

**Goal:** Buildable skeleton with a working event loop.

- [x] `Socket` — RAII wrapper (`socket`, `bind`, `listen`, `accept`, `fcntl`)
- [x] `EpollReactor` — thin wrapper (`epoll_create`, `epoll_ctl`, `epoll_wait`)
- [x] `Server` — event loop accepting connections, static `200 OK` response
- [x] Signal handling (`SIGINT`, `SIGTERM` graceful stop; `SIGPIPE` ignored)
- [x] Makefile with header dependency tracking (`-MMD -MP`), separate debug binary
- [x] Subject compliance: no `errno` after `recv()`/`send()`

**Acceptance (met):** `curl http://localhost:8080/` returns `Hello, World!`;
two concurrent clients both served; Ctrl+C shuts down cleanly.

---

## Phase 1: Connection Management

**Goal:** Robust multi-client handling with explicit connection state,
buffers, and timeouts.

### Tasks

- [ ] **`Client` class** (new files `include/Client.hpp`, `src/Client.cpp`)
  - Owns: client fd, read buffer, write buffer, state enum, last-activity timestamp
  - States: `READING_REQUEST`, `REQUEST_READY`, `WRITING_RESPONSE`, `CLOSING`
- [ ] **Connection registry** in `Server`
  - `std::map<int, Client*>` (fd → client)
  - Allocate `Client` on `accept()`, delete in `closeClient()`
- [ ] **Read buffer accumulation**
  - Loop `recv()` until `-1` (return value only — no `errno`) or buffer full
  - Keep partial request data across multiple `EPOLLIN` events
- [ ] **Write buffer with partial-send handling**
  - Track `bytesSent` offset; resume on next `EPOLLOUT`
  - Switch fd interest `EPOLLIN` ↔ `EPOLLOUT` via `modifyFd()`
- [ ] **Idle connection timeout**
  - On every `epoll_wait` return, scan registry for `now - lastActivity > timeout`
  - Close expired connections; log the reason
- [ ] **Keep-alive support**
  - Parse `Connection:` header value (temporary simple substring match is OK
    for this phase; the real parser comes in Phase 2)
  - On `keep-alive`: after response sent, reset state to `READING_REQUEST`
    instead of closing

### Acceptance Tests

| # | Test | Expected |
|---|------|----------|
| 1.1 | 10 concurrent `curl` requests | All return 200; no hangs |
| 1.2 | Client connects, sends nothing | Closed after timeout; logged |
| 1.3 | Request sent in two TCP segments (e.g., `nc` with delay) | Correct response after second segment |
| 1.4 | `curl` with keep-alive, 2 requests on one connection | Both responses on same connection |
| 1.5 | `valgrind --leak-check=full` after 100 connections | Zero leaked blocks |

### Deliverable

A server that never blocks, never leaks, and survives idle, slow, and
abruptly-disconnecting clients.

---

## Phase 2: HTTP Parsing & Configuration

**Goal:** Real HTTP/1.1 request parsing and NGINX-style configuration files.

### Tasks — Configuration

- [ ] **`ConfigParser`** — tokenizer + block parser
  - `server { ... }` blocks, `location { ... }` blocks
  - Directives: `listen`, `server_name`, `root`, `index`, `methods`,
    `autoindex`, `client_max_body_size`, `error_page`, `cgi_extension`,
    `cgi_path`, `upload_path`, `return`
- [ ] **`ServerConfig` / `LocationConfig`** data structures with defaults
  and inheritance (location overrides server)
- [ ] **Multi-port listeners** — one `Socket` per distinct `listen` directive,
  all registered in the same epoll instance
- [ ] **Config validation** — unknown directive, duplicate port, missing
  `root` → fail fast at startup with a clear message

### Tasks — HTTP Parsing

- [ ] **`HttpRequest`** — method, target, version, headers (`std::map`), body
- [ ] **Request parser state machine** — `REQUEST_LINE → HEADERS → BODY → COMPLETE`
  - Feeds from `Client` read buffer incrementally
  - Detects end of headers (`\r\n\r\n`)
  - `Content-Length` bodies; chunked transfer decoding (`Transfer-Encoding: chunked`)
- [ ] **Validation & limits**
  - Methods: `GET`, `POST`, `DELETE` only → `405` otherwise
  - URI percent-decoding per RFC 3986
  - Reject `..` path traversal
  - Header size limit → `431`; body size limit → `413`; malformed → `400`
- [ ] **Router** — server selection by port, longest-prefix location match,
  method check against `methods`
- [ ] **`HttpResponse`** — status line, headers, body; correct
  `Content-Length`, `Date`, `Connection`

### Acceptance Tests

| # | Test | Expected |
|---|------|----------|
| 2.1 | Config with 2 server blocks on ports 8080/9090 | Both ports serve independently |
| 2.2 | `curl -X PUT /` | `405 Method Not Allowed` |
| 2.3 | `GET /../etc/passwd` | `400` or `403`, never file contents |
| 2.4 | Chunked POST body (curl `-H "Transfer-Encoding: chunked"`) | Body fully decoded |
| 2.5 | Body > `client_max_body_size` | `413 Payload Too Large` |
| 2.6 | Longest-prefix: locations `/` and `/kapouet`, request `/kapouet/x` | Routed to `/kapouet` block |
| 2.7 | Invalid config file | Startup failure with line number |

### Deliverable

A configurable, multi-port HTTP/1.1 server that parses real browser requests
correctly.

---

## Phase 3: Static Content, Uploads & CGI

**Goal:** The full feature set required by the subject.

### Tasks — Static Content

- [ ] **File serving** — resolve URI against location `root`, `stat()` checks
- [ ] **MIME types** — extension table (html, css, js, png, jpg, gif, txt, pdf)
- [ ] **Directory handling** — `index` file, or `autoindex on` listing, else `403`
- [ ] **DELETE method** — remove file under `root`, `204` on success

### Tasks — Uploads

- [ ] **`multipart/form-data` parser** — boundary extraction, part iteration
- [ ] **Filename sanitization** — strip path components, generate unique names
- [ ] Write to `upload_path`; disk failure → `500`; success → `201`

### Tasks — CGI

- [ ] **`CgiProcess`** — `pipe()` ×2, `fork()`, in child: `dup2`, `chdir`,
  env setup, `execve`
- [ ] **RFC 3875 environment** — `REQUEST_METHOD`, `QUERY_STRING`,
  `CONTENT_LENGTH`, `SCRIPT_NAME`, `PATH_INFO`, `HTTP_*`, etc.
- [ ] **Pipe fds in epoll** — write request body on `EPOLLOUT`, read CGI
  output on `EPOLLIN`, detect EOF
- [ ] **CGI timeout** — `SIGKILL` after configured limit → `504`
- [ ] **Zombie reaping** — non-blocking `waitpid()` in the event loop
- [ ] **CGI response parsing** — split headers/body; synthesize status line

### Acceptance Tests

| # | Test | Expected |
|---|------|----------|
| 3.1 | `GET /index.html` from `root` | File contents, correct MIME type |
| 3.2 | Directory with `autoindex on` | HTML listing |
| 3.3 | `DELETE /file.txt` | File removed, `204` |
| 3.4 | Browser file upload | File appears in `upload_path`, `201` |
| 3.5 | PHP/Python CGI script | Script output returned as response body |
| 3.6 | CGI infinite loop | `504` after timeout; no zombie process |
| 3.7 | 100 CGI requests | `ps aux` shows no defunct children |

### Deliverable

Feature-complete per the subject: static files, uploads, CGI, DELETE, correct
error pages.

---

## Phase 4: Hardening & Polish

**Goal:** Eval-ready quality.

### Tasks

- [ ] **Error pages** — custom `error_page` files; generated fallback HTML
- [ ] **Logging** — access log (Combined Log Format), error log
- [ ] **Stress testing** — `siege -c 100 -t 30s`; server must never crash,
  never block, and remain responsive
- [ ] **FD audit** — `lsof -p <pid>` before/after stress; count must return
  to baseline (no fd leaks)
- [ ] **Memory audit** — full valgrind run under load: zero leaks, zero
  invalid reads/writes
- [ ] **README** — build instructions, config example, feature list

### Acceptance Tests

| # | Test | Expected |
|---|------|----------|
| 4.1 | `siege -c 255 -t 60s` against static + CGI mix | Availability ≥ 99%, zero crashes |
| 4.2 | Full browser test (Firefox/Chrome) | Pages render, cookies/sessions unaffected |
| 4.3 | Subject evaluation sheet walkthrough | Every mandatory point demonstrable |

### Deliverable

The final `webserv` binary, ready for evaluation.

---

## Timeline Summary

| Phase | Focus | Estimated Effort |
|-------|-------|------------------|
| 0 | Foundations (Socket, epoll, loop) | ✅ Done |
| 1 | Connection management | 1–2 weeks |
| 2 | HTTP parsing + configuration | 2 weeks |
| 3 | Static + uploads + CGI | 2 weeks |
| 4 | Hardening & polish | 1 week |

## Current Status

**→ Next task: Phase 1, Task 1 — implement the `Client` class.**
