#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED

#define SOCK_PATH "/var/run/fcgi.sock"
#define LOGFILE "/var/log/fcgi.log"
#define PIDFILE "/var/run/fcgi.pid"
#define SOCK_MASK 0000666
#define DEFLOCALE "en_US.UTF-8"
#define FCGX_BACKLOG_SIZE 10000
#define CHDIR_DIR "/"
#define FORK_UMASK 002

#define THREAD_COUNT 1

#include <pthread.h>

typedef struct main_options
{
    char *sock;
    char *pid;
    char *logf;
    char *locale;
    char *sockmask;
    int thr_c;
    int d_mode;

} s_opt_t;


typedef struct arg
{
    const char *name;
    const char *value;
} s_arg_t;

typedef struct query
{
    CURL *curl;
    void *tree;
    int thr_id; 
    FCGX_Request request;

} s_q_t;

extern FILE *g_log_fd;
extern pthread_mutex_t g_log_mutex;
extern int g_sock;
extern char* g_logf;
extern int g_thr_c;

#endif