#ifndef TSEARCH_INCLUDED
#define TSEARCH_INCLUDED

#define __USE_GNU 1
#include <search.h>
#include <curl/curl.h>

#include <fcgi_config.h>
#include <fcgiapp.h>

#include <string.h> //strcmp
#include <stdlib.h> //exit, malloc, free

#include "config.h"
#include "querys.h"

int cmp(const void *a, const void *b);
void free_func(void *my_tree);
void delwalk(const void *node, VISIT v, int __unused0);
void printwalk(const void *node, VISIT v, int __unused0);
s_arg_t *find_and_decode_arg(const char *argtofind, void *tree, int (*cmp)(const void *a, const void *b), CURL *curl);

#endif