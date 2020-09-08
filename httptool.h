//
// Created by bavel on 2020/9/8.
//

#ifndef EPORTALACTIVATOR_HTTPTOOL_H
#define EPORTALACTIVATOR_HTTPTOOL_H
#define HTTP_USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.83 Safari/537.36"
#define BUFFER_SIZE 1024
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>


void http_get(const char *url, char *response);


#endif //EPORTALACTIVATOR_HTTPTOOL_H
