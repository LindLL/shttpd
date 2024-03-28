#include "shttpd.h"
#include "shttpd_para.h"
#include "shttpd_log.h"

conf_opts conf_para = {
    .CGIPath = "./cgi-bin/",
    .DocumentRoot = "./",
    .DefaultFile = "index.html",
    .Port = 8080,
    .TimeOut = 3,
    .MaxClients = 50,
};

int serverfd;

CgiVar CGIENV[] = {
    {"CONTENT_LENGTH", &sContentlength},
    {"CONTENT_TYPE", &sContenttype},
    {"DOCUMENT_ROOT", &sDocumentRoot},
    {"HTTP_ACCEPT", &sAccept},
    {"HTTP_ACCEPT_ENCODING", &sAcceptEncoding},
    {"HTTP_COOKIE", &sCookie},
    {"HTTP_HOST", &sHost},
    {"HTTP_IF_MODIFIED_SINCE", &sIfModifiedSince},
    {"HTTP_IF_NONE_MATCH", &sIfNoneMatch},
    {"HTTP_REFERER", &sReferer},
    {"HTTP_USER_AGENT", &sAgent},
    {"QUERY_STRING", &sGetqueryString},
    {"REMOTE_ADDR", &sRemoteAddr},
    {"REQUEST_METHOD", &sMethod},
    {"REQUEST_URI", &sRequestURI},
    {"SCRIPT_DIRECTORY", &sSecretDirectory},
    {"SERVER_PROTOCOL", &sProtocol},
};

Mime_type builtin_mime_types[] = {
    {"html", MINET_HTML, 4, "text/html"},
    {"htm", MINET_HTM, 3, "text/html"},
    {"txt", MINET_TXT, 3, "text/plain"},
    {"css", MINET_CSS, 3, "text/css"},
    {"ico", MINET_ICO, 3, "image/x-icon"},
    {"gif", MINET_GIF, 3, "image/gif"},
    {"jpg", MINET_JPG, 3, "image/jpeg"},
    {"jpeg", MINET_JPEG, 4, "image/jpeg"},
    {"png", MINET_PNG, 3, "image/png"},
    {"svg", MINET_SVG, 3, "image/svg+xml"},
    {"torrent", MINET_TORRENT, 7, "application/x-bittorrent"},
    {"wav", MINET_WAV, 3, "audio/x-wav"},
    {"mp3", MINET_MP3, 3, "audio/x-mp3"},
    {"mid", MINET_MID, 3, "audio/mid"},
    {"m3u", MINET_M3U, 3, "audio/x-mpegurl"},
    {"ram", MINET_RAM, 3, "audio/x-pn-realaudio"},
    {"ra", MINET_RA, 2, "audio/x-pn-realaudio"},
    {"doc", MINET_DOC, 3, "application/msword"},
    {"exe", MINET_EXE, 3, "application/octet-stream"},
    {"zip", MINET_ZIP, 3, "application/x-zip-compressed"},
    {"xls", MINET_XLS, 3, "application/excel"},
    {"tgz", MINET_TGZ, 3, "application/x-tar-gz"},
    {"tar.gz", MINET_TARGZ, 6, "application/x-tar-gz"},
    {"tar", MINET_TAR, 3, "application/x-tar"},
    {"gz", MINET_GZ, 2, "application/x-gunzip"},
    {"arj", MINET_ARJ, 3, "application/x-arj-compressed"},
    {"rar", MINET_RAR, 3, "application/x-arj-compressed"},
    {"rtf", MINET_RTF, 3, "application/rtf"},
    {"pdf", MINET_PDF, 3, "application/pdf"},
    {"swf", MINET_SWF, 3, "application/x-shockwave-flash"},
    {"mpg", MINET_MPG, 3, "video/mpeg"},
    {"mpeg", MINET_MPEG, 4, "video/mpeg"},
    {"asf", MINET_ASF, 3, "video/x-ms-asf"},
    {"avi", MINET_AVI, 3, "video/x-msvideo"},
    {"bmp", MINET_BMP, 3, "image/bmp"},
    {NULL, -1, 0, NULL}};

static Mime_type *GetMimeType(const char *filename)
{
    char *suffix = NULL;
    char suffixname[20];
    suffix = strchr(filename, '.');
    if (!suffix)
        return NULL;
    else
    {
        strncpy(suffixname, suffix + 1, sizeof(suffixname) - 1);
        for (Mime_type *m = builtin_mime_types; m->SuffixName != NULL; m++)
        {
            if (!strncmp(suffixname, m->SuffixName, m->SuffixLen))
                return m;
        }
        return NULL;
    }
}

int InitHTTPServer()
{
    struct sockaddr_in sock;
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
    {
        Log("socket", LOG_ERR, "socket error:%s,errno:%d", strerror(errno), errno);
        return -1;
    }
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &sock, sizeof(sock));
    memset(&sock, '\0', sizeof(sock));
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = htonl(INADDR_ANY);
    sock.sin_port = htons(conf_para.Port);
    if (bind(serverfd, (struct sockaddr *)&sock, sizeof(sock)) == -1)
    {
        close(serverfd);
        Log("bind", LOG_ERR, "bind error:%s,errno:%d", strerror(errno), errno);
        return -1;
    }
    if (listen(serverfd, 10) == -1)
    {
        close(serverfd);
        Log("listen", LOG_ERR, "listen error:%s,errno:%d", strerror(errno), errno);
        return -1;
    }
    return serverfd;
}

void SetNonBlocking(int fd)
{
    int flag = -1;
    flag = fcntl(fd, F_GETFL);
    if (flag == -1)
    {
        Log("fcntl", LOG_ERR, "fcntl error:%s,errno:%d", strerror(errno), errno);
        exit(-1);
    }
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
    {
        Log("fcntl", LOG_ERR, "fcntl error:%s,errno:%d", strerror(errno), errno);
        exit(-1);
    }
}

void AddFd(int epollfd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        Log("epoll_ctl", LOG_ERR, "epoll_ctl error:%s,errno:%d", strerror(errno), errno);
        exit(-1);
    }
    SetNonBlocking(fd);
}

void DelFd(int epollfd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) == -1)
    {
        Log("epoll_ctl", LOG_ERR, "epoll_ctl error:%s,errno:%d", strerror(errno), errno);
        exit(-1);
    }
}

int AcceptConnect(int server_fd)
{
    struct sockaddr_in sock;
    int connfd;
    socklen_t socklen = sizeof(sock);
    memset(&sock, '\0', sizeof(struct sockaddr_in));
    while (1)
    {
        connfd = accept(server_fd, (struct sockaddr *)&sock, &socklen);
        if (connfd == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        break;
    }
    return connfd;
}

static char *ShttpdFgets(char *s, int size, FILE *in)
{
    return fgets(s, size, in);
}

static size_t ShttpdFread(char *s, int size, int count, FILE *in)
{
    return fread(s, size, count, stdin);
}

static int ServerLoop()
{
    int nchildren = 0;
    pid_t child;
    fd_set readfds;
    struct timeval timeout;
    char *remoteip[100];
    int epollfd = epoll_create(5);
    if (epollfd == -1)
    {
        close(serverfd);
        Log("epoll_create", LOG_ERR, "epoll_create error:%s,errno:%d", strerror(errno), errno);
        exit(-1);
    }
    struct epoll_event events[conf_para.MaxClients + 200];
    AddFd(epollfd, serverfd);
    while (1)
    {
        if (nchildren > conf_para.MaxClients)
            sleep(nchildren - conf_para.MaxClients);
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        FD_ZERO(&readfds);
        FD_SET(serverfd, &readfds);
        select(serverfd + 1, &readfds, NULL, NULL, &timeout);
        if (FD_ISSET(serverfd, &readfds))
        {
            struct sockaddr_in client;
            socklen_t client_len = sizeof(client);
            int connfd = AcceptConnect(serverfd);
            if (connfd == -1)
            {
                close(serverfd);
                Log("accept", LOG_ERR, "accept error:%s,errno:%d", strerror(errno), errno);
                exit(-1);
            }
            child = fork();
            if (child)
            {
                memset(remoteip, '\0', sizeof(remoteip));
                inet_ntop(AF_INET, &client.sin_addr, (char *)&remoteip, client_len);
                sRemoteAddr = ShttpdStrDup((char *)&remoteip);
                Log("RemoteAddr", LOG_INFO, "RemoterAddr is %s", sRemoteAddr);
                printf("new process:%d\n", child);
                nchildren++;
                close(connfd);
                fflush(stdout);
            }
            else if (child == 0)
            {
                close(0);
                dup(connfd);
                close(1);
                dup(connfd);
                close(connfd);
                close(serverfd);
                return 1;
            }
        }


        while ((child = waitpid(0, 0, WNOHANG)) > 0)
        {
            printf("process %d exit\n", child);
            fflush(stdout);
            nchildren--;
        }
    }
}

/*用于获取一个空格之前的字符串，并且将空格置空
LeftOver用于存储空格之后的字符串*/
static char *GetFirstElement(char *Input, char **LeftOver)
{
    char *Result = NULL;
    if (Input == 0)
    {
        if (LeftOver)
            *LeftOver = '\0';
        return 0;
    }
    /*GET /root/default.html HTTP/1.1\r\n*/
    while (isspace(*(unsigned char *)Input))
    {
        Input++;
    }
    Result = Input;
    while (*Input && !isspace(*(unsigned char *)Input))
    {
        Input++;
    }
    if (*Input)
    {
        *Input = '\0';
        Input++;
        while (isspace(*(unsigned char *)Input))
        {
            Input++;
        }
    }
    if (LeftOver)
    {
        *LeftOver = Input;
    }
    return Result;
}

static char *ShttpdMalloc(size_t size)
{
    char *p = (char *)malloc(size);
    if (p == NULL)
    {
        Log("malloc", LOG_ERR, "malloc error:%s,errno:%d", strerror(errno), errno);
        fflush(stdout);
        exit(1);
    }
}

static char *ShttpdStrDup(char *in)
{
    size_t size;
    char *out = NULL;
    if (in == NULL)
        return NULL;
    size = strlen(in) + 1;
    out = ShttpdMalloc(size);
    strcpy(out, in);
    return out;
}

static char *ShttpdStrAppend(char *dest, char *src)
{
    if (src == NULL)
        return NULL;
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    size_t total_len = dest_len + src_len + 1;
    char *new_out = ShttpdMalloc(total_len);
    strcpy(new_out, dest);
    strcpy(new_out + dest_len, src);
    return new_out;
}

static char *RFC822Date(time_t t)
{
    struct tm *tm;
    static char sDtae[100];
    tm = gmtime(&t);
    strftime(sDtae, sizeof(sDtae), "%a, %d %b %Y %H:%M:%S GMT", tm);
    return sDtae;
}

static int AddDateTag(time_t t)
{
    return shttpd_printf("%s: %s\r\n", "Date", RFC822Date(t));
}

static int AddStatusLine(const char *statucode)
{
    int n = 0;
    n += shttpd_printf("%s %s\r\n", sProtocol ? sProtocol : "HTTP/1.1", statucode);
}

static int AddConnection(const char *statucode)
{
    if (*(statucode) >= '4' || closeConnection == 1)
        return shttpd_printf("Connection: close\r\n");
    else
        return shttpd_printf("Connection: keep-alive\r\n");
}

static int AddContentType(const char *ContentType)
{
    return shttpd_printf("%s: %s\r\n", "Content-type", ContentType);
}

static int AddContent(const char *format, ...)
{
    int n = 0;
    va_list args;
    va_start(args, format);
    n += shttpd_vprintf(format, args);
    va_end(args);
    return n;
}

static int AddCRLF()
{
    return shttpd_printf("\r\n");
}

static int AddContentLength(int contentlength)
{
    return shttpd_printf("Content-length: %d\r\n", contentlength);
}

static void AddHeader(const char *statucode, const char *contentType)
{
    time_t now;
    time(&now);
    nOut += AddStatusLine(statucode);
    nOut += AddConnection(statucode);
    nOut += AddDateTag(now);
    nOut += AddContentType(contentType);
    nOut += AddServer("shttpd/v0.1");
}

static int AddServer(const char *servermsg)
{
    return shttpd_printf("Server: %s\r\n", servermsg);
}

static int AddControl(int control)
{
    return shttpd_printf("Cache-Control: max-age=%d\r\n", control);
}

static int AddETag(const char *ETag)
{
    return shttpd_printf("ETag: \"%s\"\r\n", ETag);
}

static int AddContentRange(int RangeStart, int RangeEnd, int All)
{
    return shttpd_printf("Content-Range: bytes %d-%d/%d\r\n", RangeStart, RangeEnd, All);
}

static void NotFound()
{
    AddHeader("404 Not Found", "text/html; charset=utf-8");
    nOut += AddCRLF();
    nOut += AddContent("<head><title>Not Found</title></head>\n<body><h1>Document Not Found</h1>\nThe document %s is not available on this server\n</body>\n", sRealuri);
}

static void Forbidden()
{
    AddHeader("403 Forbidden", "text/html; charset=utf-8");
    nOut += AddCRLF();
    nOut += AddContent("Access denied");
}

static void Malfunction()
{
    AddHeader("500 Server Malfunction", "text/plain; charset=utf-8");
    nOut += AddCRLF();
    nOut += AddContent("Web server malfunctioned");
}

static void ShttpdExit()
{
    fflush(stdout);
    exit(0);
}

static int montoi(char *s)
{
    int retval = -1;
    static char *ar[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    size_t i;

    for (i = 0; i < sizeof(ar) / sizeof(ar[0]); i++)
        if (!strcmp(s, ar[i]))
        {
            retval = i;
            return retval;
        }
}

static time_t ParseDate(char *date)
{
    struct tm tm;
    char mon[32];
    int sec, min, hour, mday, month, year;

    (void)memset(&tm, 0, sizeof(tm));
    sec = min = hour = mday = month = year = 0;

    if (((sscanf(date, "%d/%3s/%d %d:%d:%d",
                 &mday, mon, &year, &hour, &min, &sec) == 6) ||
         (sscanf(date, "%d %3s %d %d:%d:%d",
                 &mday, mon, &year, &hour, &min, &sec) == 6) ||
         (sscanf(date, "%*3s, %d %3s %d %d:%d:%d",
                 &mday, mon, &year, &hour, &min, &sec) == 6) ||
         (sscanf(date, "%d-%3s-%d %d:%d:%d",
                 &mday, mon, &year, &hour, &min, &sec) == 6)) &&
        (month = montoi(mon)) != -1)
    {
        tm.tm_mday = mday;
        tm.tm_mon = month;
        tm.tm_year = year;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
    }

    if (tm.tm_year > 1900)
        tm.tm_year -= 1900;
    else if (tm.tm_year < 70)
        tm.tm_year += 100;

    return (mktime(&tm));
}

static int CompareTwoETags(const char *etag1, const char *etag2)
{
    if (etag1 == NULL)
        return 0;
    if (etag1[0] == '"')
    {
        int etag2_len = strlen(etag2);
        if (!strncmp(etag1 + 1, etag2, etag2_len) && etag1[etag2_len] == '"')
            return 1;
    }
    return !strcmp(etag1, etag2);
}

static int SendFile(char *filename, struct stat *filestat)
{
    char ETag[100];
    time_t now;
    size_t filesize = filestat->st_size;
    Mime_type *mimetype = GetMimeType(filename);
    char *gzipfilename = NULL;
    const char *Encoding = NULL;
    struct stat statbuf;
    FILE *fp = NULL;
    const char *filetype = mimetype ? mimetype->Mimetype : "application/octet-stream";
    sprintf(ETag, "mt%lxms%lx", filestat->st_mtime, filestat->st_size);
    if (CompareTwoETags(sETag, ETag) ||
        (sIfModifiedSince &&
         (now = ParseDate(sIfModifiedSince)) && now >= filestat->st_mtime))
    {
        nOut += AddStatusLine("304 Not Modified");
        nOut += AddConnection("304 Not Modified");
        nOut += AddDateTag(filestat->st_mtime);
        nOut += AddContentType(filetype);
        nOut += AddServer("shttpd/v0.1");
        nOut += AddControl(maxAge);
        nOut += AddETag(ETag);
        nOut += AddCRLF();
        fflush(stdout);
        Log("304 Not Modified", LOG_INFO, "SendFile Success");
        return 1;
    }
    if (RangeEnd <= 0 && sAcceptEncoding && strstr(sAcceptEncoding, "gzip"))
    {
        gzipfilename = ShttpdStrDup(filename);
        gzipfilename = ShttpdStrAppend(gzipfilename, ".gz");
        if (access(gzipfilename, R_OK) == 0)
        {
            if (stat(gzipfilename, &statbuf) == 0)
            {
                filename = gzipfilename;
                Encoding = "gzip";
                filestat = &statbuf;
            }
        }
    }
    fp = fopen(filename, "rb");
    if (!fp)
    {
        Log("fopen error", LOG_ERR, "fopen error:%s,errno:%d", strerror(errno), errno);
        NotFound();
        return 0;
    }
    if (RangeEnd > 0 && RangeStart < filestat->st_size)
    {
        nOut += AddStatusLine("206 Partial Content");
        nOut += AddConnection("206 Partial Content");
        nOut += AddServer("shttpd/v0.1");
        if (RangeEnd >= filestat->st_size)
            RangeEnd = filestat->st_size;
        nOut += AddContentRange(RangeStart, RangeEnd, filestat->st_size);
        filestat->st_size = RangeEnd + 1 - RangeStart;
    }
    else
    {
        nOut += AddStatusLine("200 OK");
        nOut += AddConnection("200 OK");
        nOut += AddServer("shttpd/v0.1");
        RangeStart = 0;
    }
    nOut += AddDateTag(filestat->st_mtime);
    nOut += AddControl(maxAge);
    nOut += AddContentLength(filestat->st_size);
    nOut += AddETag(ETag);
    nOut += shttpd_printf("Content-type: %s%s\r\n", filetype, "; charset=utf-8");
    if (Encoding)
        nOut += shttpd_printf("Content-encoding: %s\r\n", Encoding);
    nOut += AddCRLF();
    fflush(stdout);
    sendfile(fileno(stdout), fileno(fp), (off_t *)&RangeStart, filestat->st_size);
    return 1;
}

static void SetHTTPEnv()
{
    CgiVar env;
    for (int i = 0; i < sizeof(CGIENV) / sizeof(CGIENV[0]); i++)
    {
        env = CGIENV[i];
        if (*(env.cgivalue) != NULL)
        {
            int len_cginame = strlen(env.cginame);
            int len_cgivalue = strlen(*(env.cgivalue));
            char *ptr = ShttpdMalloc(len_cginame + len_cgivalue + 1 + 1);
            sprintf(ptr, "%s=%s", env.cginame, *env.cgivalue);
            putenv(ptr);
        }
    }
}

static void CGIHandle(const char *filename, struct stat *filestat)
{
    int pipein[2], pipeout[2]; // 用于CGI和shttpd通信,in和out都是相对于子进程而言
    pid_t child;
    char buffer[1000];
    if (pipe(pipein) != 0 || pipe(pipeout) != 0)
    {
        Malfunction();
        Log("CGIHandle", LOG_ERR, "pipe(cgipipe) errno:%s,%d", strerror(errno), errno);
        ShttpdExit();
    }
    child = fork();
    if (child == 0) // 子进程
    {
        // 子进程用于执行CGI，并将CGI执行的结果返回给父进程
        dup2(pipeout[1], 1); // 重定向写端到标准输出
        dup2(pipein[0], 0);  // 重定向读端到标准输入
        for (int i = 3; close(i) == 0; i++)
            ; // 关闭多余的文件描述符
        SetHTTPEnv();
        execl(filename, filename, NULL);
        ShttpdExit();
    }
    else if (child < 0)
    {
        Malfunction();
        Log("CGIHandle", LOG_ERR, "Fork error:%s,errno", strerror, errno);
        ShttpdExit();
    }
    else // 父进程
    {
        close(pipeout[1]);
        close(pipein[0]);
        if (sPostdata)
        {
            ssize_t writen = 0;
            while (nPostlen > writen)
            {
                int n = write(pipein[1], sPostdata, nPostlen);
                if (n <= 0)
                    break;
                writen += n;
            }
            free(sPostdata);
            nPostlen = 0;
        }
        while (read(pipeout[0], buffer, sizeof(buffer)) > 0)
            shttpd_printf(buffer);
        close(pipeout[0]);
        close(pipein[1]);
        wait(NULL);
        ShttpdExit();
    }
}

void ProcessRequest()
{
    char sLine[10000];
    char *tmp;
    char *content = NULL;
    char *c = NULL;

    /*读取第一行*/
    /*GET /root/default.html HTTP/1.1\r\n]*/
    if (ShttpdFgets(sLine, sizeof(sLine), stdin) == NULL)
    {
        ShttpdExit();
    }
    nIn += strlen(sLine);
    sMethod = ShttpdStrDup(GetFirstElement(sLine, &tmp));
    sRequestURI = sRealuri = ShttpdStrDup(GetFirstElement(tmp, &tmp));
    sProtocol = ShttpdStrDup(GetFirstElement(tmp, &tmp));
    if (sProtocol == NULL || strncmp(sProtocol, "HTTP/", 5) || strlen(sProtocol) != 8 || nIn > 9990)
    {
        sProtocol = NULL;
        if (nIn <= 9990)
        {
            content = "This server does not understand the request protocol\r\n";
            AddHeader("400 Bad Request", "text/plain; charset=utf-8");
            nOut += AddCRLF();
            nOut += AddContent(content);
            Log("400 Bad Request", LOG_INFO, "unknow protocol");
            ShttpdExit();
        }
        else
        {
            content = "URI is too long\r\n";
            AddHeader("414 URI Too Long", "text/plain; charset=utf-8");
            nOut += AddCRLF();
            nOut += AddContent(content);
            Log("414 URI Too Long", LOG_INFO, "URI is too long");
            ShttpdExit();
        }
    }
    if (strncmp(sMethod, "GET", 3) && strncmp(sMethod, "POST", 4) && strncmp(sMethod, "HEAD", 4))
    {
        AddHeader("501 Not Implemented", "text/plain; charset=utf-8");
        nOut += AddCRLF();
        nOut += AddContent("The %s method is not implemented on this server.\r\n", sMethod);
        Log("501 Not Implemented", LOG_INFO, "method is not implemented");
        ShttpdExit();
    }
    if (sRealuri[0] != '/')
    {
        NotFound();
        Log("404 Not Found", LOG_INFO, "The document %s is not found", sRealuri);
        ShttpdExit();
    }
    for (int i = 0; i < sizeof(DangerStr) / sizeof(DangerStr[0]); i++)
    {
        if (strstr(sRealuri, DangerStr[i]))
        {
            Forbidden();
            Log("403 Forbidden", LOG_ERR, "There is some dangerous string in request uri:%s", sRealuri);
            ShttpdExit();
        }
    }
    while (*(sRealuri) == '/')
        sRealuri++;
    if (sProtocol[5] < 1 || sProtocol[7] < 1)
        closeConnection = 1;
    while (ShttpdFgets(sLine, sizeof(sLine), stdin))
    {
        char *fieldname = NULL;
        char *fieldvalue = NULL;
        nIn += strlen(sLine);
        fieldname = GetFirstElement(sLine, &fieldvalue);
        if (fieldname == NULL || *(fieldvalue) == '\0')
            break;
        if (!strcasecmp(fieldname, "Cookie:"))
            sCookie = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "User-Agent:"))
            sAgent = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Host:"))
            sHost = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Accept:"))
            sAccept = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Accept-Encoding:"))
            sAcceptEncoding = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Content-type:"))
            sContenttype = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Content-length:"))
            sContentlength = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Referer:"))
            sReferer = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Connection:"))
        {
            sConnection = ShttpdStrDup(fieldvalue);
            if (!strcasecmp(sConnection, "keep-alive"))
                closeConnection = 0;
            else if (!strcasecmp(sConnection, "close"))
                closeConnection = 1;
        }
        else if (!strcasecmp(fieldname, "If-None-Match:"))
            sIfNoneMatch = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "If-Modified-Since:"))
            sIfModifiedSince = ShttpdStrDup(fieldvalue);
        else if (!strcasecmp(fieldname, "Range:") && !strcmp(sMethod, "GET"))
        {
            int n = 0, start = 0, end = 0;
            n = sscanf(fieldvalue, "bytes=%d-%d", &start, &end);
            if (n == 2 && end > start && start >= 0)
            {
                RangeStart = start;
                RangeEnd = end;
            }
            else if (n == 1 && start > 0)
            {
                RangeStart = start;
                RangeEnd = 0x7fffffff;
            }
        }
    }
    sDocumentRoot = ShttpdStrDup((char *)(&conf_para.DocumentRoot));
    sSecretDirectory = ShttpdStrDup((char *)(&conf_para.CGIPath));
    if (*sRealuri == '\0')
        sRealuri = conf_para.DefaultFile;
    else
    {
        c = sRealuri;
        while (*c != '?' && *c != '\0')
            c++;
        if (*c == '?')
        {
            sGetqueryString = ShttpdStrDup(c + 1);
            *c = '\0';
        }
        else
            sGetqueryString = NULL;
    }

    if (strstr(sRealuri, "cgi"))
    {
        isCGIRequest = 1;
        chdir(conf_para.CGIPath);
    }
    else
        chdir(conf_para.DocumentRoot);
    targetfile = sRealuri;

    if (!strcasecmp(sMethod, "POST") && sContentlength != NULL)
    {
        int contentlength = atoi(sContentlength);
        if (contentlength > MAX_POST_LENGTH)
        {
            AddHeader("500 Request too large", "Content-type: text/plain; charset=utf-8");
            nOut += AddCRLF();
            nOut += AddContent("Too much POST data\r\n");
            Log("500 Request too large", LOG_INFO, "Too much POST data");
            ShttpdExit();
        }
        else
        {
            sPostdata = ShttpdMalloc(contentlength + 1);
            nPostlen = ShttpdFread(sPostdata, 1, contentlength, stdin);
            nIn += nPostlen;
        }
    }
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    Log("CWD", LOG_DEBUG, "Currenr workdir is %s", cwd);
    Log("targetfile", LOG_INFO, "HTTP URI:%s%s", conf_para.DocumentRoot, targetfile);
    struct stat file_stat;
    if (access(targetfile, F_OK) == 0)
    {
        Log("TARGETFILE", LOG_INFO, "The target file is %s", targetfile);
        stat(targetfile, &file_stat);
        if (S_ISDIR(file_stat.st_mode))
        {
            NotFound();
            Log("404 NOT FOUND", LOG_INFO, "The target file :%s is a directory", targetfile);
            ShttpdExit();
        }
        if (S_ISREG(file_stat.st_mode))
        {
            if (!isCGIRequest)
                SendFile(targetfile, &file_stat);
            else
                CGIHandle(targetfile, &file_stat);
        }
    }
    else
    {
        NotFound();
        Log("404 Not Found", LOG_INFO, "The document %s is not found", sRealuri);
        ShttpdExit();
    }
}

int main(int argc, char *argv[])
{
    Para_Init(argc, argv);
    serverfd = InitHTTPServer();
    if (serverfd == -1)
    {
        printf("InitHTTPServer error\n");
        exit(-1);
    }
    ServerLoop();
    ProcessRequest();
    ShttpdExit();
}
