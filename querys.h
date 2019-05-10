#ifndef QUERYS_INCLUDED
#define QUERYS_INCLUDED

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <curl/curl.h>
#include <string.h> //strcmp

#include "tsearch.h"
#include "helpers.h"

void *serve(void *);
void *requestRouter[3][2];
void query1(struct query *q);
void query2(struct query *q);
void query3(struct query *q);

#endif
