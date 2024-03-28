#ifndef __SHTTPD_H
#define __SHTTPD_H

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

typedef struct conf_opts
{
    char CGIPath[128];      // CGI文件路径
    char DocumentRoot[128]; // web根目录
    char DefaultFile[128];
    uint32_t Port;       // 监听端口号
    uint32_t TimeOut;    // 超时时间
    uint32_t MaxClients; // 最大客户端
} conf_opts;

static char *DangerStr[] = {"../", "..", "./."};

typedef enum MimeTypes
{
    MINET_HTML,
    MINET_HTM,
    MINET_TXT,
    MINET_CSS,
    MINET_ICO,
    MINET_GIF,
    MINET_JPG,
    MINET_JPEG,
    MINET_PNG,
    MINET_SVG,
    MINET_TORRENT,
    MINET_WAV,
    MINET_MP3,
    MINET_MID,
    MINET_M3U,
    MINET_RAM,
    MINET_RA,
    MINET_DOC,
    MINET_EXE,
    MINET_ZIP,
    MINET_XLS,
    MINET_TGZ,
    MINET_TARGZ,
    MINET_TAR,
    MINET_GZ,
    MINET_ARJ,
    MINET_RAR,
    MINET_RTF,
    MINET_PDF,
    MINET_SWF,
    MINET_MPG,
    MINET_MPEG,
    MINET_ASF,
    MINET_AVI,
    MINET_BMP
} MineTypes;

typedef struct Mime_type
{
    const char *SuffixName;
    MineTypes Type;
    int SuffixLen;
    const char *Mimetype;
} Mime_type;

typedef struct CgiVar
{
    char *cginame;
    char **cgivalue;
}CgiVar;

#define shttpd_printf printf
#define shttpd_vprintf vprintf
#define MAX_POST_LENGTH 10000
static int nIn = 0;
static int nOut = 0;

static int closeConnection = 0;
static int nPostlen = 0;
static char *sMethod = NULL;
static char *sRealuri = NULL;
static char *sProtocol = NULL;
static char *sCookie = NULL;
static char *sHost = NULL;
static char *sAgent = NULL;
static char *sAccept = NULL;
static char *sAcceptEncoding = NULL;
static char *sContentlength = NULL;
static char *sContenttype = NULL;
static char *sConnection = NULL;
static char *sReferer = NULL;
static char *sIfNoneMatch = NULL;
static char *sIfModifiedSince = NULL;
static char *sETag = NULL;
static char *sRemoteAddr=NULL;
static char *sRequestURI=NULL;
static char *sDocumentRoot=NULL;
static char *sSecretDirectory=NULL;
static int RangeStart;
static int RangeEnd;
static int maxAge=300;
static int isCGIRequest=0;

static char *targetfile = NULL;
static char *sGetqueryString = NULL;
static char *sPostdata = NULL;

int InitHTTPServer();
void SetNonBlocking(int fd);
int AcceptConnect(int server_fd);
void AddFd(int epollfd, int fd);
void DelFd(int epollfd, int fd);
static int ServerLoop();
static char *ShttpdFgets(char *s, int size, FILE *in);
static size_t ShttpdFread(char *s, int size, int count, FILE *in);
void ProcessRequest();
static char *GetFirstElement(char *Input, char **LeftOver);
static char *ShttpdMalloc(size_t size);
static char *ShttpdStrDup(char *in);
static char *ShttpdStrAppend(char *dest, char *src);
static char *RFC822Date(time_t t);
static int AddDateTag(time_t t);
static int AddStatusLine(const char *statucode);
static int AddConnection(const char *statucode);
static int AddContentType(const char *ContentType);
static int AddContent(const char *format, ...);
static int AddServer(const char *servermsg);
static int AddContentLength(int contentlength);
static int AddCRLF();
static int AddControl(int control);
static int AddETag(const char *ETag);
static int AddContentRange(int RangeStart,int RangeEnd,int All);
static void AddHeader(const char *statucode, const char *contentType);
static void NotFound();
static void Forbidden();
static void Malfunction();
static void ShttpdExit();
static Mime_type *GetMimeType(const char *filename);
static int SendFile(char *filename, struct stat *filestat);
static void CGIHandle(const char *filename,struct stat *filestat);
static int montoi(char *s);
static time_t ParseDate(char *date);
static int CompareTwoETags(const char *etag1, const char *etag2);
static void SetHTTPEnv(); 

#endif