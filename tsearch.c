#include "tsearch.h"

int cmp(const void *a, const void *b)
{
    s_arg_t *fa, *fb;

    fa = (s_arg_t *)a;
    fb = (s_arg_t *)b;
    if (fa->name && fb->name)
    {
        return strcmp(fa->name, fb->name);
    }
    return 0;
}

void free_func(void *my_tree)
{

    free(my_tree);
    return;
}

void delwalk(const void *node, VISIT v, int __unused0)
{
    if (v == postorder || v == leaf)
    {
        if ((*(s_arg_t **)node)->name)
            curl_free( (void *)(*(s_arg_t **)node)->name );
        if ((*(s_arg_t **)node)->value)
            curl_free( (void *)(*(s_arg_t **)node)->value );
    }
}

void printwalk(const void *node, VISIT v, int __unused0)
{
    if (v == postorder || v == leaf)
    {
        log_msg(0, (*(s_arg_t **)node)->name);
        log_msg(0, (*(s_arg_t **)node)->value);
    }
    fflush(g_log_fd);
}

s_arg_t *find_and_decode_arg(const char *argtofind, void *tree, int (*cmp)(const void *a, const void *b), CURL *curl)
{
    s_arg_t *ret, *temp, buf;
    temp = (struct arg *)&buf;
    temp->name = argtofind;
    ret = tfind(temp, &tree, cmp);

    if (ret)
    {
        void *ptr_to_free1 = (void *)(*(s_arg_t **)ret)->name;
        // (*(s_arg_t **)ret)->name = g_uri_unescape_string((*(s_arg_t **)ret)->name, NULL);
        (*(s_arg_t **)ret)->name = curl_easy_unescape(curl, (*(s_arg_t **)ret)->name, 0, NULL);
        free(ptr_to_free1);

        void *ptr_to_free2 = (void *)(*(s_arg_t **)ret)->value;
        // (*(s_arg_t **)ret)->value = g_uri_unescape_string((*(s_arg_t **)ret)->value, NULL);
        (*(s_arg_t **)ret)->value = curl_easy_unescape(curl, (*(s_arg_t **)ret)->value, 0, NULL);

        free(ptr_to_free2); 

    } else {
        log_msg(0, "unescape error");
    }
    return ret;
}