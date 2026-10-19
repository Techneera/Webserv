# The webserv shall be written in C++ 98

WEBSERV
---

| Requisite  | Assigned to  |
|-|-|
|`use` a configuration file `when` the program starts  |                        |
|`if` a configuration file is provided as an argument | `Julio` |
|`or` it is in a default path | |
|`otherwise` terminate with an error| |
| ======================================================= | ========================== |
|`ignore` errno after a read or write operation | `Rafa Alex` |
|`if` any of these functions returns an error | |
|`otherwise` proceed normally | |
| ======================================================= | ========================== |
| `remain` non-blocking | `Rafa Alex` |
|`when` multiple clients are connected | |
| ======================================================= | ========================== |
| `handle` client disconnections | `Rafa Alex` |
| `when` necessary | |
| ======================================================= | ========================== |
| `use` only 1 poll or equivalent | `Rafa Alex` |
| `when` any I/O operation happens between | |
| client and server, including listen() | |
| ======================================================= | ========================== |
| `perform` I/O operations without poll() | `Rafa Alex` |
| `when` any read or write operation is necessary | |
| `if` it is on regular disk files | |
| ======================================================= | ========================== |
| `use` poll() to monitor both reading and writing | `Rafa Alex` |
| simultaneously | |
| ======================================================= | ========================== |
| perform a read or write operation | `Rafa Alex` |
| only after going through poll() or equivalent | |
| ======================================================= | ========================== |
| `use` any associated macro or helper function | `Rafa Alex` |
| `when` using poll() or equivalent call | |
| ======================================================= | ========================== |
| `be` compatible with standards web browsers | `Rafa Alex` |
| ======================================================= | ========================== |
| `provide` accurate HTTP responses and status | `Rafa Alex` |
| codes without hanging indefinetely | |
| `when` a request is made to the server | |
| ======================================================= | ========================== |
| `serve` default error pages | `Rafa Alex` |
| `when` there is an HTTP error | |
| `if` no custom error pages are provided | |
| ======================================================= | ========================== |
| `use` fork() | `Rafa Alex` |
| `only` `when` CGI is used | |
| ======================================================= | ========================== |
| `serve` a fully static website | `Rafa Alex` |
| `when` a client requests for it | |
| ======================================================= | ========================== |
| `store` files | `Rafa Alex` |
| `when` a client uploads them | |
| ======================================================= | ========================== |
| `handle` GET request method | `Julio` |
| `when` a client makes a GET request | |
| `if` the request is valid as per the RFC | |
| `otherwise` provide a proper error response | |
| ======================================================= | ========================== |
| `handle` POST request method | `Julio` |
| `when` a client makes a POST request | |
| `if` the request is valid as per the RFC | |
| `otherwise` provide a proper error response | |
| ======================================================= | ========================== |
| `handle` DELETE request method | `Julio` |
| `when` a client makes a DELETE request | |
| `if` the request is valid as per the RFC | |
| `otherwise` provide a proper error response | |
| ======================================================= | ========================== |
| `listen` to multiple ports and deliver content | `Rafa Alex` |
| `when` the corresponding service is needed | |
| `if` the correct port is specified in the configuration | |
| `otherwise` provide an internal | |
| server error response | |


CONFIGURATION FILE REQUIREMENTS
---

| Requisite  | Assigned to  |
|---------------------------|------------------------|
| `define` all the interface:port pairs (127.0.0.1:3000) | `Julio` |
| the server will listen to |  |
| =============================================================== | ========================== |
| `provide` setup for default error pages | `Julio` |
| =============================================================== | ========================== |
| `set` maximum allowed size for client request bodies | `Julio` |
| =============================================================== | ========================== |
| `specify` rules on a route for a website as defined below | `Julio` |
| List of accepted methods on the route | |
| HTTP redirection | |
| Directory where the requested file should be located | |
| Enabling or disabling directory listing. | |
| Default file to serve `when` the requested resource | |
| is a directory. | |
| Uploading files from the clients to the server is | |
| authorized, and storage location is provided. | |
| Execution of CGI based on file extension | |


CGI (Common Gateway Interface)
---

| Requisite  | Assigned to  |
|---------------------------|------------------------|
| `find` the correct directory to run the requested | `Rafa Alex` |
| process. | |
| =============================================================== | ========================== |
| `run` in the correct direcotry for relative path | `Rafa Alex` |
| file access. | |
| =============================================================== | ========================== |
| `work` with the environment variables | `Rafa Alex` |
| involved in the web-CGI communication. | |
| =============================================================== | ========================== |
| `interpret` the full request and arguments | `Rafa Alex` |
| provided by the client. | |
| =============================================================== | ========================== |
| `unchunk` chunked requests, expecting EOF as the | `Rafa Alex` |
| end of the body. | |
| =============================================================== | ========================== |
| `send` an EOF a the end of the output | `Rafa Alex` |
| `when` no content_length is returned from the CGI. | |
| (THIS NEEDS A SERIOUS REVIEW) | |


GENERAL
---

| Requisite  | Assigned to  |
|---------------------------|------------------------|
| `configuration` files and default files `shall` be | `Rafa Alex` |
| provided to prove every feature works | |
