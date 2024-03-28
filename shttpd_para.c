#include "shttpd.h"
#include "shttpd_para.h"

char *l_opt_arg;
char *short_opts = "c:d:f:p:t:m:h";

struct option long_option[] = {
    {"cgipath", required_argument, NULL, 'c'},
    {"documentroot", required_argument, NULL, 'd'},
    {"defaultfile", required_argument, NULL, 'f'},
    {"port", required_argument, NULL, 'p'},
    {"timeout", required_argument, NULL, 't'},
    {"maxclients", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'}};

extern conf_opts conf_para;

void display_usage(void)
{
    printf("SHTTPD -c cgipath -d documentroot -f defaultfile -p port -t -m maxclients timeout -h\n");
    printf("SHTTPD --cgipath cgipath\n");
    printf("       --documentroot defaultfile\n");
    printf("       --port port\n");
    printf("       --timeout timeout\n");
    printf("       --maxclients maxclients\n");
    printf("       --help\n");
}

void display_para()
{
    printf("SHTTPD  CGIPath:%s\n", conf_para.CGIPath);
    printf("        DocumentRoot:%s\n", conf_para.DocumentRoot);
    printf("        DefaultFile:%s\n", conf_para.DefaultFile);
    printf("        Port:%d\n", conf_para.Port);
    printf("        TimeOut:%d\n", conf_para.TimeOut);
    printf("        MaxClients:%d\n", conf_para.MaxClients);
}

void Para_CmdParse(int argc, char *argv[])
{
    int opt;
    int len;
    int value;
    while ((opt = getopt_long(argc, argv, short_opts, long_option, NULL)) != -1)
    {
        switch (opt)
        {
        case 'c':
            l_opt_arg = optarg;
            len = strlen(optarg) >= sizeof(conf_para.CGIPath) ? sizeof(conf_para.CGIPath) : strlen(optarg);
            if (access(optarg, F_OK) == 0)
                strncpy(conf_para.CGIPath, optarg, len);
            else
                puts("New cgipath does not exist!");
            break;
        case 'd':
            l_opt_arg = optarg;
            len = strlen(optarg) >= sizeof(conf_para.DocumentRoot) ? sizeof(conf_para.DocumentRoot) : strlen(optarg);
            if (access(optarg, R_OK) == 0)
            {
                memset(conf_para.DocumentRoot,'\0',sizeof(conf_para.DocumentRoot));
                strncpy(conf_para.DocumentRoot, optarg, len);
            }
            else
                puts("New DocumentRoot can not read!");
            break;
        case 'f':
            l_opt_arg = optarg;
            len = strlen(optarg) >= sizeof(conf_para.DefaultFile) ? sizeof(conf_para.DefaultFile) : strlen(optarg);
            if (access(optarg, R_OK) == 0)
            {
                memset(conf_para.DefaultFile,'\0',sizeof(conf_para.DefaultFile));
                strncpy(conf_para.DefaultFile, optarg, len);
            }
            else
                puts("New DefaultFile can not read!");
            break;
        case 'p':
            l_opt_arg = optarg;
            value = strtol(l_opt_arg, NULL, 10);
            if (value > 0 && value < USHRT_MAX)
                conf_para.Port = value;
            else
                printf("port set error\n");
            break;
        case 't':
            l_opt_arg = optarg;
            value = strtol(l_opt_arg, NULL, 10);
            conf_para.TimeOut = value;
            break;
        case 'm':
            l_opt_arg = optarg;
            value = strtol(l_opt_arg, NULL, 10);
            conf_para.MaxClients = value;
            break;
        case 'h':
            display_usage();
            exit(0);
            break;
        default:
            break;
        }
    }
}

void Para_Init(int argc, char *argv[])
{
    Para_CmdParse(argc, argv);
    display_para();
}