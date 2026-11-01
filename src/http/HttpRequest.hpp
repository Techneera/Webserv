#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include <string>
# include <stdint.h>

/**
 * @enum FormType_e
 * @brief Identifies the request-target form used in an HTTP request.
 */
enum FormType_e
{
    FORM_ABSOLUTE_e, /**< Absolute-form request target. */
    FORM_ORIGIN_e    /**< Origin-form request target. */
};

/**
 * @struct OriginForm_t
 * @brief Represents an HTTP origin-form request target.
 *
 * Contains the path and optional query component.
 */
typedef struct
{
    std::string path;  /**< Request target path. */
    std::string query; /**< Query component, if present. */
} OriginForm_t;

/**
 * @struct AbsoluteForm_t
 * @brief Represents an HTTP absolute-form request target.
 *
 * Contains the URI scheme, host, optional port, path and query.
 */
typedef struct
{
    std::string scheme; /**< URI scheme (e.g. http or https). */
    std::string host;   /**< Target host name. */
    std::string path;   /**< Request target path. */
    std::string query;  /**< Query component, if present. */
    uint16_t    port;   /**< Target port number. */
} AbsoluteForm_t;

/**
 * @struct RequestTarget_t
 * @brief Stores the request target and its corresponding form.
 *
 * The @c type field determines whether @c origin or @c absolute
 * contains the relevant request-target information.
 */
typedef struct
{
    FormType_e      type;     /**< Active request-target form. */
    OriginForm_t    origin;   /**< Origin-form representation. */
    AbsoluteForm_t  absolute; /**< Absolute-form representation. */
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
    RequestTarget_t requestTarget; /**< Request-target information. */
    uint8_t         version;       /**< HTTP version identifier. */
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
