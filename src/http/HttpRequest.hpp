#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include <string>
# include <stdint.h>
# include <map>

/**
 * @struct RequestTarget_t
 * @brief Stores the request target and its corresponding form.
 *
 * The @c type field determines whether @c origin or @c absolute
 * contains the relevant request-target information.
 */
typedef struct
{
    std::string path;  /**< Request target path. */
    std::multimap<std::string, std::string> query; /**< Query component, if present. */
} RequestTarget_t;

/**
 * @struct RequestLine_t
 * @brief Represents the HTTP request-line.
 *
 * Contains the HTTP method, request target and protocol version.
 */
typedef struct
{
    std::string     method;        /**< HTTP method (e.g. GET, POST). */
    std::string     version;       /**< HTTP version identifier. */
    RequestTarget_t requestTarget; /**< Request-target information. */
} RequestLine_t;

/**
 * @struct HttpRequest_t
 * @brief Represents an HTTP request.
 *
 * Currently contains the parsed HTTP request-line.
 */
typedef struct
{
    RequestLine_t requestLine; /**< HTTP request-line. */
} HttpRequest_t;

#endif
