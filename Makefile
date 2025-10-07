LDFLAGS = -lcriterion

all: server_db test

testtest: tests/smoke.c
	gcc tests/smoke.c -o bin/smoke $(LDFLAGS)

test: testtest
	./bin/smoke

server_db: bin/server

bin/server: build/server.o build/http.o build/ip.o
	gcc -g $(LDFLAGS) -o bin/server build/server.o build/http.o build/ip.o

build/server.o: src/server/server.c
	gcc -Iinclude -g -c src/server/server.c -o build/server.o

build/http.o: src/server/http.c
	gcc -Iinclude -g -c src/server/http.c -o build/http.o

build/ip.o: src/server/ip.c
	gcc -Iinclude -g -c src/server/ip.c -o build/ip.o

clean:
	rm -f build/*.o bin/server bin/smoke


