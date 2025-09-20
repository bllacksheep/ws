# ws
Bare minimum implementation of rfc 2616, 9110 and 9112 to facilitate bootstrapping websocket connections rfc 6455. Should only support recieving websocket handshake http requests and websocket stream establishment.
```
make clean
make server_db
```
bench
```
set ~/siege/siege.conf `connection = keep-alive`
siege -c1 -r10 http://localhost:443/chat
```
