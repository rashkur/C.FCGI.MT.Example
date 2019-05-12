#include "helpers.h"

char *mb_reverse(const char *str)
{
    if (g_utf8_validate(str, g_utf8_strlen(str, -1), NULL) != TRUE)
    {
        return NULL;
    }

    char *temp = g_utf8_strreverse(str, g_utf8_strlen(str, -1));
    return temp;
}

char *str_toup(const char *str)
{
    char *temp = g_utf8_strup(str, strlen(str));
    return temp;
}

char *str_tolow(const char *str)
{
    char *temp = g_utf8_strdown(str, strlen(str));
    return temp;
}

void createpidf(const char *pid)
{
    errno = 0;
    FILE *pidfile = fopen(pid, "w");
    if (pidfile < 0 || errno)
    {
        log_msg(0, "Opening pidfile error");
        log_msg(0, strerror(errno));
        exit(1);
    }

    int p_pid = getpid();
    fprintf(pidfile, "%d", p_pid);
    fflush(pidfile);
    fclose(pidfile);
}

void daemon_mode()
{
        errno = 0;
        int master_pid = getpid();
        pid_t new_pid = fork();
        
        if (new_pid < 0)
        {
            log_msg(0, "daemon fork error");
            if (errno) {
                log_msg(0, strerror(errno));
            }
            exit(1);
        }
        
        if (getpid() == master_pid) 
        {
            exit(0);
        }
        
        umask(FORK_UMASK);

        // get rid of master terminal
        pid_t new_sid = setsid();
        
        if (new_sid < 0)
        {
            log_msg(0, "setsid error");
            if (errno) log_msg(0, strerror(errno)); 
            exit(2);
        }

        if (chdir(CHDIR_DIR) < 0)
        {
            log_msg(0, "chdir error");
            if (errno) log_msg(0, strerror(errno));
            exit(3);
        }

        log_msg(0, "postcdir");

        //close master process descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
}

void set_globs(s_opt_t *opt)
{
    errno = 0;
    mode_t l_sockmask;
    char *l_locale;
    char *l_pid;
    char *sockpath;

    if (opt && opt->sock)
    {
        g_sock = FCGX_OpenSocket(opt->sock, FCGX_BACKLOG_SIZE);
        sockpath = opt->sock;
    }
    else
    {
        g_sock = FCGX_OpenSocket(SOCK_PATH, FCGX_BACKLOG_SIZE);
        sockpath = SOCK_PATH;
    }
    if (g_sock < 0)
    {
        log_msg(0, "FCGX_OpenSocket error");
        if (errno)
            log_msg(0, strerror(errno));
        exit(1);
    }
    else
    {
        // fix for FCGX_OpenSocket No such file or directory
        errno = 0;
    }

    if (opt && opt->sockmask)
    {
        enum
        {
            // all permission bits, equal to 07777
            PERMBITS = S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX,
        };

        char *endptr;
        mode_t mode;
        unsigned long newmode;
        newmode = strtoul(opt->sockmask, &endptr, 8);

        if(errno)
        {
            log_msg(0, "sockmask error");
            log_msg(0, strerror(errno));
            exit(2);
        }

        mode &= ~PERMBITS;
        mode |= newmode;

        l_sockmask = mode;
    }
    else
    {
        l_sockmask = SOCK_MASK;
    }

    if (chmod(sockpath, l_sockmask) != 0 || errno)
    {
        log_msg(0, "Socket chmod error");
        log_msg(0, strerror(errno));
        exit(3);
    }

    if (opt && opt->logf)
    {
        g_log_fd = fopen(opt->logf, "a+");
    }
    else
    {
        g_log_fd = fopen(LOGFILE, "a+");
    }
    if (!g_log_fd || errno)
    {
        log_msg(0, "error opening log file");
        log_msg(0, strerror(errno));
        exit(4);
    }

    if (opt && opt->locale)
    {
        l_locale = opt->locale;
    }
    else
    {
        l_locale = DEFLOCALE;
    }
    if (!l_locale)
    {
        log_msg(0, "error setting locale");
        if (errno)
            log_msg(0, strerror(errno));
        exit(5);
    }
    if (strcmp(setlocale(LC_CTYPE, l_locale), l_locale) != 0 )
    {
        log_msg(0, "Locale setup error");
        exit(6);
    }

    if (opt && opt->d_mode) {
        daemon_mode();
    }

    if (opt && opt->thr_c)
    {
        g_thr_c = opt->thr_c;
    }
    else
    {
        g_thr_c = THREAD_COUNT;
    }
    if (!g_thr_c)
    {
        log_msg(0, "error setting threads count");
        exit(7);
    }

    if (opt && opt->pid)
    {
        l_pid = opt->pid;
    }
    else
    {
        l_pid = PIDFILE;
    }

    if (!l_pid)
    {
        log_msg(0, "error opening pid file");
        exit(8);
    }
    createpidf(l_pid);
}

void log_msg(int thr_id, const char *msg)
{
    pthread_mutex_lock(&g_log_mutex);
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[100];

    strftime(s, sizeof(s), "%Y/%m/%d %H:%M:%S", tm);
    
    if (g_log_fd) {
        fprintf(g_log_fd, "[%s] thread %d:%s\n", s, thr_id, msg);
        fflush(g_log_fd);
    } else {
        printf("[%s] thread %d:%s\n", s, thr_id, msg);
    }

    pthread_mutex_unlock(&g_log_mutex);
}

s_opt_t *parse_args(int argc, char *argv[])
{
    errno = 0;

    s_opt_t *opt = calloc(1, sizeof(s_opt_t));

    if (!opt)
    {
        log_msg(0, "parse_main calloc error");
        exit(1);
    }

    int i, o = 0;
    while ( (i = getopt(argc, argv, "s:p:c:w:m:l:d")) != -1 )
    {
        switch(i)
        {
        case 's':
            opt->sock = strdup(optarg);
            if(!(*opt).sock)
            {
                log_msg(0, "parse_main sock malloc error");
                exit(1);
            }
            break;
        case 'p':
            opt->pid = strdup(optarg);
            if(!(*opt).pid)
            {
                log_msg(0, "parse_main pid malloc error");
                exit(1);
            }
            break;
        case 'c':
            opt->locale = strdup(optarg);
            if(!(*opt).locale)
            {
                log_msg(0, "parse_main pid malloc error");
                exit(1);
            }
            break;
        case 'w':
            opt->thr_c = atoi(optarg);
            break;
        case 'm':
            opt->sockmask = strdup(optarg);
            break;
        case 'd':
            opt->d_mode = 1;
            break;
        case 'l':
            opt->logf = strdup(optarg);
            if(!(*opt).logf)
            {
                log_msg(0, "parse_main logfile_name malloc error");
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "%s\n", "To change default settings: -s (listen socket) -w (workers) -d (daemon mode) -l (logfile) -p (pid file) -m (socket mask 0000666) -c (locale)");
            exit(1);
        }
        o++;
    }

    if (o)
    {
        return opt;
    }
    else
    {
        free(opt);
        return NULL;
    }
}