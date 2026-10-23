# CONFIGURATION FILE STRUCUTRE #

|Sources |
|-|
|https://nginx.org/en/docs/beginners_guide.html|


- As one of the tasks of a webserver is serving files, depending on file it is requested to serve, it can define a configuration file that systematically presents information on what are the directories to fetch the files that are requested.
    
    :arrow_forward:

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


    - `#` marks the start of a comment line
