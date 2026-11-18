# Event Loop and Configuration Review

**Project:** webserv  
**Language target:** C++98  
**Scope:** Event loop, configuration parsing/validation, their integration, and directly related tests/documentation  
**Review type:** Static analysis plus targeted compile and parser probes; no production source files were changed

## 1. Review basis and confirmed decisions

The following project decisions were confirmed before this report was written:

1. **The current parser syntax is authoritative.** Retain separate `listen PORT;` and `host IPv4;` directives, `client_max_body_size`, `error_pages`, the outer `http` block, and the existing location/CGI style. `ARCHITECTURE.md` §11 is not authoritative where it conflicts with these choices.
2. **A full stateful Linux/epoll rewrite is required.** The target includes multiple listeners, per-client state, buffered reads and writes, timeouts, and graceful shutdown.
3. **The 40-file custom error-page directory is mandatory.** The existing status-code set is retained, but the parsed paths must actually be stored and made available to the response layer.
4. **Multiple endpoints are represented by multiple server blocks.** The current semantic code permits one `listen` and one `host` per server block; the event loop must support all resulting server blocks.

For planning, the recommended default configuration path is `./default.conf`, with an explicit command-line path taking precedence. This is a proposed integration detail, not an existing implementation fact.

## 2. Materials reviewed

- Requirements: `trimmed_webserv.json`, especially pp. 9–12; `requisites.md`.
- Architecture and design: `ARCHITECTURE.md`, `configuration.md`, `dataStructures.md`, `responses.md`.
- Configuration implementation: `configuration.cpp`, `configuration.hpp`, `server.cpp`, `server.hpp`, `utils.cpp`, `utils.hpp`.
- Event-loop implementation: `event_loop.cpp`, `event_loop.hpp`, `signal.cpp`, `signal.hpp`.
- Supplied configurations: `default.conf`, `empty.conf`, `multiple_servers_nested.conf`, `test.conf`, `test2.conf`.
- `response.cpp` and `response.hpp` were inspected only to understand integration boundaries; response-generation details outside the requested scope are not reported.

No `main.cpp`, Makefile, HTTP parser implementation, or automated event-loop/configuration test suite was supplied. Consequently, full-program linking and end-to-end runtime verification were not possible. Individual translation units were checked with `-std=c++98 -Wall -Wextra -Werror`. Diagnostic parser probes used C++11 only to get past the existing C++11 enum extension; this does not imply that the current code satisfies the C++98 requirement.

## 3. Executive summary

The current implementation is a prototype rather than an integrated server:

- It does not compile as C++98 with the required warning flags.
- Every supplied non-empty sample configuration is rejected by the current parser.
- The event loop ignores the configuration and command-line path, binds one hard-coded endpoint, and has no per-client protocol state.
- Socket responses are sent after read readiness without `EPOLLOUT`, contrary to the central I/O requirement in `trimmed_webserv.json` p. 9.
- Non-blocking partial writes, fragmented requests, idle clients, disconnected peers, and most runtime failures are not handled safely.
- The configuration model lacks mandatory route features and discards both location configuration and the validated error-page mapping.

The remediation should therefore be treated as two coordinated workstreams, not as isolated patching: **Developer A owns configuration and its typed semantic model; Developer B owns the stateful event loop and runtime integration.** The contract between them must be fixed first.

## 4. Findings

Severity meanings:

- **Critical:** Prevents required compilation or core operation, violates a central subject constraint, or can terminate the server.
- **High:** Produces incorrect parsing/runtime behavior or leaves a mandatory feature unusable.
- **Medium:** Robustness, resource-management, portability, or diagnosability defect.
- **Low:** Localized implementation/documentation weakness with limited direct runtime effect.

### 4.1 Cross-cutting and configuration findings

#### F-01 — Critical — Required C++98 compilation fails

**Why flagged:** `configuration.hpp:57` gives `e_directive_type` an explicit underlying type (`enum e_directive_type : size_t`), which is a C++11 feature. The compiler rejects it under `-std=c++98 -Werror`. After that blocker is bypassed diagnostically, `event_loop.cpp:105` also produces `-Werror=unused-parameter` errors for `argc` and `argv` because their only uses are commented out. This prevents the required build before runtime behavior can be assessed.

**References:** `configuration.hpp:57`; `event_loop.cpp:105-128`.

#### F-02 — High — All supplied non-empty sample configurations are rejected

**Why flagged:** The documentation, samples, and parser describe different grammars. `server_name` appears in samples but is absent from the directive table; `test.conf` and `test2.conf` start with `server` although the parser requires `http`; `test2.conf` also nests a second `server` inside the first. A probe of the current parser rejected `default.conf`, `multiple_servers_nested.conf`, `test.conf`, and `test2.conf`; `empty.conf` was also rejected. Thus the project currently supplies no positive example of its authoritative syntax.

**References:** `configuration.cpp:26-36`, `configuration.cpp:334-354`; `default.conf:3-20`, especially line 8; `multiple_servers_nested.conf:3-34`, especially lines 8, 19, and 22; `test.conf:17-30`; `test2.conf:17-59`; `configuration.md:30-38`; conflicting syntax in `ARCHITECTURE.md:691-760`.

#### F-03 — High — Trailing non-configuration text can be silently accepted

**Why flagged:** After parsing the root block, `stt_parseConfig` calls `stt_consumeComments` without first establishing that the current character is `#`. That helper consumes through the end of the line regardless of content. A probe accepted a file containing `http { listen 8000; }` followed by `this is not a directive`. Invalid trailing content can therefore be hidden instead of producing a startup error.

**References:** `configuration.cpp:79-88`, `configuration.cpp:356-367`.

#### F-04 — High — Incomplete or server-less configurations can be returned as valid parse results

**Why flagged:** If EOF is reached while skipping whitespace after the token `http`, lines 351–352 return a default-constructed block instead of reporting the missing `{`. A probe confirmed that the input `http ` is accepted with an empty root name. `http {}` is also syntactically accepted, and there is no subsequent startup validation requiring at least one usable server block. These cases can flow into runtime with no listeners or an undefined configuration shape.

**References:** `configuration.cpp:334-354`; `configuration.cpp:378-393`; absence of server-list validation in `server.cpp:228-262`.

#### F-05 — Medium — Block parsing consumes lookahead inconsistently

**Why flagged:** When a closing brace is handled inside the block loop, the parser advances once at lines 265–267 and again at line 312. Other closing-brace paths advance only once. This makes tokenization depend on formatting. A probe accepted adjacent braces in one layout but rejected the otherwise token-delimited sequence `}server` between sibling server blocks. A parser should have exactly one owner of lookahead advancement, independent of whitespace layout.

**References:** `configuration.cpp:245-315`, especially `configuration.cpp:260-269` and `configuration.cpp:312`.

#### F-06 — High — Directive context is not validated

**Why flagged:** The parser checks only whether a directive name exists in one global table and whether it is simple or block. It does not check whether that directive is legal in the current `http`, `server`, `location`, or `cgi` context. Probes confirmed acceptance of `listen` directly inside `http` and acceptance of nested `server` blocks. This creates trees that the semantic layer cannot interpret consistently and contradicts the documented context hierarchy.

**References:** `configuration.cpp:14-37`, `configuration.cpp:277-309`; `configuration.md:30-38`; nested example in `test2.conf:41-58`.

#### F-07 — High — A location with no URI is accepted

**Why flagged:** For `location`, the parser calls `stt_parseToken` for the argument but never checks that the resulting string is non-empty or unique. If the next character is `{`, tokenization returns an empty name and the block is accepted. Probes confirmed acceptance of both `location { root /tmp; }` and `location { }`. An empty route cannot be matched safely and makes configuration errors difficult to diagnose.

**References:** `configuration.cpp:160-173`, `configuration.cpp:282-299`; route argument storage in `configuration.hpp:88-94`.

#### F-08 — Critical — Mandatory route configuration capabilities are absent

**Why flagged:** The directive table contains only `listen`, `host`, `client_max_body_size`, `error_pages`, `root`, one Rust-specific CGI extension, and block names. There is no way to configure accepted methods, HTTP redirection, directory listing, an index file, or an upload destination. These are explicit configuration requirements in `trimmed_webserv.json` pp. 11–12 and `requisites.md:93-102`. This is a missing feature, not merely a validation weakness.

**References:** `configuration.cpp:24-37`; `configuration.hpp:57-76`; `configuration.md:41-72`; requirements in `requisites.md:81-103`.

#### F-09 — High — Parsed location blocks are discarded by the semantic model

**Why flagged:** `fetch_config` returns a generic block tree, while the `Server` constructor processes only four simple directives. Its location-processing code is commented out. `Config::location` is a `std::map<long, std::string>`, which cannot represent a URI-keyed route with methods, root, redirects, autoindex, index, uploads, and CGI settings. Even a syntactically valid location therefore has no usable runtime representation.

**References:** `configuration.hpp:88-109`; `server.cpp:228-262`, especially `server.cpp:254-261`; `server.hpp:28-36`.

#### F-10 — High — A valid 40-file error-page directory still produces an empty map

**Why flagged:** `stt_errorPagesGet` recognizes and counts status-named files, but the insertion into `errorPagesResponses` is commented out. The function consequently returns an empty map even after all validation succeeds. A probe using exactly the required 40 files produced `errorPages=0`. The confirmed mandatory-directory policy is therefore implemented only as a gate, not as usable configuration.

**References:** `server.cpp:166-225`, especially `server.cpp:181-184` and `server.cpp:211-224`; `server.hpp:33`.

#### F-11 — Medium — Error-page validation is neither exception-safe nor sufficient for later use

**Why flagged:** If the recognized file count is not exactly 40, the function throws before `closedir`, leaking the directory stream. During scanning it checks only selected filenames; it does not retain each path or establish that the target is a readable regular file. A directory entry named `404.html` can therefore satisfy the count without providing a usable page. Cleanup and validation need to cover every exit path.

**References:** `server.cpp:176-224`, especially the throw at `server.cpp:219-220` before `closedir` at line 222.

#### F-12 — High — Port conversion can accept overflow and wrap it into an enormous unsigned value

**Why flagged:** On `strtol` overflow, `stt_portGet` returns `-1` rather than reporting a configuration error. `Server` assigns that result to `Config::port`, which is `size_t`; a probe accepted an overflowing token and produced port `18446744073709551615` on the review platform. The no-digits check also compares `end` with `digits.c_str()`, where `digits` is an unrelated empty string, instead of comparing with the start of the input token. The trailing-character check happens to catch many malformed inputs, but it does not make the conversion logic correct.

**References:** `server.cpp:48-79`, especially `server.cpp:59-75`; assignment at `server.cpp:238-240`; unsigned destination at `server.hpp:30`.

#### F-13 — High — The configured host is not validated

**Why flagged:** `stt_ipGet` checks only the argument count and then returns the string unchanged. Empty-looking malformed addresses, non-numeric names, and invalid IPv4 values are not rejected at configuration load. The event loop separately ignores this value and binds `INADDR_ANY`, so there is currently no later validation stage that could compensate.

**References:** `server.cpp:81-94`, `server.cpp:242-244`; `event_loop.cpp:67-71`.

#### F-14 — Medium — Body-size parsing uses undocumented length limits instead of checked arithmetic

**Why flagged:** `stt_clientMaxBodySizeGet` stores the converted value in `size_t`, applies `K`, `M`, or `G` multiplication, and uses token-length thresholds such as 14, 10, and 6 as overflow guards. These thresholds are platform-dependent, undocumented, and not a substitute for checking `value > max / multiplier`. On platforms with a narrower `size_t`, a multiplication can wrap even when `strtol` itself succeeds.

**References:** `server.cpp:96-157`; destination type at `server.hpp:32`.

#### F-15 — Medium — Semantic validation is rigidly coupled to exactly four directive kinds

**Why flagged:** The `Server` constructor rejects `dSimple.size() != 4`. This does not merely enforce the four confirmed mandatory fields: it also rejects any additional simple directive accepted by the parser, including otherwise legal future extensions. Required validation should check each directive explicitly, its cardinality, and its context rather than using the total map size as a proxy.

**References:** `server.cpp:228-252`, especially `server.cpp:231-236`; current parser acceptance of generic known directives at `configuration.cpp:300-307`.

#### F-16 — Low — Configuration errors lack actionable source locations

**Why flagged:** The parser throws integer codes and `errorPrint` emits broad messages such as “Bad block directive structure!” without a file, line, column, directive, or expected token. This is why several different defects in the supplied samples collapse into the same error. Typed diagnostics with source positions are necessary for maintainable parser tests and for users to correct configuration files.

**References:** representative throws at `configuration.cpp:250-280`, `configuration.cpp:339-366`, and `configuration.cpp:382-388`; messages in `utils.cpp:9-69`; error API in `utils.hpp:39`.

#### F-17 — Medium — No automated configuration or event-loop regression tests were supplied

**Why flagged:** The only configuration artifacts are samples, and all non-empty samples currently fail parsing. There are no tests for fragmented socket input, partial writes, multiple listeners, timeouts, signals, malformed configuration, numeric overflow, or error-page loading. These are precisely the areas where the confirmed defects occurred. `trimmed_webserv.json` p. 12 also calls for tests beyond a single manual client.

**References:** supplied file inventory; test expectations in `requisites.md:130-136`; sample files listed in F-02.

### 4.2 Event-loop and runtime-integration findings

#### F-18 — Critical — Configuration and command-line input are disconnected from the event loop

**Why flagged:** The argument checks and `fetch_config` call are commented out. The loop hard-codes port 8000, creates one listener, and `createListener` always binds `INADDR_ANY`. There is no default-path behavior and no way to serve the multiple interface/port pairs represented by configuration. The single `listener_fd` comparison also cannot distinguish multiple listeners.

**References:** `event_loop.cpp:42-93`, especially `event_loop.cpp:67-71`; `event_loop.cpp:105-154`; `event_loop.cpp:165-170`; requirements in `requisites.md:8-11` and `requisites.md:74-87`.

#### F-19 — Critical — Socket writes occur without write-readiness monitoring

**Why flagged:** Clients are registered only for `EPOLLIN`, yet receiving a request immediately calls `respond`, which calls `send_file` and issues `send` for headers and body. No `EPOLLOUT` event is requested before these writes. This directly violates the requirement that every socket read/write follow readiness from the single multiplexer, described in `trimmed_webserv.json` p. 9 and `requisites.md:23-38`.

**References:** client registration at `event_loop.cpp:191-201`; immediate response path at `event_loop.cpp:213-223`; writes at `event_loop.cpp:255-299`, especially lines 271 and 285–287.

#### F-20 — Critical — There is no per-client request or connection state

**Why flagged:** Each readiness event creates a fresh local 4096-byte array, performs one `recv`, interprets the bytes immediately through substring checks, sends, and closes. Partial request lines, split headers, bodies spanning reads, pipelined requests, and persistent connections cannot be represented. A TCP read event is not a complete HTTP message boundary.

**References:** `event_loop.cpp:206-232`; request-dependent response selection and unconditional close at `event_loop.cpp:307-327`; contrasting required state model in `ARCHITECTURE.md:328-410`.

#### F-21 — High — Edge-triggered clients are not drained

**Why flagged:** Accepted clients are registered with `EPOLLET`, but only one `recv` is performed per delivered event. If more bytes remain in the socket receive queue, edge-triggered operation does not guarantee another event merely because data remains. Oversized or coalesced input can therefore stall indefinitely. The rewrite can avoid this defect by using level-triggered interests initially; retaining `EPOLLET` requires a complete drain strategy without post-read `errno` checks.

**References:** `event_loop.cpp:191`; single read at `event_loop.cpp:208-224`.

#### F-22 — Critical — Runtime failures can terminate the entire process

**Why flagged:** `createListener` calls `exit(1)` on socket-option, bind, and listen failures; client `epoll_ctl` failure also exits; any `recv == -1` closes the listener and epoll descriptor and exits. A per-client readiness or registration error must not take down unrelated connections. Startup errors may reject the configuration and exit cleanly, but runtime errors require connection-local handling and centralized cleanup.

**References:** `event_loop.cpp:49-88`; `event_loop.cpp:184-201`; `event_loop.cpp:224-230`; resilience requirements in `trimmed_webserv.json` pp. 4 and 12.

#### F-23 — High — An accept failure breaks the main event loop

**Why flagged:** When `accept` fails, the code logs non-would-block errors and then executes `break`. Because the accept-retry loop is commented out, this `break` exits the main `while (!interrupt)` loop, stopping the server rather than ending only an accept batch. Temporary listener/resource errors therefore become loss of service.

**References:** `event_loop.cpp:169-183`; commented accept loop at `event_loop.cpp:171-175` and `event_loop.cpp:204`.

#### F-24 — High — Non-blocking partial writes and backpressure are not handled

**Why flagged:** The header `send` return value is ignored, so a short header write is never resumed. Body writes use a loop, but on `send < 0` the function abandons the response; no output buffer or offset survives for a later writable event. A client that does not read quickly can therefore receive a truncated response, and the current structure cannot apply bounded output buffering.

**References:** `event_loop.cpp:255-299`, especially `event_loop.cpp:271` and `event_loop.cpp:280-295`; close after response at `event_loop.cpp:324-326`.

#### F-25 — Critical — A disconnected peer can terminate the process through SIGPIPE

**Why flagged:** Active socket writes pass flags `0`, not `MSG_NOSIGNAL`, and the supplied signal setup registers only `SIGINT`. It neither ignores `SIGPIPE` nor establishes another consistent suppression mechanism. Writing to a closed socket can therefore invoke the default SIGPIPE action and kill the server.

**References:** `event_loop.cpp:271`, `event_loop.cpp:285-287`; `signal.cpp:6-20`; contrasting design requirement in `ARCHITECTURE.md:882-907`.

#### F-26 — High — Connections and requests can hang indefinitely

**Why flagged:** `epoll_wait` uses an infinite timeout, and no connection stores timestamps or deadlines. There is no request-read, idle-connection, response-write, or future CGI timeout enforcement. A client that connects and sends nothing, sends an incomplete request slowly, or stops reading can retain resources indefinitely.

**References:** `event_loop.cpp:151-162`; per-client locals without timestamps at `event_loop.cpp:206-232`; required timeout model in `ARCHITECTURE.md:392-401`; no-hang requirement in `trimmed_webserv.json` p. 9.

#### F-27 — Medium — The event mask is ignored

**Why flagged:** The loop uses only `events[i].data.fd`; it never inspects `events[i].events`. `EPOLLERR`, `EPOLLHUP`, and peer shutdown are consequently handled as though they were ordinary readable events, and clients are not registered for `EPOLLRDHUP`. This obscures disconnect handling and can turn expected connection lifecycle events into fatal receive errors.

**References:** `event_loop.cpp:165-169`, `event_loop.cpp:206-231`; registration masks at `event_loop.cpp:139-143` and `event_loop.cpp:191-193`.

#### F-28 — High — Descriptor cleanup is incomplete on startup, runtime, and shutdown paths

**Why flagged:** Failure to create epoll leaves the listener open; failure to register the listener leaves both descriptors open; client non-blocking setup failure returns from the loop without global cleanup; normal loop termination returns without closing the listener, epoll descriptor, or active clients. Individual `close` calls are scattered through error branches rather than centralized by ownership.

**References:** `event_loop.cpp:130-149`, `event_loop.cpp:184-200`, `event_loop.cpp:224-230`, and `event_loop.cpp:251-252`; existing cleanup TODO at `event_loop.cpp:39-41`.

#### F-29 — Medium — Signal handling is async-signal-unsafe and incomplete

**Why flagged:** The signal handler writes to `std::cerr`, which is not async-signal-safe. Only `SIGINT` is registered, `sigaction` errors are ignored, and there is no `SIGTERM` handling. The handler should only set a `volatile sig_atomic_t` flag; the event loop should perform logging and deterministic cleanup outside handler context.

**References:** `signal.cpp:6-20`; global flag in `event_loop.cpp:35`; signal wait handling in `event_loop.cpp:152-161`.

#### F-30 — High — Route, body-limit, and error-page configuration have no runtime effect

**Why flagged:** Response selection uses hard-coded HTML/CSS/JavaScript paths and substring searches in the raw request. No parsed location selects a root, no configured body limit is enforced during reads, and no configured error page is selected on failure. Even after F-18 is fixed for endpoints, the remaining confirmed configuration would still be unused.

**References:** hard-coded paths at `event_loop.cpp:22-30`; single-read request handling at `event_loop.cpp:208-223`; hard-coded selection at `event_loop.cpp:307-327`; unused fields in `server.hpp:28-36`.

## 5. Remediation plan — Developer A: configuration and semantic model

Developer A owns `configuration.*`, configuration-related parts of `server.*` and `utils.*`, sample configurations, and configuration tests. The objective is to return a fully validated, typed configuration rather than a loosely interpreted generic tree.

### A1. Freeze and document the authoritative grammar

**Tasks:**

1. Document the retained syntax: one outer `http`; sibling `server` blocks; mandatory `listen`, `host`, `client_max_body_size`, and `error_pages` per server; URI-bearing `location` blocks; existing `root` and CGI style.
2. Keep one `host`/`listen` pair per server and use multiple server blocks for multiple endpoints.
3. Preserve the mandatory 40-status error-page directory policy and enumerate the exact 40 required filenames.
4. Add the missing required route directives. Recommended names are `methods`, `return`, `autoindex`, `index`, and `upload_path`; retain the existing `root` and CGI spelling unless the team explicitly changes the grammar.
5. Define cardinality, duplicates, allowed contexts, defaults if any, size suffixes, and port range. Retain and document the current project port range **1025–65535** unless the team deliberately changes that separate policy.
6. Rewrite all sample configurations to this grammar and update `configuration.md`. Mark the conflicting part of `ARCHITECTURE.md` §11 as superseded so there is one contract.

**Files:** `configuration.md`, `ARCHITECTURE.md`, all `*.conf`, `configuration.hpp`.  
**Addresses:** F-02, F-06, F-08, F-15.  
**Done when:** A concise grammar can parse every revised valid sample on paper, and every directive has an explicit context, arity, and duplicate rule.

### A2. Replace character-fragile parsing with deterministic tokenization and recursive descent

**Tasks:**

1. Introduce a C++98 lexer that emits directive tokens plus `{`, `}`, `;`, comment/whitespace skipping, EOF, and source line/column.
2. Give exactly one component ownership of token advancement; remove the implicit character advancement scattered across helpers.
3. Require the outer `http` block and at least one server; reject missing braces, empty locations, trailing tokens, malformed simple directives, and unexpected EOF.
4. Validate directive context during parsing: server only in `http`, location only in `server`, CGI only in its documented context, and simple directives only in approved contexts.
5. Validate token count and non-empty arguments at the point of parsing. Reject duplicate singleton directives and duplicate location paths according to A1.
6. Return source-aware errors containing file, line, column, directive, and expected token. Replace integer-only throws with a consistent C++98 exception/error type.

**Files:** `configuration.cpp`, `configuration.hpp`, `utils.cpp`, `utils.hpp`.  
**Addresses:** F-03, F-04, F-05, F-06, F-07, F-16.  
**Done when:** The probes for trailing garbage, `http `, nested servers, empty locations, and `}server` produce deliberate, tested outcomes with stable diagnostics.

### A3. Define typed configuration output and preserve all parsed data

**Tasks:**

1. Introduce C++98 structs such as `Config`, `ServerConfig`, `LocationConfig`, and `CgiConfig`; use `std::vector` for ordered server/location collections and `std::map` only for keyed lookups.
2. Replace `Config::location` (`std::map<long, std::string>`) with a representation keyed by URI or an ordered vector suitable for longest-prefix matching.
3. Return all server blocks from configuration loading. Keep declaration order so endpoint/default-server behavior is deterministic.
4. Populate every route field required by A1 and apply only documented defaults/inheritance.
5. Remove the `dSimple.size() == 4` shortcut; validate mandatory fields individually.
6. Agree on the typed API with Developer B before either runtime integration begins.

**Files:** `configuration.hpp`, `configuration.cpp`, `server.hpp`, `server.cpp`, `ARCHITECTURE.md` data-flow notes.  
**Addresses:** F-08, F-09, F-15, F-30.  
**Done when:** A valid two-server configuration returns two complete `ServerConfig` objects, and every location and CGI block survives conversion unchanged except for documented defaults.

### A4. Make scalar and endpoint validation correct

**Tasks:**

1. Parse ports with the correct input-start pointer, reject empty/trailing characters and `ERANGE`, and throw on every invalid value; never return `-1` into an unsigned field. Use a fixed-width or explicitly sized port type appropriate for 0–65535.
2. Validate `host` as the project’s numeric IPv4 form and retain a resolved bind representation for Developer B. Resolution failure is a configuration/startup error with the endpoint named in the diagnostic.
3. Parse body sizes into a sufficiently wide unsigned intermediate, validate the single optional suffix, and check overflow before multiplication using `std::numeric_limits`.
4. Define and test the meaning of zero and the maximum accepted body size.

**Files:** `server.cpp`, `server.hpp`, `utils.cpp`, `utils.hpp`.  
**Addresses:** F-12, F-13, F-14.  
**Done when:** Non-numeric, negative, overflowing, and out-of-range values are rejected; the previous overflowing-port probe cannot produce a wrapped port.

### A5. Load the mandatory error-page directory completely and safely

**Tasks:**

1. Use an RAII wrapper or a single cleanup path for `DIR *`; close it on success and every failure.
2. Require exactly the confirmed 40 status pages, rejecting missing or duplicate effective mappings.
3. Validate each entry as a readable regular file and store `status code -> full path` in `ServerConfig`.
4. Do not build HTTP responses in the configuration loader; provide validated paths to the response component.
5. Test missing directories, permission failures, non-regular entries, missing codes, and the valid 40-file case.

**Files:** `server.cpp`, `server.hpp`, configuration tests.  
**Addresses:** F-10, F-11.  
**Done when:** The valid-directory probe returns 40 usable mappings, and failure tests show no directory-descriptor leak.

### A6. Add a configuration regression suite

**Tasks:**

1. Add table-driven valid/invalid parser cases for whitespace, comments, adjacent delimiters, EOF at every production, unknown directives, wrong contexts, duplicates, missing location URIs, and trailing garbage.
2. Add semantic cases for every scalar boundary, all required route directives, multiple servers, duplicate endpoints, and the 40 error pages.
3. Make each supplied `.conf` either a passing positive fixture or an intentionally failing negative fixture with an asserted diagnostic; remove ambiguous examples.
4. Compile the suite and production configuration units with `c++ -std=c++98 -Wall -Wextra -Werror`.

**Files:** new configuration test driver/scripts and fixtures; `configuration.*`, `server.*`, `utils.*` as needed.  
**Addresses:** F-01 through F-17 as applicable.  
**Done when:** Valid fixtures pass, invalid fixtures fail for the stated reason, and the suite runs without crashes or resource leaks.

## 6. Remediation plan — Developer B: event loop and runtime

Developer B owns `event_loop.*`, `signal.*`, runtime descriptor/connection types, and socket integration tests. Developer B should build against a temporary typed-config stub until A3 is ready, then replace the stub without changing connection logic.

### B1. Establish startup and ownership boundaries

**Tasks:**

1. Replace the unused-argument interface with an explicit startup flow: command-line path or proposed `./default.conf`, parse/validate, reject invalid startup configuration cleanly, then construct the runtime.
2. Create small C++98 RAII owners for file descriptors, the epoll instance, listener state, and connection state. Centralize `closeConnection` and shutdown cleanup.
3. Remove `exit()` from runtime handlers. Startup failure returns a nonzero status after cleanup; a client failure closes only that client; a listener failure follows a deliberate logged policy without scattering process termination through the loop.
4. Remove the C++11 enum dependency from the shared configuration header in coordination with A2 and make `event_loop.cpp` warning-clean.

**Files:** `event_loop.cpp`, `event_loop.hpp`, new runtime state header/source if desired, `signal.cpp`; startup caller when supplied.  
**Addresses:** F-01, F-18, F-22, F-28.  
**Done when:** Every early return and runtime error has one defined owner for every descriptor, and no client-originated failure terminates the process.

### B2. Create and manage all configured listeners through one epoll instance

**Tasks:**

1. For each unique configured `host`/`port`, resolve the numeric address, create a non-blocking socket, set `SO_REUSEADDR`, bind, listen, and register it with the single epoll descriptor.
2. Detect duplicate endpoints during startup and report both the endpoint and applicable configuration context.
3. Maintain `listener fd -> ServerConfig` and `client fd -> Connection` mappings so events dispatch by role rather than comparison with one global descriptor.
4. On listener readiness, accept pending connections according to the selected trigger mode, make each client non-blocking, allocate its state, and register it. Accept failure must end only the current accept batch or follow an explicit listener-error policy, never break the main loop accidentally.
5. Treat descriptor exhaustion and registration failure as handled runtime conditions with cleanup and logging.

**Files:** `event_loop.cpp`, `event_loop.hpp`, typed config API from A3.  
**Addresses:** F-13, F-18, F-22, F-23, F-28.  
**Done when:** Two configured endpoints serve independent connections concurrently through the same epoll instance, and one client failure leaves both listeners operational.

### B3. Implement per-client state and incremental request input

**Tasks:**

1. Define connection states for reading headers, reading a fixed-length/chunked body, processing, writing a response, waiting for future CGI I/O, and closing.
2. Store input buffers, parse progress, body limits, response state, timestamps, listener/server association, and any future CGI descriptors in each `Connection`.
3. Use **level-triggered `EPOLLIN`/`EPOLLOUT` for the first correct implementation**. This removes the current edge-trigger drain defect. Add bounded work per connection per loop iteration so one ready client cannot starve others.
4. Append bytes across reads and invoke the HTTP parser only on buffered input. Do not treat one `recv` as one request.
5. Enforce header/body limits while receiving; produce the appropriate error response when possible and transition to write/close rather than discarding the connection silently.
6. Handle `recv == 0` as orderly disconnect and negative returns without consulting `errno` after the read. Use event flags and connection state for subsequent handling.
7. After a complete response, either reset state for the next request when persistence is supported or close deliberately when the response says `Connection: close`.

**Files:** `event_loop.cpp`, connection-state definitions, HTTP-parser integration boundary.  
**Addresses:** F-20, F-21, F-26, F-27, F-30.  
**Done when:** Byte-at-a-time requests, split headers/bodies, requests larger than 4096 bytes, and sequential requests on one connection all produce the same parser-visible messages as coalesced input.

### B4. Make all socket output readiness-driven and backpressure-aware

**Tasks:**

1. Remove direct sends from request-read handling and from `respond`/`send_file` in their current form.
2. Build or acquire a response, then store an output buffer and sent offset in `Connection`. Enable `EPOLLOUT` only while output is pending.
3. Call `send` only after an epoll writable notification. Handle short writes by advancing the offset and retaining the remainder; disable `EPOLLOUT` when complete.
4. Use `MSG_NOSIGNAL` consistently and also ignore `SIGPIPE` at startup as defense in depth.
5. For large static files, retain file state and refill a bounded output queue as space becomes available. Regular-file reads are exempt from epoll readiness, but socket sends are not. Avoid unbounded buffering of slow clients.
6. Route file open/read/allocation failures through response generation so a configured error page can be queued instead of closing with an empty or truncated response.

**Files:** `event_loop.cpp`, response/file streaming integration, `signal.cpp`.  
**Addresses:** F-19, F-24, F-25, F-30.  
**Done when:** A client that stops reading does not block other clients, the server never writes without `EPOLLOUT`, and resumed writes preserve exact response bytes.

### B5. Add event-flag handling, deadlines, and deterministic cleanup

**Tasks:**

1. Inspect every returned event mask. Handle `EPOLLERR`, `EPOLLHUP`, and `EPOLLRDHUP` explicitly and consistently with partial buffered input/output.
2. Store last-activity and phase deadlines. Use a finite epoll wait computed from the nearest deadline and a monotonic time source where available.
3. Implement request-read, idle-keepalive, and response-write timeouts; leave a CGI deadline hook for the future CGI manager. Queue 408 for an incomplete request when feasible, then close after output.
4. Centralize cleanup: deregister descriptors where needed, close client and future CGI descriptors, release buffers/files, and remove state exactly once.
5. Keep regular-file I/O outside epoll as permitted, while including every socket and future CGI pipe in the same epoll instance.

**Files:** `event_loop.cpp`, connection-state definitions.  
**Addresses:** F-22, F-26, F-27, F-28.  
**Done when:** Idle, slow-read, and non-reading clients expire without affecting other traffic, and repeated connect/disconnect stress does not grow descriptor or connection counts.

### B6. Make signals safe and shutdown graceful

**Tasks:**

1. Reduce signal handlers to assignment of a `volatile sig_atomic_t` flag; perform no stream output or allocation in handler context.
2. Register and check errors for `SIGINT` and `SIGTERM`; ignore `SIGPIPE`. Add `SIGCHLD` handling only with the CGI lifecycle implementation.
3. On shutdown, stop accepting, close or drain connections according to a bounded policy, clean up all descriptors, and exit through one path with status 0 for an intentional signal.
4. Log shutdown from normal event-loop context, not from the handler.

**Files:** `signal.cpp`, `signal.hpp`, `event_loop.cpp`.  
**Addresses:** F-25, F-28, F-29.  
**Done when:** SIGINT/SIGTERM during idle wait and active traffic produce clean, leak-free shutdown without unsafe handler calls.

### B7. Add event-loop integration and stress tests

**Tasks:**

1. Use Python or Go clients to test fragmented writes, large/coalesced input, sequential persistent requests, half-closes, abrupt disconnects, and multiple simultaneous clients.
2. Test two or more configured listeners and verify that each connection retains the correct server/location configuration.
3. Force backpressure with a large response and a non-reading client; verify partial-write resumes, bounded memory, timeout, and continued service to other clients.
4. Exercise listener/client registration failures where practical, malformed startup configurations, descriptor exhaustion behavior, SIGPIPE-sensitive disconnects, SIGINT, and SIGTERM.
5. Run prolonged connection churn and inspect descriptor counts, memory growth, hangs, and unexpected process exits.

**Files:** new runtime integration/stress test scripts and fixtures.  
**Addresses:** F-17 through F-30 as applicable.  
**Done when:** All tests pass under the required C++98 build, and the server remains available after every client-side fault.

## 7. Shared integration sequence and ownership boundaries

| Order | Work | Owner | Dependency / acceptance gate |
|---|---|---|---|
| 1 | Freeze grammar, typed config API, endpoint uniqueness, and default path | A leads; B approves runtime API | A1 and A3 interface definitions complete |
| 2 | Build parser/semantic model and tests | A | A2, A4–A6; valid two-server fixture available |
| 3 | Build runtime against a small typed-config stub | B | B1–B6; no dependency on parser internals |
| 4 | Replace stub with real configuration and bind all endpoints | B with A support | Multiple-listener integration test passes |
| 5 | Pass server/location/body-limit/error-page data into HTTP processing | B integrates; A resolves model defects | F-30 closed; route-specific behavior demonstrated |
| 6 | Rewrite samples and architecture notes; run combined regression/stress suite | A documentation; B runtime validation | No conflicting grammar or invalid positive sample remains |

**Interface rule:** Developer B must not read `D_Block::dSimple` or `D_Block::dBlock` directly. Runtime code consumes only the typed model from A3. This prevents parser representation changes from destabilizing the event loop.

**Error-boundary rule:** Invalid configuration is a clean startup failure. Once the loop is running, malformed client traffic and per-connection I/O errors are isolated to that connection and never invoke `exit()`.

## 8. Recommended implementation priorities

### P0 — Restore a compliant, testable core

- Remove C++11/warning build blockers (F-01).
- Freeze grammar and produce one valid positive configuration (F-02, F-08).
- Establish typed configuration and complete error-page/location preservation (F-09, F-10).
- Connect all configured endpoints to one epoll instance (F-18).
- Add per-client state and readiness-driven buffered output (F-19 through F-25).

### P1 — Close correctness and resilience gaps

- Strict parser context/EOF/trailing-input validation and numeric checks (F-03 through F-07, F-12 through F-15).
- Timeouts, event-mask handling, centralized cleanup, and safe signals (F-26 through F-29).
- Enforce body limits and consume route/error-page configuration (F-30).

### P2 — Make the system maintainable and demonstrable

- Source-aware diagnostics, exception-safe error-page loading, and complete documentation (F-11, F-16).
- Comprehensive positive/negative configuration fixtures and runtime stress tests (F-17).

## 9. Final acceptance checklist

- [ ] All reviewed production units compile with `c++ -std=c++98 -Wall -Wextra -Werror`.
- [ ] Every supplied positive `.conf` parses; every negative fixture fails with a specific source-aware diagnostic.
- [ ] Two or more configured interface/port pairs operate through one epoll instance.
- [ ] Every socket and CGI-pipe read/write is non-blocking and follows the appropriate epoll readiness event; regular files remain the only exemption.
- [ ] Fragmented/coalesced requests and partial writes are handled by persistent per-client state.
- [ ] The 40 configured error pages are present in the typed model and usable by response generation.
- [ ] Every required route directive is represented and reaches routing/handling code.
- [ ] Idle, incomplete, disconnected, and non-reading clients cannot hang or terminate the server.
- [ ] SIGINT/SIGTERM shutdown cleans up every descriptor and active connection.
- [ ] Configuration and runtime regression/stress tests pass without descriptor leaks, unexpected exits, or invalid sample files.
