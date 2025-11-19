# ws
Bare minimum implementation of rfc 2616, 9110 and 9112 to facilitate bootstrapping websocket connections rfc 6455. Should only support recieving websocket handshake http requests and websocket stream establishment.

`siege` required for testing.

```console
make clean
# build tests
make all
# server only
make server_db
```
usage
```console
# optional address and port :- default 127.0.0.1 443
bin/server
bin/server <address> <port>
```
bench
```
set ~/siege/siege.conf "connection = keep-alive"
siege -c1 -r10 http://localhost:443/chat
```
##todo
- ~implement fsm parser for http~
- ~implement request context pattern~
- ~use request context to communicate request state and conn state
- keepalive for http/1.1 and non-persistent conns for http/1.0 (ab)
- ~connection manager track state~
- remove realloc from hot path - use pre-allocation and allocator back cleanup
- implement chunked data tracking
- implement websocket stream handling
- test setup and stream with browser client
- build my own client after ensuring server works
