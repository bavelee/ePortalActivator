#include <stdio.h>
#include "httptool.h"

//#define PROGRAM_NAME "ePortalActivator"
#define PROGRAM_NAME "epact"
#define KEEP_ALIVE_INTERVAL 600

#define VER_STR "20200908-1"

//校园网Web认证网页的IP地址
#define XYW_HOSTNAME "http://10.0.11.5"

//退出登录的URL : userId -> 用户名 pass -> 密码
#define XYW_LOGOUT "%s/eportal/InterFace.do?method=logoutByUserIdAndPass&userId=%s&pass=%s"

//queryString 可以理解为认证服务器下发给客户端的口令
#define XYW_QUERY_STRING "http://123.123.123.123"

//需要替换掉的字符串
#define XYW_QUERY_STRING_PREFIX "<script>top.self.location.href='http://10.0.11.5/eportal/index.jsp?"

//登录认证的URL : userId -> 用户名 password -> 密码 queryString -> 未登陆时访问 XYW_HOSTNAME 可获取到
#define XYW_LOGIN "%s/eportal/InterFace.do?method=login&userId=%s&password=%s&service=&queryString=%s&operatorPwd=&operatorUserId=&validcode=&passwordEncrypt=false"

//保活，定期访问一次，保证不被踢下线，不一定有用 : userIndex -> login成功后服务器会返回userIndex
#define XYW_KEEP_ALIVE "%s/eportal/InterFace.do?method=keepalive&userIndex=%s"

void login();

void logout();

void keep_alive();

void real_login();

void get_query_string();

char *str_replace(char const *original,
                  char const *pattern, char const *replacement);

int indexOf(char *str1, char *str2);

char *username;
char *password;
char *query_string;

void usage() {
    printf("Usage : %s <账号> <密码> [start|login|logout|getqs]\n\t\
	start\t:\t登录校园网并持续进行注册(防止被检测下线 , 此为默认选项)\n\t\
	login\t:\t按顺序执行 [下线]-[获取queryString]-[登录]\n\t\
	logout\t:\t执行 [下线]\n\t\
	getqs\t:\t执行 [获取queryString]\n\t\
	version\t:\t%s\n", PROGRAM_NAME, VER_STR);
}

int main(int argc, char **argv) {

    if (argc < 3) {
        usage();
        return 1;
    }
    query_string = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    username = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    password = (char *) malloc(sizeof(char) * BUFFER_SIZE);

    strcpy(username, argv[1]);
    strcpy(password, argv[2]);
    printf("[账号] => %s\n", username);
    printf("[密码] => %s\n", password);
    if (argc < 4 || !strcmp(argv[3], "start")) {
        //每隔 KEEP_ALIVE_INTERVAL 秒汇报一次
        login();
        if (strlen(query_string) < 11) {
            fprintf(stderr, "[错误] => 未获取到queryString\n");
            return 1;
        }
        int i = 0;
        while (++i) {
            fprintf(stderr, "[保活] => 正在进行第 %5d 次保活\n", i);
            keep_alive();
            sleep(KEEP_ALIVE_INTERVAL);
        }
    } else if (!strcmp(argv[3], "login")) {
        fprintf(stdout, "[登录] => 尝试进行登录\n");
        login();
    } else if (!strcmp(argv[3], "logout")) {
        fprintf(stdout, "[退出] => 下线所有设备\n");
        logout();
    } else if (!strcmp(argv[3], "getqs")) {
        fprintf(stdout, "[QS] => 获取 queryString\n");
        get_query_string();
    }

    free(username);
    free(password);
    free(query_string);
    return 0;
}

//登录
void login() {
    get_query_string();
    real_login();
}

void logout() {
    char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    char *url = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    sprintf(url, XYW_LOGOUT, XYW_HOSTNAME, username, password);
    http_get(url, buffer);
    printf("%s\n", buffer);
    free(url);
    free(buffer);
}

void keep_alive() {
    real_login();
}

void real_login() {
    char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    char *url = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    sprintf(url, XYW_LOGIN, XYW_HOSTNAME, username, password, query_string);
    http_get(url, buffer);
    if (indexOf(buffer, "userIndex")) {
        fprintf(stdout, "[登录] => 登录成功\n");
    } else {
        fprintf(stdout, "[登录] => 登录失败，未获取到userIndex\n");
    }
    free(url);
    free(buffer);
}

void get_query_string() {
    logout();
    char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
    http_get(XYW_QUERY_STRING, buffer);
    //获取主页重定向后的get参数
    str_replace(buffer, XYW_QUERY_STRING_PREFIX, "");
    //获取主页重定向后的get参数
    char *s1 = str_replace(buffer, XYW_QUERY_STRING_PREFIX, "");
    //'后面的全部丢弃
    int i = indexOf(s1, "'");
    s1[i] = '\0';
    //模拟encodeURIComponent转码(实则就是字符串替换)
    char *s2 = str_replace(s1, "=", "%3D");
    s1 = str_replace(s2, "&", "%26");
    s2 = str_replace(s1, ":", "%3A");
    s1 = str_replace(s2, "/", "%2F");
    s2 = str_replace(s1, "%", "%25");
    strcpy(query_string, s2);
    fprintf(stdout, "[获取到QS] => %s\n", query_string);
    free(s1);
    free(s2);
    free(buffer);
}


char *str_replace(char const *original,
                  char const *pattern, char const *replacement) {
    size_t const replen = strlen(replacement);
    size_t const patlen = strlen(pattern);
    size_t const orilen = strlen(original);

    size_t patcnt = 0;
    const char *oriptr;
    const char *patloc;

    // find how many times the pattern occurs in the original string
    for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen) {
        patcnt++;
    }

    {
        // allocate memory for the new string
        size_t const retlen = orilen + patcnt * (replen - patlen);
        char *const returned = (char *) malloc(sizeof(char) * (retlen + 1));

        if (returned != NULL) {
            // copy the original string,
            // replacing all the instances of the pattern
            char *retptr = returned;
            for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen) {
                size_t const skplen = patloc - oriptr;
                // copy the section until the occurence of the pattern
                strncpy(retptr, oriptr, skplen);
                retptr += skplen;
                // copy the replacement
                strncpy(retptr, replacement, replen);
                retptr += replen;
            }
            // copy the rest of the string.
            strcpy(retptr, oriptr);
        }
        return returned;
    }
}

/*返回str2第一次出现在str1中的位置(下表索引),不存在返回-1*/
int indexOf(char *str1, char *str2) {
    char *p = strstr(str1, str2);
    int i = 0;
    if (p == NULL)
        return -1;
    else {
        while (str1 != p) {
            str1++;
            i++;
        }
    }
    return i;
}