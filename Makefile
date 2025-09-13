client:
	gcc -Iinclude src/client/ws-client.c -o bin/client

server:
	gcc -Iinclude src/server/ws-server.c -o bin/server
