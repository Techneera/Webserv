# CONFIGURATION FILE STRUCUTRE #


|Sources |
|-|
|https://nginx.org/en/docs/beginners_guide.html|


- As one of the tasks of a webserver is serving files, depending on file it is requested to serve, it can define a configuration file that systematically presents information on what are the directories to fetch the files that are requested.
    |Directory|Content|
    |-|-|
    |`/data/www/`|`html` files|
    |=|=|
    |/data/images/|`images`|


- The configuration file is constituted by `directives` divided into `simple` and `block` directives.

    - If a block directive can have other directives inside braces, it is called a `context`.

    | Directive|  Description|
    |-|-|
    |Simple|  Consists of a `name` and `parameters` separated by `spaces` and ends with `;`|
    |=|=|
    |Block     |Same strucutre as a simple directive, but ends with a set of additional instructions surrounded by `{` `}`|
    |=|=|
    |Context|Directive that contains other directives (`events`, `http`, `server`, `location`, etc..)|


    - Directives placed in the configuration file outside of any contexts are consisered to be the `main` context

    |Directives|Location|
    |-|-|
    | `events`, `http` |Outside of any contexts (`main`)|
    |=|=|
    |`server`| `http`|
    |=|=|
    |`location`| `server`|


- Directives

    - Simple

        - `listen` defines the port to listen to

        - `host` defines the ip to listen listen to

        - `client_max_body_size` defines the max client http request size

        - `error_pages` defines the serveral error pages root directory


    - Block

        - `http` defines the whole configuration file block directives except http itself

        - `server` defines a whole socket configuration, containing all the simple and block directives, except server and http
        
        - `location` defines the directory to look for matching the first directory after the `main page` address.
            It may contain another block called `cgi`.

        |Request          |Configuration file        | Directory     |
        |-|-|-|
        |http://localhost:8000/homepage.html |/location / {root /data/www} |/data/www/homepage.html |
        |=|=|=|
        |http://localhost/images/image.png|location /images {root /data}|/data/images/image.png|
        |=|=|=|
        |http://localhost/somedirectory/somefile.extension|location / {root /data/www}| /data/www/somedirectory/somefile.extension|


        - `cgi` TODO: 1- To define and understand


- `#` marks the start of a comment line


# CONFIGURATION FILE PARSER #

- Order of tokens:
    |order|token|mandatory|
    |-|-|-|
    |1|directive name|yes|
    |=|=|=|
    |2|directive flag| yes if location|
    |=|=|=|
    |3|'{'| yes if block|
    |=|=|=|
    |4|
    |=|=|=|
    |5|
    |=|=|=|
    |=|=|=|
    |=|=|=|
    |=|=|=|

