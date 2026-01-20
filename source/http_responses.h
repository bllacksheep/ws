#ifndef _HTTP_RESP_H
#define _HTTP_RESP_H 1

const char *_405_method_not_allowed =
    "\n\nHTTP/1.1 405 Method Not Allowed\r\n"
    "Allow: GET\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 18\r\n\r\n"
    "Method Not Allowed\n";

const char *_400_bad_request = "\n\nHTTP/1.1 400 Bad Request\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 13\r\n\r\n"
                                  "That Ain't It\n";

const char *_200_ok = "HTTP/1.1 200 OK\r\n"
                         "Server: server/0.0.0 (Ubuntu)\r\n"
                         "Content-Length: 0\r\n"
                         "Date: Thu, 06 Nov 2025 21:58:42 GMT\r\n"
                         "Content-Type: text/plain\r\n"
                         "Connection: keep-alive\r\n\r\n";

#endif
