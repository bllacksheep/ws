#include <parser.h>

char *req = "GET /chat HTTP/1.1\r\nHost: 127.0.0.1:443\r\nUser-Agent: "
            "curl/7.81.0\r\nAccept: */*\r\n\r\n";

// dynamic array returned of tokens?
// METHOD, PATH, VERSION, HEADERS - what's headers then
// can headers here, be parsed before being put into hashmap
// i.e. use the parsed output as input for hashmap

int main() { return 0; }
