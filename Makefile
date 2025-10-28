CC = gcc
CFLAGS = -Iinclude -g
LDFLAGS = -lcriterion
LDLIBS = -lm

SRC = src/server
OBJ = build/server.o build/conn_man.o build/http.o build/ip.o build/parser.o build/hash_table.o

all: server_db test

server_db: bin/server

bin/server: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

testtest: tests/smoke.c
	$(CC) $(CFLAGS) -c tests/smoke.c -o build/smoke.o
	$(CC) build/smoke.o -o bin/smoke $(LDFLAGS)

test: testtest
	./bin/smoke

clean:
	rm -f build/*.o bin/server bin/smoke
