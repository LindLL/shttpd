#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <assert.h>
#include <stdbool.h>
#include <syslog.h>
#include <ctype.h>

int main(int argc, char const *argv[])
{//num1=1&num2=2&operation=add
    char *Method = getenv("REQUEST_METHOD");
    if (!strcmp(Method, "GET"))
    {
        char *QueryString = getenv("QUERY_STRING");
        char *token = strtok(QueryString, "&");
        char *num1 = NULL, *num2 = NULL, *operation = NULL;
        int intnum1, intnum2, res;
        num1 = strchr(token, '=') + 1;
        token = strtok(NULL, "&");
        num2 = strchr(token, '=') + 1;
        token = strtok(NULL, "&");
        operation = strchr(token, '=') + 1;
        intnum1 = atoi(num1);
        intnum2 = atoi(num2);
        if (!strncmp(operation, "add", 3))
        {
            res = intnum1 + intnum2;
        }
        else if (!strncmp(operation, "sub", 3))
        {
            res = intnum1 - intnum2;
        }
        printf("HTTP/1.1 200 OK\r\n");
        printf("Connection: keep-alive\r\n");
        printf("Content-type: text/html; charset=utf-8\r\n");
        printf("\r\n");
        printf("<!DOCTYPE html>\n");
        printf("<html lang='en'>\n");
        printf("<head>\n");
        printf("<meta charset='UTF-8'>\n");
        printf("<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
        printf("<title>Calculator Result</title>\n");
        printf("</head>\n");
        printf("<body>\n");
        printf("<h2>Calculator Result</h2>\n");
        printf("<p>Operation: %s</p>\n", operation);
        printf("<p>Number 1: %d</p>\n", intnum1);
        printf("<p>Number 2: %d</p>\n", intnum2);
        printf("<p>Result: %d</p>\n", res);
        printf("</body>\n");
        printf("</html>\n");
    }
    else if (!strcmp(Method, "POST"))
    {
        char *ContentLength = getenv("CONTENT_LENGTH");
        int cl = atoi(ContentLength);
        if (cl > 0)
        {
            char *postdata = malloc(cl + 1);
            if (!postdata)
            {
                printf("HTTP/1.1 500 Server Malfunction\r\n");
                printf("Connection: close\r\n");
                printf("Content-type: text/plain; charset=utf-8");
                printf("\r\n");
                printf("NO POST DATA");
            }
            else
            {
                printf("HTTP/1.1 200 OK");
                printf("Connection: keep-alive\r\n");
                printf("Content-type: text/plain; charset=utf-8");
                printf("\r\n");
                printf("Your POST data is %s\r\n", postdata);
            }
        }
    }
    return 0;
}
