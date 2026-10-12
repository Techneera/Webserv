# Webserv Data Structures

## Overview

This document describes the core data structures used in the whole webserv project.

---

# RequestLine

Represents the first line of an HTTP request.

```cpp
typedef struct
{
    std::string method;
    RequestTarget requestTarget;
    uint8_t version;
} RequestLine;
```

## Fields

| Field | Type | Description |
|---------|------|-------------|
| `method` | `std::string` | HTTP method (e.g., `GET`, `POST`, `PUT`, `DELETE`). |
| `requestTarget` | `RequestTarget` | Parsed request target in one of the supported HTTP forms. |
| `version` | `uint8_t` | HTTP protocol version identifier. |

## Example

```
GET /index.html HTTP/1.1
```

Would produce:

```cpp
method = "GET";
version = 1;
requestTarget.type = ORIGIN_e;
```

---

# OriginForm

Represents the **origin-form** request target used by most HTTP requests.

```cpp
typedef struct
{
    std::string path;
    std::string query;
} OriginForm;
```

## Fields

| Field | Type | Description |
|---------|------|-------------|
| `path` | `std::string` | Requested resource path. |
| `query` | `std::string` | Raw query component without the `?` separator. |

## Example

Request target:

```
/api/users?id=42
```

Parsed as:

```cpp
path = "/api/users";
query = "id=42";
```

---

# AbsoluteForm

Represents the **absolute-form** request target.

```cpp
typedef struct
{
    std::string scheme;
    std::string host;
    uint16_t port;
    std::string path;
    std::string query;
} AbsoluteForm;
```

## Fields

| Field | Type | Description |
|---------|------|-------------|
| `scheme` | `std::string` | URI scheme (`http`, `https`, etc.). |
| `host` | `std::string` | Target host name or IP address. |
| `port` | `uint16_t` | Target port number. |
| `path` | `std::string` | Resource path. |
| `query` | `std::string` | URI query string. |

## Example

Request target:

```
http://example.com:8080/index.html?lang=en
```

Parsed as:

```cpp
scheme = "http";
host = "example.com";
port = 8080;
path = "/index.html";
query = "lang=en";
```

---

# AuthorityForm

Represents the **authority-form** request target used primarily by the `CONNECT` method (may not be used in this project).

```cpp
typedef struct
{
    std::string host;
    uint16_t port;
} AuthorityForm;
```

## Fields

| Field | Type | Description |
|---------|------|-------------|
| `host` | `std::string` | Target host name or IP address. |
| `port` | `uint16_t` | Target port number. |

## Example

Request target:

```
example.com:443
```

Parsed as:

```cpp
host = "example.com";
port = 443;
```

---

# AsteriskForm

Represents the **asterisk-form** request target.

```cpp
typedef struct
{
} AsteriskForm;
```

## Usage

Used with server-wide operations, typically the `OPTIONS` method (may not be used in this project).

## Example

Request target:

```
*
```

---

# FormType

Enumerates the possible request target formats.

```cpp
typedef enum
{
    ORIGIN_e,
    ABSOLUTE_e,
    AUTHORITY_e,
    ASTERISK_e
} FormType;
```

## Values

| Value | Description |
|---------|-------------|
| `ORIGIN_e` | Origin-form request target. |
| `ABSOLUTE_e` | Absolute URI request target. |
| `AUTHORITY_e` | Authority-form request target. |
| `ASTERISK_e` | Asterisk-form request target. |

---

# RequestTarget

Represents a polymorphic request target capable of holding any supported target form.

```cpp
typedef struct
{
    FormType type;

    union
    {
        OriginForm origin;
        AbsoluteForm absolute;
        AuthorityForm authority;
        AsteriskForm asterisk;
    };
} RequestTarget;
`

# Headers

Represents the collection of HTTP headers.

```cpp
typedef std::multimap<std::string, std::string> Headers;
```

## Rationale

A `std::multimap` is used because HTTP allows multiple headers with the same name.

## Example

```
Set-Cookie: session=abc
Set-Cookie: theme=dark
```

Stored as:

```cpp
headers.insert({"Set-Cookie", "session=abc"});
headers.insert({"Set-Cookie", "theme=dark"});
```

---

# HttpRequest

Top-level structure representing a complete HTTP request.

```cpp
typedef struct HttpRequest
{
    RequestLine requestLine;
    Headers headers;
    std::string body;
} HttpRequest;
```

## Fields

| Field | Type | Description |
|---------|------|-------------|
| `requestLine` | `RequestLine` | Request method, target, and protocol version. |
| `headers` | `Headers` | Collection of HTTP header fields. |
| `body` | `std::string` | Request payload/body content. |

## Example Structure

For the request:

```http
POST /api/users HTTP/1.1
Host: example.com
Content--Length: 18
{"name":"Alice"}
```

The resulting object would conceptually contain:

```cpp
HttpRequest request;
request.requestLine.method = "POST";
request.requestLine.version = 1;
request.headers.insert({"Host", "example.com"});
request.headers.insert({"Content-Type", "application/json"});
request.body = R"({"name":"Alice"})";
```
