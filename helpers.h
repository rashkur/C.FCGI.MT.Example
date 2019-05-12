#ifndef HELPERS_INCLUDED
#define HELPERS_INCLUDED

#include <glib.h>
#include <fcgi_config.h>
#include <fcgiapp.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h> //date and time
#include <curl/curl.h> 

#include <pthread.h>
#include <fcntl.h> 
#include <getopt.h>
#include <locale.h>
#include <errno.h>

#include <unistd.h>
#include <sys/stat.h>

#include "config.h"

char *mb_reverse(const char *);
char *str_toup(const char *);
char *str_tolow(const char *);
void createpidf(const char*);

// FILE *open_log(const char *logfile);
void set_globs(s_opt_t*);
void log_msg(const int, const char *);
s_opt_t* parse_args(int, char*[]);

#endif