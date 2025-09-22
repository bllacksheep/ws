LDFLAGS = -lcriterion

client_db:
	gcc -Iinclude -g src/client/ws-client.c -o bin/client

all: server_db test

testtest: tests/smoke.c
	gcc tests/smoke.c -o bin/smoke $(LDFLAGS)

test: testtest
	./bin/smoke

server_db: bin/server

bin/server: build/ws-server.o build/http.o
	gcc -g $(LDFLAGS) -o bin/server build/ws-server.o build/http.o

build/ws-server.o: src/server/ws-server.c
	gcc -Iinclude -g -c src/server/ws-server.c -o build/ws-server.o

build/http.o: src/server/http.c
	gcc -Iinclude -g -c src/server/http.c -o build/http.o

clean:
	rm -f build/*.o bin/server bin/smoke


