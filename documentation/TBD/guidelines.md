# Coding Standards & Guidelines

> **Project:** webserv — HTTP/1.1 Server in C++98
> **Audience:** all contributors (rluis-ya, avieira-, jcesar-s)
> **Enforcement:** reviewed at every merge. Code that violates a **MUST**
> rule does not merge.

Legend: **MUST** = hard rule (subject or safety). **SHOULD** = strong default;
deviations need a code-review justification.

---

## 1. Language & Toolchain

- **MUST** compile with `c++ -std=c++98 -Wall -Werror -Wextra` with **zero
  warnings**. Warnings are errors in this project.
- **MUST NOT** use any C++11+ feature: no `auto`, no range-based `for`, no
  lambdas, no `nullptr` (use `NULL`), no smart pointers, no `std::thread`,
  no `std::to_string`, no `unordered_*` containers.
- **MUST NOT** use external libraries. The C++98 standard library and POSIX
  system calls only.
- Target platform is Linux; `epoll` is allowed and required.

### Allowed standard headers (non-exhaustive)

`<iostream>`, `<string>`, `<vector>`, `<map>`, `<set>`, `<algorithm>`,
`<sstream>`, `<cstring>`, `<cstdlib>`, `<csignal>`, `<cerrno>` (see §3),
`<ctime>`, `<stdint.h>` plus POSIX headers (`<sys/socket.h>`, `<sys/epoll.h>`,
`<netinet/in.h>`, `<arpa/inet.h>`, `<fcntl.h>`, `<unistd.h>`, `<sys/stat.h>`,
`<sys/wait.h>`, `<dirent.h>`).

---

## 2. Subject Constraints (Non-Negotiable)

These come from the project subject and architecture document (constraint
IDs C1–C8). Violating any of these is an automatic merge rejection.

### C1 — C++98 only
Covered in §1.

### C2 — Single epoll instance
**MUST** register every socket and every CGI pipe in the one `EpollReactor`
owned by `Server`. No `select()`, no `poll()`, no second epoll.

### C3 — Everything non-blocking
**MUST** call `fcntl(fd, F_SETFL, O_NONBLOCK)` on every accepted client
socket and every parent-side CGI pipe end. (Disk files are exempt from both
this rule and epoll registration.)

### C4 — No I/O without epoll readiness
**MUST NOT** call `recv()`, `send()`, `read()`, or `write()` on a socket or
pipe unless epoll reported that fd ready (`EPOLLIN` / `EPOLLOUT`) in the
current event batch.

### C5 — No `errno` after read/write  ⚠️ most commonly violated
**MUST NOT** inspect `errno` after `recv()`, `send()`, `read()`, or
`write()` on sockets/pipes. Decide from the return value alone:

| Call | Return | Action |
|------|--------|--------|
| `recv` | `> 0` | data — process it |
| `recv` | `0` | peer closed — close connection |
| `recv` | `-1` | fatal — close connection (epoll said ready; `EAGAIN` "cannot" happen) |
| `send` | `>= 0` | advance send offset; if incomplete, wait for next `EPOLLOUT` |
| `send` | `-1` | fatal — close connection |

`errno` **is** allowed after: `socket`, `bind`, `listen`, `accept`,
`connect`, `setsockopt`, `fcntl`, `epoll_*`, `pipe`, `fork`, `execve`,
`waitpid`, `open`, `stat`, `access`, `opendir` (Appendix B of the
architecture document). Use it there for diagnostics.

### C6 — `fork()` only for CGI
**MUST NOT** create threads or fork for connection handling. One process,
one thread, one loop.

### C7 — Self-contained
**MUST NOT** `execve` another web server (nginx, apache, ...). CGI
interpreters (`php-cgi`, `python3`) are the only legal `execve` targets.

### C8 — Browser compatibility
Responses **MUST** be valid HTTP/1.1: correct status line, `Content-Length`
(or chunked), `Date`, `Connection`. A real browser must render the result.

---

## 3. Error Handling Philosophy

- **No exceptions.** Return `bool` / status enums; check every return value.
- **Fail fast at startup**: configuration or listener errors → print a clear
  message to `stderr` and exit non-zero.
- **Fail safe at runtime**: a bad client/request/CGI **MUST NEVER** crash or
  hang the server. Close that connection, log, continue.
- Every system call return value is checked. Silent swallowing is a bug.

---

## 4. Memory & Resource Management

- **RAII for every resource.** A file descriptor, heap allocation, or child
  process is owned by exactly one object whose destructor releases it.
  (`Socket`, `EpollReactor`, and — in Phase 3 — `CgiProcess` are the models.)
- **Non-copyable owners.** Any class owning a raw resource **MUST** disable
  copying using the C++98 idiom:

```cpp
class Socket
{
    // ...
private:
    Socket(const Socket&);              // declared, never defined
    Socket& operator=(const Socket&);   // declared, never defined
};
```

- **`new`/`delete` pairing.** Heap-allocated `Client` objects are `delete`d
  in exactly one place (`Server::closeClient`). After `delete`, the pointer
  is removed from the registry in the same statement block.
- **No leaks.** `valgrind --leak-check=full` must report zero leaks before a
  phase is declared done.
- **Zombies.** CGI children are reaped with non-blocking
  `waitpid(-1, &status, WNOHANG)` inside the event loop.

---

## 5. Naming Conventions

| Entity | Convention | Example |
|--------|-----------|---------|
| Types (class/struct) | `PascalCase` | `Server`, `HttpRequest`, `LocationConfig` |
| Functions / methods | `camelCase` | `handleClientRead()`, `bindToPort()` |
| Member variables | `_camelCase` (leading underscore) | `_listener`, `_epollFd`, `_running` |
| Local variables | `camelCase` | `bytesRead`, `clientFd` |
| Constants / enums | `ALL_CAPS` | `DEFAULT_PORT`, `READING_REQUEST` |
| Macros | `ALL_CAPS` (avoid macros; prefer `static const`) | — |
| Files | `PascalCase.hpp/.cpp` matching the class | `EpollReactor.cpp` |
| Header guards | `NAME_HPP` | `EPOLLREACTOR_HPP` |

---

## 6. Code Layout & Formatting

- **Indentation:** 4 spaces, no tabs.
- **Braces:** Allman style (opening brace on its own line) for classes and
  functions; same-line braces acceptable for short control blocks — pick one
  per file and stay consistent.
- **Line length:** 100 columns soft limit.
- **One class per file pair** (`X.hpp` / `X.cpp`). Small tightly-coupled
  helper structs may share a header.
- **Includes:** project headers first (`"Server.hpp"`), then C++ standard
  headers, then POSIX headers. Keep the list minimal — no unused includes.
- **`.hpp` files** contain declarations only (inline one-liner getters are
  acceptable). No `using namespace` in headers, ever.
- **Comments:** explain *why*, not *what*. Mark subject-driven decisions:

```cpp
// Epoll said EPOLLOUT. If send() still fails, the connection is dead.
// No errno check per subject constraint C5.
closeClient(clientFd);
```

---

## 7. Class Design Defaults

- **Composition over inheritance.** `Server` *has* a `Socket`; it does not
  *inherit* one. Inheritance needs review justification.
- Every class gets the Orthodox Canonical Form treatment:
  default constructor, destructor, and — if copy is disabled — the private
  declarations from §4.
- Keep classes small and single-purpose: `Socket` does sockets,
  `EpollReactor` does epoll, `HttpRequest` holds a parsed request. If a
  class needs a comment longer than three lines to explain its job, split it.

---

## 8. Logging

- Phase 0–1: `std::cout` for lifecycle events (accepted/closed, listening
  port) and `std::cerr` for errors. Every line includes the fd when relevant:
  `New connection accepted (fd=5)`.
- Phase 4: migrate to the `Logger` component (access/error logs, Combined
  Log Format) per `ARCHITECTURE.md` §13. Until then, do not build ad-hoc
  logging frameworks.

---

## 9. Git Workflow

- **Branches:** `main` is always releasable. Work happens in
  `feature/<phase>-<short-name>` branches (e.g., `feature/p1-client-class`).
- **Commits:** small, compiling, one concern each. Message format:
  `<type>: <summary>` where type ∈ `feat`, `fix`, `refactor`, `docs`,
  `test`, `build`.
- **Before every commit:** `make re` passes with zero warnings.
- **Before every merge to `main`:**
  1. All phase acceptance tests from `IMPLEMENTATION_ROADMAP.md` pass.
  2. `valgrind` clean (from Phase 1 onward).
  3. Review by at least one other team member.

---

## 10. Definition of Done

A task is **done** only when all of the following hold:

1. Compiles with zero warnings under the §1 flags.
2. Complies with every **MUST** in §2 (spot-check C4/C5 by grepping the diff
   for `errno` after I/O calls).
3. Its acceptance test row in `IMPLEMENTATION_ROADMAP.md` passes.
4. No fd or memory leaks (valgrind / `lsof` as applicable).
5. Code reviewed and merged to `main`.

---

## 11. Quick Pre-Commit Checklist

```
□ make re                      → zero warnings
□ grep -n "errno" src/         → no errno after recv/send/read/write
□ All sockets non-blocking?    → fcntl right after accept()/pipe()
□ Every new fd in epoll?       → addFd() before any recv/send
□ Every removed fd deleted?    → delete in closeClient only
□ Valgrind clean (Phase 1+)?   → yes
□ Roadmap checkbox ticked?     → update IMPLEMENTATION_ROADMAP.md
```
