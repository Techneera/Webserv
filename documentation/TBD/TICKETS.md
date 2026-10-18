# webserv — System Requirements (Kanban Tickets)

**Parent documents**: 42 `webserv` subject (mandatory part); `documentation/architecture/ARCHITECTURE.md` (HLA).
**Scope**: these tickets are the high-level **product requirements** for the `webserv` server. They state *what* the product must do, not *how*; design decisions live in the HLA section referenced by each ticket.

## Conventions (per writing guidelines C.1–C.4)

- **shall** = binding requirement · **will** = fact or declaration of purpose · **should** = goal (used for bonus scope only).
- The product is referred to consistently as **"the server"** (the `webserv` executable and its runtime process).
- Each requirement is a single statement with one subject and one predicate, uniquely numbered `WEB-<ticket>-R<n>` for bidirectional traceability.
- Verification method per requirement: **[I]**nspection (code/config review), **[A]**nalysis (incl. code audit against constraints), **[D]**emonstration (live feature walkthrough), **[T]**est (automated or scripted check with pass/fail criteria).
- Priorities: **P0** subject-mandatory core · **P1** subject-mandatory supporting · **P2** quality/evaluation · **P3** bonus (goal-level).

## Glossary

| Term | Definition |
|------|------------|
| server | The `webserv` executable / running process (the product) |
| multiplexer | The single I/O event-notification instance (`epoll`) monitoring all sockets and pipes |
| readiness | A multiplexer event reporting a descriptor as readable and/or writable |
| connection | One accepted client TCP session, from `accept()` to close |
| server block | A `server { }` configuration unit bound to one or more `IP:port` pairs |
| location | A `location <prefix> { }` configuration unit inside a server block |
| root | The filesystem directory a location's URIs resolve against |
| CGI | A child process executed per RFC 3875 to generate a response |

## Assumptions (to be confirmed before baselining)

- **A1**: Target platform is Linux; all development and evaluation hosts provide `epoll`.
- **A2**: At least one CGI interpreter (`php-cgi` or `python3`) is installed and executable on the evaluation host.
- **A3**: Evaluation binds to ports > 1024, so no elevated privileges are required.
- **A4**: A standard web browser (per subject) and NGINX are available for compatibility comparison.

## TBR Register (best estimates pending resolution)

| ID | Value | Best estimate | Rationale | Resolution |
|----|-------|---------------|-----------|------------|
| TBR-1 | Default maximum request header size | 8 KB | Matches NGINX default; HLA §7.1 mandates a configurable limit without a value | Fix during WEB-030; owner: parser implementer |
| TBR-2 | Stress-test load parameters | ≥ 100 concurrent connections, ≥ 60 s sustained | Must prove availability under load without over-specifying hardware | Fix when writing the WEB-062 test plan |
| TBR-3 | Policy when one of several `listen` binds fails | Abort with non-zero exit | Fail-fast matches NGINX behavior and HLA "fail fast, fail safe"; continuing partially may hide misconfiguration | Confirm with team before WEB-021 baselining |

---

## Epic 0 — Project Foundation

### WEB-001 — Build system — P0

**Intent.** The project will build through a Makefile into a single self-contained executable.

**Requirements**

- **WEB-001-R1**: The project shall provide a Makefile exposing the targets `all`, `clean`, `fclean`, and `re`. [T]
- **WEB-001-R2**: The default build shall produce an executable named `webserv`. [T]
- **WEB-001-R3**: The server shall compile under `-std=c++98 -Wall -Wextra -Werror` with zero errors and zero warnings. [T]
- **WEB-001-R4**: The server shall depend only on the C++98 standard library and the subject-authorized system functions. [I]
- **WEB-001-R5**: The server shall accept a configuration file path as its first command-line argument. [T]

**Rationale.** Subject-mandated submission format (program name, Makefile targets, C++98, argument form).
**Subject trace**: Program name; Files to Submit; Makefile; Arguments; External Functions.
**HLA ref**: §15.1, §15.2.

### WEB-002 — Shared data model and coding baseline — P1

**Intent.** All modules will share one set of constants, types, and class conventions so the codebase stays uniform.

**Requirements**

- **WEB-002-R1**: Shared constants (buffer sizes, event limits, default timeouts) shall be defined in exactly one common header. [I]
- **WEB-002-R2**: All production code shall conform to ISO/IEC 14882:1998 (C++98) with no C++11 or later constructs. [I]
- **WEB-002-R3**: All code, comments, and identifiers shall be written in English. [I]

**Rationale.** C1 constraint (HLA §3.1) and project style rules; centralized constants prevent divergent magic numbers across modules.
**HLA ref**: §3.1 (C1); AGENTS.md code style.

---

## Epic 1 — Configuration

### WEB-010 — Configuration file parsing — P0

**Intent.** The server will derive its entire runtime behavior from one NGINX-inspired configuration file.

**Requirements**

- **WEB-010-R1**: When no argument is given, the server shall load the configuration from the default path `./webserv.conf`. [T]
- **WEB-010-R2**: The server shall parse one or more server blocks from the configuration file. [T]
- **WEB-010-R3**: The server shall support one or more `listen` `IP:port` pairs per server block. [T]
- **WEB-010-R4**: The server shall support the server-level directives `server_name`, `client_max_body_size`, `error_page`, `access_log`, `error_log`, `debug_log`, `timeout`, and `cgi_timeout`. [T]
- **WEB-010-R5**: The server shall support prefix-matched location blocks (no regular expressions) containing the directives `methods`, `root`, `index`, `autoindex`, `return`, `upload_path`, `client_max_body_size`, `cgi_extension`, and `cgi_path`. [T]
- **WEB-010-R6**: The server shall apply the documented default value to every directive the configuration omits. [T]
- **WEB-010-R7**: A location-level directive shall override the corresponding server-level value for that location. [T]
- **WEB-010-R8**: On invalid syntax, an unknown directive, or a missing mandatory directive, the server shall report the error and exit non-zero before opening any socket. [T]
- **WEB-010-R9**: On an unreadable configuration file, the server shall report the error and exit non-zero. [T]

**Rationale.** The subject requires every listed capability to be configurable; failing fast on bad configuration (HLA §3.2) prevents running with unintended behavior.
**Subject trace**: "Define all the interface:port pairs"; "Set up default error pages"; "maximum allowed size for client request bodies"; all route-rule bullets; "configuration file ... or available in a default path".
**HLA ref**: §11.
**Depends on**: WEB-002.

---

## Epic 2 — Server Core (Event Loop & Connections)

### WEB-020 — Single event multiplexer — P0

**Intent.** One multiplexer will govern all I/O readiness; the server never performs unnotified or blocking I/O.

**Requirements**

- **WEB-020-R1**: The server shall use exactly one multiplexer instance for all socket and CGI-pipe I/O. [A]
- **WEB-020-R2**: The server shall register every listening socket, client socket, and CGI pipe with the multiplexer. [I]
- **WEB-020-R3**: The server shall register each descriptor for read readiness, write readiness, or both, matching its current state. [A]
- **WEB-020-R4**: The server shall set every socket and CGI pipe to non-blocking mode. [I]
- **WEB-020-R5**: The server shall perform a `recv`, `send`, `read`, or `write` on a socket or pipe only after the multiplexer has reported readiness for that descriptor. [A]
- **WEB-020-R6**: The server shall not consult `errno` after a read or write operation; I/O outcomes shall be derived from return values and multiplexer events. [A]
- **WEB-020-R7**: The server shall block only inside the multiplexer wait call; no busy-waiting is permitted. [A]

**Rationale.** Direct subject constraints C2–C5; they define the server's fundamental availability and fairness behavior.
**Subject trace**: "use only 1 poll() (or equivalent)"; "monitor both reading and writing simultaneously"; "never do a read or write operation without going through poll()"; "Checking the value of errno ... is strictly forbidden"; "remain non-blocking at all times".
**HLA ref**: §3.1 (C2–C5), §4.2.1, §5.1.
**Depends on**: WEB-002.

### WEB-021 — Listening sockets on multiple ports — P0

**Intent.** Every configured `IP:port` pair will accept connections concurrently.

**Requirements**

- **WEB-021-R1**: The server shall bind and listen on every configured `IP:port` pair. [T]
- **WEB-021-R2**: The server shall serve each port's configured content concurrently with all other ports. [D]
- **WEB-021-R3**: The server shall permit rebinding a listening port immediately after a server restart. [T]
- **WEB-021-R4**: The server shall accept a new connection only after read readiness on a listening socket. [A]
- **WEB-021-R5**: If any `listen` bind fails, the server shall report the failing address and exit non-zero *(TBR-3)*. [T]

**Rationale.** Subject requires multiple ports delivering different content; R3 removes restart friction from the evaluation workflow; R5 fail-fast policy pending TBR-3.
**Subject trace**: "listen to multiple ports to deliver different content".
**HLA ref**: §4.2.3, §11.4.
**Depends on**: WEB-010, WEB-020.

### WEB-022 — Client connection lifecycle — P0

**Intent.** Each connection is a state machine with buffered partial I/O; connections are isolated from one another.

**Requirements**

- **WEB-022-R1**: The server shall resume an incomplete request reception across successive read-readiness events. [T]
- **WEB-022-R2**: The server shall resume an incomplete response transmission across successive write-readiness events. [T]
- **WEB-022-R3**: The server shall support HTTP/1.1 persistent connections (keep-alive). [T]
- **WEB-022-R4**: The server shall close the connection after the response when the client requests `Connection: close`. [T]
- **WEB-022-R5**: Upon client disconnection or an unrecoverable socket error, the server shall close the connection and release all associated resources. [T]
- **WEB-022-R6**: The failure of any single connection shall not interrupt the service of any other connection. [T]

**Rationale.** Non-blocking partial I/O is the core of the single-threaded model; per-connection isolation is the subject's "handle client disconnections" and "remain available" requirement.
**Subject trace**: "remain non-blocking at all times and properly handle client disconnections"; browser compatibility.
**HLA ref**: §6.1, §6.3.
**Depends on**: WEB-021.

### WEB-023 — Connection and request timeouts — P1

**Intent.** Bounded waiting guarantees no request occupies the server indefinitely.

**Requirements**

- **WEB-023-R1**: The server shall close a connection with no activity for longer than the configured `timeout` (default: 60 s). [T]
- **WEB-023-R2**: The server shall respond `408 Request Timeout` when a request remains incomplete past the configured timeout. [T]
- **WEB-023-R3**: The server shall close a connection when the client stops reading the response for longer than the configured timeout. [T]
- **WEB-023-R4**: Timeout durations shall be configurable per server block. [T]

**Rationale.** Subject: "A request to your server should never hang indefinitely"; defaults per HLA §11.2.
**Subject trace**: "A request to your server should never hang indefinitely".
**HLA ref**: §6.2.
**Depends on**: WEB-022.

---

## Epic 3 — HTTP Pipeline

### WEB-030 — HTTP/1.1 request parsing — P0

**Intent.** Every request is validated against RFC 9112 incrementally, with hard limits on size.

**Requirements**

- **WEB-030-R1**: The server shall parse the request line and headers per RFC 9112, incrementally across partial reads. [T]
- **WEB-030-R2**: The server shall accept the methods `GET`, `POST`, and `DELETE`. [T]
- **WEB-030-R3**: The server shall respond `405 Method Not Allowed` to any other method. [T]
- **WEB-030-R4**: The server shall percent-decode request URIs per RFC 3986. [T]
- **WEB-030-R5**: The server shall accept message bodies delimited by `Content-Length`. [T]
- **WEB-030-R6**: The server shall decode `Transfer-Encoding: chunked` request bodies into a contiguous body. [T]
- **WEB-030-R7**: The server shall respond `431 Request Header Fields Too Large` when headers exceed the configured maximum (default: 8 KB, *TBR-1*). [T]
- **WEB-030-R8**: The server shall respond `413 Payload Too Large` when a body exceeds the applicable `client_max_body_size` (default: 1 MB). [T]
- **WEB-030-R9**: The server shall respond `400 Bad Request` to a malformed request. [T]

**Rationale.** Correct parsing with explicit limits is both a subject requirement and the first defense against untrusted input (HLA §3.2 "Defensive Parsing").
**Subject trace**: "GET, POST, and DELETE methods"; "maximum allowed size for client request bodies"; "HTTP response status codes must be accurate"; CGI un-chunking remark.
**HLA ref**: §7.1.
**Depends on**: WEB-022.

### WEB-031 — Request routing — P0

**Intent.** Each parsed request is mapped to exactly one server block, one location, and one handler action.

**Requirements**

- **WEB-031-R1**: The server shall select the server block matching the connection's destination port. [T]
- **WEB-031-R2**: When several server blocks share a port, the server shall use the first declared block. [T]
- **WEB-031-R3**: The server shall match each request URI to the location with the longest matching prefix. [T]
- **WEB-031-R4**: The server shall respond `405 Method Not Allowed` when the method is absent from the matched location's `methods`. [T]
- **WEB-031-R5**: When the matched location contains a `return` directive, the server shall respond with the configured redirect status code and `Location` header. [T]
- **WEB-031-R6**: The server shall dispatch a request to exactly one handler — redirect, CGI, upload, static file, or directory — per the precedence in HLA §7.2. [T]

**Rationale.** Port-based selection and longest-prefix matching implement the subject's route rules and the `/kapouet` root-mapping example.
**Subject trace**: "Specify rules or configurations on a URL/route"; "HTTP redirection"; route root-mapping example.
**HLA ref**: §7.2, §11.4.
**Depends on**: WEB-010, WEB-030.

### WEB-032 — Response generation and error pages — P0

**Intent.** Every response is a well-formed RFC 9112 message with an accurate status code and a body the client can delimit.

**Requirements**

- **WEB-032-R1**: The server shall produce responses conforming to the RFC 9112 message format. [T]
- **WEB-032-R2**: The server shall include a `Date` header in every response. [T]
- **WEB-032-R3**: The server shall include an accurate `Content-Length` in every response with a known body length. [T]
- **WEB-032-R4**: The server shall include a `Connection` header matching the keep-alive decision for the connection. [T]
- **WEB-032-R5**: The server shall include an accurate `Content-Type` header in every response carrying a body. [T]
- **WEB-032-R6**: When a custom `error_page` is configured and readable, the server shall serve it with the corresponding status code. [T]
- **WEB-032-R7**: When no readable custom error page exists, the server shall generate a default HTML error page carrying the correct status code. [T]
- **WEB-032-R8**: The server shall emit status codes per the trigger table in HLA §12.2 (200, 201, 204, 301, 302, 400, 403, 404, 405, 408, 413, 431, 500, 502, 504). [T]

**Rationale.** Subject demands accurate status codes and default error pages; header completeness is required for browser compatibility.
**Subject trace**: "default error pages if none are provided"; "HTTP response status codes must be accurate"; browser compatibility.
**HLA ref**: §7.3, §12.1, §12.2.
**Depends on**: WEB-031.

---

## Epic 4 — Method Handlers

### WEB-040 — Static content serving (GET) — P0

**Intent.** The server will deliver a complete static website from each location's `root`.

**Requirements**

- **WEB-040-R1**: The server shall resolve a request URI to a filesystem path beneath the matched location's `root`. [T]
- **WEB-040-R2**: The server shall respond `403 Forbidden` to a URI whose resolved path escapes the location's `root`. [T]
- **WEB-040-R3**: The server shall respond `404 Not Found` when the resolved path does not exist. [T]
- **WEB-040-R4**: The server shall respond `403 Forbidden` when the resolved file is not readable. [T]
- **WEB-040-R5**: The server shall determine `Content-Type` from the file extension using the mapping in HLA §8.3, defaulting to `application/octet-stream`. [T]
- **WEB-040-R6**: For a directory URI with an existing `index` file, the server shall serve that file. [T]
- **WEB-040-R7**: For a directory URI without an `index` file and `autoindex on`, the server shall generate an HTML listing of the directory. [T]
- **WEB-040-R8**: For a directory URI without an `index` file and `autoindex off`, the server shall respond `403 Forbidden`. [T]
- **WEB-040-R9**: The server shall serve a multi-page static website, including CSS and image assets, that renders correctly in a standard web browser. [D]

**Rationale.** Subject: "serve a fully static website", plus the directory-listing and default-file configuration capabilities; containment checks prevent path traversal (HLA §8.1).
**Subject trace**: "serve a fully static website"; "Directory where the requested file should be located"; "Enabling or disabling directory listing"; "Default file to serve when the requested resource is a directory".
**HLA ref**: §8.
**Depends on**: WEB-032.

### WEB-041 — File upload (POST) — P0

**Intent.** Clients will upload files into the location's configured `upload_path`.

**Requirements**

- **WEB-041-R1**: The server shall accept `POST` uploads to locations with a configured `upload_path`. [T]
- **WEB-041-R2**: The server shall parse `multipart/form-data` bodies using the boundary delimiter (RFC 2046). [T]
- **WEB-041-R3**: The server shall store each uploaded file part in the location's `upload_path`. [T]
- **WEB-041-R4**: The server shall strip all path components from client-supplied filenames before storage. [T]
- **WEB-041-R5**: The server shall store each upload under a unique name so no existing file is overwritten. [T]
- **WEB-041-R6**: The server shall respond `201 Created` after a successful upload. [T]
- **WEB-041-R7**: The server shall respond `500 Internal Server Error` when the upload cannot be written. [T]
- **WEB-041-R8**: A stored upload shall be byte-identical to the file content the client sent. [T]

**Rationale.** Subject: "Clients must be able to upload files" with a provided storage location; R4/R5 prevent traversal and clobbering (HLA §10.3); size enforcement is covered by WEB-030-R8.
**Subject trace**: "Clients must be able to upload files"; "Uploading files from the clients to the server is authorized, and storage location is provided".
**HLA ref**: §10.
**Depends on**: WEB-032.

### WEB-042 — Resource deletion (DELETE) — P0

**Intent.** Clients will remove resources identified by URI within a location's `root`.

**Requirements**

- **WEB-042-R1**: The server shall delete the file identified by the request URI when it resolves beneath the matched location's `root`. [T]
- **WEB-042-R2**: The server shall respond `200 OK` or `204 No Content` after a successful deletion. [T]
- **WEB-042-R3**: The server shall respond `404 Not Found` when the target does not exist. [T]
- **WEB-042-R4**: The server shall respond `403 Forbidden` when deletion is not permitted, including targets outside the `root`. [T]

**Rationale.** Subject mandates the DELETE method; containment mirrors WEB-040-R2 so traversal handling stays consistent.
**Subject trace**: "You need at least the GET, POST, and DELETE methods".
**HLA ref**: §7.2, §12.2.
**Depends on**: WEB-032.

---

## Epic 5 — CGI

### WEB-050 — CGI execution and environment — P0

**Intent.** Requests for configured script extensions will be answered by executing the configured interpreter per RFC 3875.

**Requirements**

- **WEB-050-R1**: The server shall execute a CGI when the request URI resolves to a file whose extension matches a configured `cgi_extension`. [T]
- **WEB-050-R2**: The server shall invoke the interpreter configured as the location's `cgi_path`. [T]
- **WEB-050-R3**: The server shall execute the CGI with the script's directory as the child process working directory. [T]
- **WEB-050-R4**: The server shall provide the CGI with the RFC 3875 meta-variables `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `PATH_INFO`, `PATH_TRANSLATED`, `SCRIPT_NAME`, `SERVER_NAME`, `SERVER_PORT`, `SERVER_PROTOCOL`, `SERVER_SOFTWARE`, `REMOTE_ADDR`, and `GATEWAY_INTERFACE`. [T]
- **WEB-050-R5**: The server shall provide every request header to the CGI as an `HTTP_*` meta-variable. [T]
- **WEB-050-R6**: The server shall create child processes exclusively for CGI execution. [A]
- **WEB-050-R7**: The server shall support at least one CGI interpreter, such as `php-cgi` or `python3`. [D]
- **WEB-050-R8**: The server shall not execute another web server. [I]

**Rationale.** Subject CGI bullets; R4/R5 implement "the full request and arguments provided by the client must be available to the CGI"; R3 implements "run in the correct directory for relative path file access".
**Subject trace**: "Execution of CGI, based on file extension"; "environment variables involved in the web server-CGI communication"; "run in the correct directory"; "can't use fork for anything other than CGI"; "You can't execve another web server"; "support at least one CGI".
**HLA ref**: §9.1–9.3.
**Depends on**: WEB-032.

### WEB-051 — CGI input/output streaming — P0

**Intent.** All CGI pipe traffic flows through the same multiplexer under the same non-blocking rules as client sockets.

**Requirements**

- **WEB-051-R1**: The server shall set every CGI pipe descriptor to non-blocking mode and register it with the multiplexer. [A]
- **WEB-051-R2**: The server shall perform CGI pipe reads and writes only after multiplexer readiness. [A]
- **WEB-051-R3**: The server shall deliver the complete, de-chunked request body to the CGI's standard input. [T]
- **WEB-051-R4**: The server shall signal end-of-body to the CGI by closing its standard-input stream. [T]
- **WEB-051-R5**: When CGI output carries no `Content-Length`, the server shall treat end-of-file on the CGI's output stream as the end of the response body. [T]
- **WEB-051-R6**: The server shall parse CGI response headers and support the document, local-redirect, and client-redirect response forms (RFC 3875 §6). [T]
- **WEB-051-R7**: The server shall respond `502 Bad Gateway` when CGI output is invalid. [T]
- **WEB-051-R8**: Upon CGI completion, the server shall close all pipe descriptors and remove their multiplexer registrations. [A]

**Rationale.** Subject remarks on chunked un-chunking and EOF semantics; the single-multiplexer constraint (WEB-020) extends to pipes; R8 prevents descriptor leaks.
**Subject trace**: "your server needs to un-chunk them, the CGI will expect EOF as the end of the body"; "If no content_length is returned from the CGI, EOF will mark the end of the returned data".
**HLA ref**: §5.2, §9.3, §9.4.
**Depends on**: WEB-050.

### WEB-052 — CGI timeout and child reaping — P1

**Intent.** CGI execution time is bounded and no child process is left unreaped.

**Requirements**

- **WEB-052-R1**: The server shall terminate a CGI process that exceeds the configured `cgi_timeout` (default: 30 s). [T]
- **WEB-052-R2**: The server shall respond `504 Gateway Timeout` to the client of a terminated CGI. [T]
- **WEB-052-R3**: The server shall reap every terminated child process, leaving zero zombie processes. [T]

**Rationale.** Subject: no request hangs indefinitely; zombies exhaust the process table under load (HLA §12.3).
**Subject trace**: "A request to your server should never hang indefinitely".
**HLA ref**: §9.3 (step 5), §12.3, §14.1.
**Depends on**: WEB-051.

---

## Epic 6 — Robustness & Operations

### WEB-060 — Signal handling and graceful shutdown — P1

**Intent.** External signals produce controlled behavior: shutdown on request, survival on broken pipes.

**Requirements**

- **WEB-060-R1**: Upon `SIGINT` or `SIGTERM`, the server shall stop accepting new connections, terminate running CGI processes, release all resources, and exit with status 0. [D]
- **WEB-060-R2**: The termination of any client connection, including mid-response, shall not cause the server process to terminate. [T]

**Rationale.** Subject: "properly handle client disconnections" and availability under stress; shutdown sequence per HLA §14.2.
**Subject trace**: "properly handle client disconnections when necessary"; "remains available at all times".
**HLA ref**: §14.
**Depends on**: WEB-022.

### WEB-061 — Logging — P2

**Intent.** The server will record request, error, and diagnostic activity to configurable destinations.

**Requirements**

- **WEB-061-R1**: The server shall record every completed request in the access log using the Combined Log Format (HLA §13.2). [T]
- **WEB-061-R2**: The server shall record errors with a timestamp, severity level, and component identifier (HLA §13.3). [I]
- **WEB-061-R3**: Log destinations shall be configurable per server block. [T]
- **WEB-061-R4**: When a configured log file cannot be opened, the server shall fall back to the standard stream defined in HLA §13.5. [T]
- **WEB-061-R5**: The server shall emit debug output only when a `debug_log` directive is configured. [I]

**Rationale.** Observability is an HLA design principle (§3.2) and supports debugging during evaluation; not subject-mandatory, hence P2.
**HLA ref**: §13.
**Depends on**: WEB-010, WEB-032.

### WEB-062 — Availability under stress — P1

**Intent.** The server will remain available under sustained load and hostile input.

**Requirements**

- **WEB-062-R1**: The server shall sustain ≥ 100 concurrent connections for ≥ 60 s (*TBR-2*) with zero crashes and zero hangs. [T]
- **WEB-062-R2**: The server shall reject malformed, oversized, and indefinitely slow requests with the applicable error response while continuing to serve concurrent well-formed requests. [T]
- **WEB-062-R3**: The server's open-descriptor count and resident memory shall return to their pre-test baseline after the load ends. [T]

**Rationale.** Subject: "Stress test your server to ensure it remains available at all times"; R3 detects descriptor and memory leaks that would degrade availability over time.
**Subject trace**: "Stress test your server to ensure it remains available at all times"; "A request to your server should never hang indefinitely".
**HLA ref**: §12.3.
**Depends on**: WEB-023, WEB-042, WEB-052.

### WEB-063 — Browser and NGINX compatibility — P2

**Intent.** Observable HTTP behavior will match what standard browsers and NGINX-led comparison expect.

**Requirements**

- **WEB-063-R1**: The server shall serve a fully static website end-to-end in at least one standard web browser, with all page assets loading correctly. [D]
- **WEB-063-R2**: The server's headers and status-code behavior shall be compared against NGINX for every scenario in HLA §12.2, with any deviation documented and justified. [A]

**Rationale.** Subject: browser compatibility and NGINX as behavioral reference (noting HTTP-version differences).
**Subject trace**: "compatible with standard web browsers of your choice"; "NGINX may be used to compare headers and answer behaviours".
**Depends on**: WEB-040.

---

## Epic 7 — Evaluation Assets

### WEB-070 — Demonstration configuration and content — P2

**Intent.** The repository will ship everything needed to demonstrate every feature during evaluation.

**Requirements**

- **WEB-070-R1**: The project shall provide configuration files exercising multiple ports, multiple server blocks, and every location directive (methods, redirect, root, autoindex on and off, index, upload path, body-size limit, CGI extension). [I]
- **WEB-070-R2**: The project shall provide demonstration content: a static website, custom error pages, an upload form, at least one CGI script, and deletable files. [D]
- **WEB-070-R3**: Every mandatory feature shall be demonstrable using only the provided configuration files and content. [D]

**Rationale.** Subject explicitly requires configuration files and default files for evaluation demonstration.
**Subject trace**: "You must provide configuration files and default files to test and demonstrate that every feature works during the evaluation."
**Depends on**: all P0/P1 tickets.

---

## Epic 8 — Bonus (goals)

### WEB-080 — Cookies and session management — P3

**Intent.** As a goal beyond the mandatory scope, the server will support cookie-based sessions.

**Requirements**

- **WEB-080-R1**: The server should issue session cookies via `Set-Cookie` and recognize returning sessions via the `Cookie` header. [D]
- **WEB-080-R2**: The server should maintain session state across requests and demonstrate it with a working example page. [D]

**Subject trace**: "Support cookies and session management (provide simple examples)."

### WEB-081 — Second CGI type — P3

**Intent.** As a goal beyond the mandatory single CGI, two interpreter types will run side by side.

**Requirements**

- **WEB-081-R1**: The server should execute two distinct CGI types (e.g. PHP via `php-cgi` and Python via `python3`) selected by file extension within one configuration. [D]

**Subject trace**: "Handle 2 (easiest to implement) CGI types."

---

## Suggested Execution Order

1. **Foundation**: WEB-001 → WEB-002
2. **Config**: WEB-010
3. **Core**: WEB-020 → WEB-021 → WEB-022 → WEB-023
4. **HTTP**: WEB-030 → WEB-031 → WEB-032
5. **Handlers**: WEB-040 → WEB-041 → WEB-042
6. **CGI**: WEB-050 → WEB-051 → WEB-052
7. **Ops**: WEB-060, WEB-061 (parallelizable with 5–6)
8. **Hardening**: WEB-062, WEB-063
9. **Evaluation**: WEB-070
10. **Bonus** (only after all mandatory requirements verified): WEB-080, WEB-081

## Definition of Done (all tickets)

- The server compiles under the WEB-001 flags.
- Every P0/P1 requirement passes its stated verification method ([I]/[A]/[D]/[T]).
- All constraints C1–C8 (HLA §3.1) hold under code audit.
- All features are demonstrable via the WEB-070 assets.
- All TBRs in the register are resolved or explicitly accepted.
