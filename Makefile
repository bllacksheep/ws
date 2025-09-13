client_db:
	gcc -Iinclude -g src/client/ws-client.c -o bin/client

server_db:
	gcc -Iinclude -g src/server/ws-server.c -o bin/server
