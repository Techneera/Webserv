# Debug Information

- Use the following command to assess the state of the server and clients.

```C
ss -tlnp
```
|flag|Meaning|Relevance|
|--:|--:|--:|
|-t|TCP sockets only|filters out UPD connections|
|-l|listening sockets only|only thing program does at this stage|
|-n|numeric ports/IPs (no DNS/service name lookup)|instant output, shows 8080 not http-alt|
|-p|owing process|display the name of the actual program|

- Use the following command to see accepted client connections, this is how to watch keep-alive connections accumulation.

```Bash
ss -tn state established'(dport=:8080)'

ss -tni
```

