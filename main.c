#include "main.h"

extern int errno;
pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *g_log_fd;
int g_sock;
char* g_logf;
int g_thr_c;

// /query1?arg1=xxxx&arg2=yyyyy&arg3=zzzzzz - returning arg2 reversed
// /query2?arg1=xxxx&arg2=yyyyy&arg3=zzzzzz - returning arg1(uppercase)+" "+arg3(lowercase)
// /query3?arg1=xxxx&arg2=yyyyy&arg3=zzzzzz - returning YES if arg3 сontains test123 case insesitive or NO in other cases

int main(int argc, char *argv[])
{
    s_opt_t *opt = parse_args(argc, argv);
    set_globs(opt);

    if(FCGX_Init())
    {
        log_msg(0, "FCGX_Init error");
        exit(9);
    }

    log_msg(0, "FCGI daemon started");

    int i;
    pthread_t id[g_thr_c];
    for (i = 0; i < g_thr_c; i++)
    {
        int thr_id = i + 1;
        void *arg = malloc(sizeof(int));
        if (arg)
            memcpy(arg, &thr_id, sizeof(int));

        pthread_create(&id[i], NULL, serve, arg);
    }

    for (i = 0; i < g_thr_c; i++ )
    {
        pthread_join(id[i], NULL);
    }

    log_msg(0, "FCGI deamon stopped");
    fclose(g_log_fd);
    return 0;
}


