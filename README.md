# ws
Bare minimum implementation of rfc 2616, 9110 and 9112 to facilitate bootstrapping websocket connections rfc 6455. Should only support recieving websocket handshake http requests and websocket stream establishment.
```
make clean
make all # builds tests
# only the server
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
