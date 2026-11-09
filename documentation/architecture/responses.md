# HTTP ERROR RESPONSES STRUCTURES #


|Response Type|STATUS CODE|Base Structure|
|-|-|-|
| | 400, 402-404|HTTP/1.1 <STATUS_CODE> <REASON_PHRASE>\r\n|
| | 406, 408-415|Content-Type: text/html\r\n|
|~Generic|417, 418|Content-Length: <BODY_LENGTH>\r\n|
|Error|421-426, 428|Connection: close\r\n|
|Response|431, 451|\r\n|
| |  500-502, 504-508|<BODY>|
| |510, 511| |
|==============|===================|=====================================================|
| | |HTTP/1.1 401 Unauthorized\r\n|
| |  |WWW-Authenticate: <AUTHENTICATION_CHALLENGE>\r\n|
| |401 |Content-Type: text/html\r\n|
| | |Content-Length: <BODY_LENGTH>\r\n|
| | |Connection: close\r\n|
| | |\r\n|
|Authentication | |<BODY>|
|Errors | | |
| | |HTTP/1.1 407 Proxy Authentication Required\r\n|
| |  |WWW-Authenticate: <PROXY_AUTHENTICATION_CHALLENGE>\r\n|
| |407 |Content-Type: text/html\r\n|
| | |Content-Length: <BODY_LENGTH>\r\n|
| | |Connection: close\r\n|
| | |\r\n|
| | |<BODY>|
|==============|===================|=====================================================|
| | |HTTP/1.1 405 Method Not Allowed\r\n|
| | |Allow: <SUPPORTED_METHODS>\r\n|
|Method | |Content-Type: text/html\r\n|
| Not|405 |Content-Length: <BODY_LENGTH>\r\n|
|Allowed | |Connection: close\r\n|
| | |\r\n|
| | |<BODY>|
|==============|===================|=====================================================|
| | |HTTP/1.1 416 Range Not Satisfiable\r\n|
| | |Content-Range: <UNSATISFIED_RANGE>\r\n|
|Range | |Content-Type: text/html\r\n|
| Errors|416|Content-Length: <BODY_LENGTH>\r\n|
| | |Connection: close\r\n|
| | |\r\n|
| | |<BODY>|
|==============|===================|=====================================================|
| | |HTTP/1.1 426 Upgrade Required\r\n|
| | |Upgrade: <SUPPORTED_PROTOCOL>\r\n|
|Protocol| |Connection: Upgrade\r\n|
|Upgrade|426|Content-Type: text/html\r\n|
| | |Content-Length: <BODY_LENGTH>\r\n|
| | |\r\n|
| | |<BODY>|
|==============|===================|=====================================================|
| | |HTTP/1.1 429 Too Many Requests\r\n|
| | |Retry-After: <RETRY_DELAY>\r\n|
|Rate| |Content-Type: text/html\r\n|
| Limiting|429 |Content-Length: <BODY_LENGTH>\r\n|
| | |Connection: close\r\n|
| | |\r\n|
| | |<BODY>|
|==============|===================|=====================================================|

