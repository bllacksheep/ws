client_db:
	gcc -Iinclude -g src/client/ws-client.c -o bin/client

server_db: bin/server

bin/server: build/ws-server.o build/http.o
	gcc -g -o bin/server build/ws-server.o build/http.o

build/ws-server.o: src/server/ws-server.c
	gcc -Iinclude -g -c src/server/ws-server.c -o build/ws-server.o

build/http.o: src/server/http.c
	gcc -Iinclude -g -c src/server/http.c -o build/http.o

clean:
	rm -f build/*.o bin/server


