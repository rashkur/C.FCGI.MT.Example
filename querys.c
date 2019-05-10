#include "querys.h"
#include "tsearch.h"

void *requestRouter[][2] =
{
    {"/query1",   query1  },
    {"/query2",   query2  },
    {"/query3",   query3  },
};


void *serve(void *arg)
{
    int thr_id = -1;
    if (arg)
    {
        thr_id = *(int *)arg;
        free(arg);
    }

    FCGX_Request request;

    if (FCGX_InitRequest(&request, g_sock, 0) != 0 )
    {
        log_msg(thr_id, "FCGX_InitRequest error");
        exit(2);
    }

    log_msg(thr_id, "thread started");

    for (;;)
    {
        int rc = FCGX_Accept_r(&request);
        if (rc < 0)
            break;

        char *request_method = FCGX_GetParam("REQUEST_METHOD", request.envp);

        if (!request_method)
        {
            log_msg(thr_id, "REQUEST_METHOD error\n");
            continue;
        }

        char *query_string = FCGX_GetParam("QUERY_STRING", request.envp);

        if (!query_string)
        {
            log_msg(thr_id, "QUERY_STRING error\n");
            continue;
        }

        const char *document_uri = FCGX_GetParam("DOCUMENT_URI", request.envp);

        if (!document_uri)
        {
            log_msg(thr_id, "DOCUMENT_URI error\n");
            continue;
        }

        // checking request method
        if (strcmp(request_method, "GET") != 0)
        {
            FCGX_PutS("Content-Type: text/plain\r\n\r\n", request.out);
            FCGX_PutS("Can't process query type:", request.out);
            FCGX_PutS(request_method, request.out);

            FCGX_Finish_r(&request);
            continue;
        }

        // search for params in query string and adds them to the tree
        const char amp[] = "&";
        const char equal[] = "=";
        char *qs, *token, *subtoken;
        char *saveptr1;
        int i, tok_len, val_len, name_len;
        void *tree = NULL;

        for (i = 1, val_len = 0, qs = query_string; ; i++, qs = NULL)
        {
            s_arg_t *val;
            token = strtok_r(qs, amp, &saveptr1);
            if (!token)
                break;

            tok_len = strlen(token);
            subtoken = strpbrk(token, equal);
            val_len = subtoken ? strlen(subtoken) : 0;
            name_len = tok_len - val_len;

            if (name_len <= 0) continue;

            val = malloc(sizeof(s_arg_t));

            // > "="
            if (val_len > 1)
            {
                val_len = strlen(subtoken);
                val->value = strdup(subtoken + 1);
            }
            else
            {
                val->value = NULL;
            }

            val->name = strndup(token, name_len);

            // for duplicate arguments - replace to latest
            s_arg_t *dup_check = tfind(val, &tree, cmp);
            if (dup_check)
            {
                void *ptr_to_free1 = (void *)(*(s_arg_t **)dup_check)->name;
                void *ptr_to_free2 = (void *)(*(s_arg_t **)dup_check)->value;
                void *ptr_to_free3 = val;

                (*(s_arg_t **)dup_check)->name = val->name;
                (*(s_arg_t **)dup_check)->value = val->value;

                free(ptr_to_free1);
                free(ptr_to_free2);
                free(ptr_to_free3);

            } else {
                tsearch(val, &tree, cmp);
            }

        }

        CURL *curl = curl_easy_init();

        //request router
        void (*HandleQuery)(s_q_t *) = NULL;
        int reqIndex;
        for (reqIndex = 0 ; reqIndex < sizeof(requestRouter) / sizeof(requestRouter[0]) ; reqIndex++)
        {
            if (strcmp(requestRouter[reqIndex][0], document_uri) == 0)
            {
                HandleQuery = requestRouter[reqIndex][1];
            }
        }
        if (!HandleQuery)
        {
            FCGX_PutS("Status: 404 Not found\r\n\r\n", request.out);
            FCGX_Finish_r(&request);
            continue;
        }

        s_q_t q;
        q.curl = curl;
        q.tree = tree;
        q.thr_id = thr_id;
        q.request = request;

        HandleQuery(&q);

        curl_easy_cleanup(curl);
        twalk(tree, delwalk);
        tdestroy(tree, free_func);

        FCGX_Finish_r(&request);

        // malloc_stats();
        // malloc_info(0, stdout);

        // TODO: call trim every 3 min
        // malloc_trim(0);
    }

    return NULL;
}

void query1(struct query *q)
{
    s_arg_t *arg2;
    arg2 = find_and_decode_arg("arg2", q->tree, cmp, q->curl);

    if (arg2)
    {
        char *arg2_reverse = mb_reverse((*(s_arg_t **)arg2)->value);
        if (!arg2_reverse)
        {
            log_msg(q->thr_id, "arg2_reverse error");
            FCGX_PutS("Status: 500 Internal Server Error\r\n\r\n", q->request.out);
            return;
        }

        FCGX_PutS("Content-Type: text/plain\r\n\r\n", q->request.out);
        FCGX_PutS(arg2_reverse, q->request.out);

        g_free(arg2_reverse);
        return;

    }
    else    //not enough args
    {
        FCGX_PutS("Status: 400 Bad Request\r\n\r\n", q->request.out);
        return;
    }
}


void query2(struct query *q)
{
    s_arg_t *arg1, *arg3;
    arg1 = find_and_decode_arg("arg1", q->tree, cmp, q->curl);
    arg3 = find_and_decode_arg("arg3", q->tree, cmp, q->curl);

    if (arg1 && arg3)
    {

        char *arg1_up = str_toup((*(s_arg_t **)arg1)->value);
        // log_msg(thr_id, arg1_up);
        if (!arg1_up)
        {
            log_msg(q->thr_id, "query1 str_toup error.");
            FCGX_PutS("Status: 500 Internal Server Error\r\n\r\n", q->request.out);
            return;
        }

        char *arg3_low = str_tolow((*(s_arg_t **)arg3)->value);
        if (!arg3_low)
        {
            log_msg(q->thr_id, "query1 str_tolow error.");
            FCGX_PutS("Status: 500 Internal Server Error\r\n\r\n", q->request.out);

            g_free(arg1_up);
            return;
        }

        FCGX_PutS("Content-Type: text/plain\r\n\r\n", q->request.out);
        FCGX_PutS(arg1_up, q->request.out);
        FCGX_PutS(" ", q->request.out);
        FCGX_PutS(arg3_low, q->request.out);

        // twalk(tree, printwalk);
        g_free(arg1_up);
        g_free(arg3_low);
        return;

    }
    else    //not enough args
    {
        FCGX_PutS("Status: 400 Bad Request\r\n\r\n", q->request.out);
        return;

    }
}

void query3(struct query *q)
{
    s_arg_t *arg3;
    arg3 = find_and_decode_arg("arg3", q->tree, cmp, q->curl);

    if (arg3)
    {
        char *ans = "Content-Type: text/plain\r\n\r\nYES\r\n";

        char *arg3_low = str_tolow((*(s_arg_t **)arg3)->value);
        if (!arg3_low)
        {
            log_msg(q->thr_id, "query3 str_tolow error.");
            FCGX_PutS("Status: 500 Internal Server Error\r\n\r\n", q->request.out);
            return;
        }

        if(!strstr(arg3_low, "test123"))
        {
            ans = "Content-Type: text/plain\r\n\r\nNO\r\n";
        }

        FCGX_PutS(ans, q->request.out);

        g_free(arg3_low);
        return;

    }
    else //not enough args
    {
        FCGX_PutS("Status: 400 Bad Request\r\n\r\n", q->request.out);
        return;
    }
}
