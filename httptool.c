//
// Created by bavel on 2020/9/8.
//

#include "httptool.h"

void mk_request_body(const char *key, const char *value, char *body) {
    strcat(body, key);
    strcat(body, ": ");
    strcat(body, value);
    strcat(body, "\r\n");
}

void pexit(char *s) {
    fprintf(stderr, "error : %s\n", s);
}

// extract hostname from url
static unsigned get_hn(const char *url, char *hostname) {
    unsigned i;

    for (i = 0; url[i] != '\0' && url[i] != ':' && url[i] != '/'; i++) {
        if (i >= 1024) pexit("too long hostname in URL");
        hostname[i] = url[i];
    }
    hostname[i] = '\0';

    return i;
}

// extract port number
static unsigned get_port(const char *url, char *port, unsigned url_i) {
    unsigned i;

    for (i = 0; url[i] != '\0' && url[i] != '/'; i++, url_i++) {
        if ('0' <= url[i] && url[i] <= '9') port[i] = url[i];
        else pexit("wrong decimal port number");
    }
    if (i <= 6) port[i] = '\0';
    else pexit("too long port number");

    return url_i;
}

void parse_url(const char *url, char *hostname, char *port, char *path) {
    unsigned i = 7, j, len;
    if (strncmp(url, "http://", i) != 0) pexit("only HTTP support");
    len = get_hn(url + i, hostname);
    i += len;

    // get port if exists
    if (url[i] == ':') {
        i++;
        i = get_port(url + i, port, i);
    } else strcpy(port, "80");

    // get uri in URL
    if (url[i] == '\0') strcpy(path, "/");
    else if (url[i] == '/') {
        if (strlen(url + i) < 1024) strcpy(path, url + i);
        else pexit("too long path in URL");
    } else pexit("wrong URL");
}

// connect to any IPv4 or IPv6 server
static int conn_svr(const char *hostname, const char *port) {
    struct addrinfo hints, *result, *rp;
    int sock;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if ((errno = getaddrinfo(hostname, port, &hints, &result)))
        fprintf(stderr, "getaddrinfo: %s", gai_strerror(errno));

    // try all address list(IPv4 or IPv6) until success
    for (rp = result; rp; rp = rp->ai_next) {
        if ((sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))
            == -1) {
            pexit("socket error");
            continue;
        }
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // succeed in connecting to any server IP
        else pexit("connect error");
        close(sock);
    }
    freeaddrinfo(result);
    if (!rp) pexit("can't connect");
    return sock;
}

// get http response body starting address and its length
static char *parse_body(ssize_t len, ssize_t *body_len, char *buffer) {
    int i;
    for (i = 0; i < len - 4; i++)
        if (!strncmp(buffer + i, "\r\n\r\n", 4)) break;

    *body_len = len - i - 4;
    return buffer + i + 4;
}

void http_get(const char *url, char *response) {
    int sock;
    ssize_t len, body_len;
    char hostname[BUFFER_SIZE], port[6], path[BUFFER_SIZE], buffer[BUFFER_SIZE];

    parse_url(url, hostname, port, path);
    sock = conn_svr(hostname, port);

    // compose HTTP request
    sprintf(buffer, "GET %s HTTP/1.1\r\n", path);
    mk_request_body("Host", hostname, buffer);
    mk_request_body("User-Agent", HTTP_USER_AGENT, buffer);
    mk_request_body("Connection", "close", buffer);
    strcat(buffer, "\r\n");

    // send the HTTP request
    len = strlen(buffer);
    if (write(sock, buffer, len) != len) pexit("write error");

    // read HTTP response
    if ((len = read(sock, buffer, BUFFER_SIZE)) == -1) pexit("read error");
    if (!strstr(buffer, "\r\n\r\n")) pexit("too long HTTP response");
    strcpy(response, parse_body(len, &body_len, buffer));
    response[body_len] = '\0';
}
