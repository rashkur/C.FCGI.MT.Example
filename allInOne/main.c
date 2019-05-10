#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <execinfo.h>
#include <signal.h>

#include <fcgi_config.h>
#include <fcgiapp.h>

#include <pthread.h>

#include <curl/curl.h>

// ----------------------------------------------------------------------------
//
// /query1?arg1=xxxx&arg2=yyyyy&arg3=zzzzzz - returning arg2 reversed
// /query2?arg1=xxxx&arg2=yyyyy&arg3=zzzzzz - returning arg1(uppercase)+" "+arg3(lowercase)
// /query3?arg1=xxxx&arg2=yyyyy&arg3=zzzzzz - returning YES if arg3 сontains test123 case insesitive or NO in other cases
//
// ----------------------------------------------------------------------------

char *g_socket_str = NULL, *g_logfile_name = NULL;
int g_worker_count = 1;
int g_daemon_mode = 0;

int g_socket_id;
pthread_mutex_t g_mutex_for_log = PTHREAD_MUTEX_INITIALIZER;

// ----------------------------------------------------------------------------

inline void str_reverse(char *str) {
  size_t l, i;
  char t;

  l = strlen(str);
  for (i = 0; i < l / 2; i++) {
    t = str[i];
    str[i] = str[l - i - 1];
    str[l - i - 1] = t;
  }
}

inline void str_tolower(char *str) {
  size_t l, i;

  l = strlen(str);
  for (i = 0; i < l; i++)
    str[i] = tolower(str[i]);
}

inline void str_toupper(char *str) {
  size_t l, i;

  l = strlen(str);
  for (i = 0; i < l; i++)
    str[i] = toupper(str[i]);
}

// ----------------------------------------------------------------------------

void parse_uri_params(const char *src, char **arg1, char **arg2, char **arg3) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Curl error!\n");
    return;
  }

  const char *a = src;
  const char *b;
  do {
    b = strchr(a, '&');
    int l = b ? (b - a) : strlen(a);
    if (l > 0) {
      char *c = (char *)malloc(l + 1);
      if (c) {
        memcpy(c, a, l);
        c[l] = '\0';
        char *d = strchr(c, '=');
        if (d) {
          *d = '\0';
          char *name = curl_easy_unescape(curl, c, 0, NULL);
          char *value = curl_easy_unescape(curl, d + 1, 0, NULL);
          if (name && value) {
            str_tolower(name);
            if (!strcmp(name, "arg1")) {
              *arg1 = value;
              value = NULL;
            } else if (!strcmp(name, "arg2")) {
              *arg2 = value;
              value = NULL;
            } else if (!strcmp(name, "arg3")) {
              *arg3 = value;
              value = NULL;
            }
          }
          if (name) curl_free(name);
          if (value) curl_free(value);
        }
        free(c);
      }
    }
    a = b + 1;
  } while (b);

  curl_easy_cleanup(curl);
}

// ----------------------------------------------------------------------------

void log_msg(int thr_id, const char *msg) {
  pthread_mutex_lock(&g_mutex_for_log);
  if (g_daemon_mode && g_logfile_name) {
    FILE *fd = fopen(g_logfile_name, "a");
    if (fd) {
      fprintf(fd, "thread %d: %s\n", thr_id, msg);
      fclose(fd);
    }
  } else {
    fprintf(stderr, "thread %d: %s\n", thr_id, msg);
  }
  pthread_mutex_unlock(&g_mutex_for_log);
}

// ----------------------------------------------------------------------------

void *worker_fn(void *arg) {
  int thr_id = -1;
  if (arg) {
    thr_id = *(int *)arg;
    free(arg);
  }

  FCGX_Request request;
  if (FCGX_InitRequest(&request, g_socket_id, 0)) {
    log_msg(thr_id, "Can't init request");
    return NULL;
  }
  while (1) {
    if (FCGX_Accept_r(&request)) {
      log_msg(thr_id, "Can't accept request");
      continue;
    }

    char *method = FCGX_GetParam("REQUEST_METHOD", request.envp);
    if (!method) {
      log_msg(thr_id, "REQUEST_METHOD error");
      continue;
    }

    char *response = NULL;
    size_t l;
    int is_404 = 0;

    if (strcmp(method, "GET")) {
      l = 32 + strlen(method);
      response = (char *)malloc(l);
      if (response)
        snprintf(response, l, "Cant\'t process query type %s", method);
    } else {
      char *uri = FCGX_GetParam("DOCUMENT_URI", request.envp);
      if (!uri) {
        log_msg(thr_id, "DOCUMENT_URI error");
        continue;
      }
      char *query_string = FCGX_GetParam("QUERY_STRING", request.envp);
      if (!query_string) {
        log_msg(thr_id, "QUERY_STRING error");
        continue;
      }

      char *q_arg1 = NULL, *q_arg2 = NULL, *q_arg3 = NULL;
      parse_uri_params(query_string, &q_arg1, &q_arg2, &q_arg3);

      str_tolower(uri);

      log_msg(thr_id, "test message");

      if (!strcmp(uri, "/query1")) {
        if (q_arg2) {
          str_reverse(q_arg2);
          l = strlen(q_arg2) + 2;
          response = (char *)malloc(l);
          if (response) snprintf(response, l, "%s\n", q_arg2);
        }
      } else if (!strcmp(uri, "/query2")) {
        if (q_arg1 || q_arg3) {
          l = (q_arg1 ? strlen(q_arg1) : 0) + 1 +
              (q_arg3 ? strlen(q_arg3) : 0) + 2;
          response = (char *)malloc(l);
          if (response) {
            if (q_arg1) str_toupper(q_arg1);
            if (q_arg3) str_tolower(q_arg3);
            snprintf(response, l,
                "%s %s\n", (q_arg1 ? q_arg1 : ""), (q_arg3 ? q_arg3 : ""));
          }
        }
      } else if (!strcmp(uri, "/query3")) {
        if (q_arg3) {
          str_tolower(q_arg3);
          l = 5;
          response = (char *)malloc(l);
          if (response) {
            if (strstr(q_arg3, "test123"))
              snprintf(response, l, "YES\n");
            else
              snprintf(response, l, "NO\n");
          }
        }
      } else {
        is_404 = 1;
      }

      if (q_arg1) curl_free(q_arg1);
      if (q_arg2) curl_free(q_arg2);
      if (q_arg3) curl_free(q_arg3);
    }

    if (!is_404) {
      FCGX_PutS("Status: 200 OK\r\n\r\n", request.out);
      if (response) FCGX_PutS(response, request.out);
    } else {
      FCGX_PutS("Status: 404 Not Found\r\n\r\n", request.out);
    }

    FCGX_Finish_r(&request);
    if (response) free(response);
  }
  return NULL;
}

// ----------------------------------------------------------------------------

void sig_handler(int sig) {
  if (sig == SIGTERM || sig == SIGINT) {
    log_msg(0, "TERM");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "s:w:dl:")) != -1) {
    switch (opt) {
      case 's':
        g_socket_str = (char *)malloc(strlen(optarg) + 1);
        if (!g_socket_str) {
          fprintf(stderr, "Memory error\n");
          return 1;
        }
        strcpy(g_socket_str, optarg);
        break;
      case 'w':
        g_worker_count = atoi(optarg);
        break;
      case 'd':
        g_daemon_mode = 1;
        break;
      case 'l':
        g_logfile_name = (char *)malloc(strlen(optarg) + 1);
        if (!g_logfile_name) {
          fprintf(stderr, "Memory error\n");
          return 1;
        }
        strcpy(g_logfile_name, optarg);
        break;
      default:
        fprintf(stdout,
            "Usage: -s (listen socket) -w (workers) -d (daemon mode) "
            "-l (logfile)");
        return 1;
    }
  }

  if (!g_socket_str) {
    fprintf(stderr, "need listen socket!\n");
    return 1;
  }

  if (g_daemon_mode) {
    pid_t pid = fork();
    if (pid < 0) return 1;
    if (pid > 0) return 0;
    umask(002);
    pid_t sid = setsid();
    if (sid < 0) return 1;
    if ((chdir("/")) < 0) return 1;
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);

  if (FCGX_Init()) {
    log_msg(0, "FCGI init error");
    return 1;
  }
  g_socket_id = FCGX_OpenSocket(g_socket_str, 10000);
  if (g_socket_id == -1) {
    log_msg(0, "Can't open socket");
    return 1;
  }

  pthread_t workers[g_worker_count];
  for (int i = 0; i < g_worker_count; i++) {
    
    int thr_id = i + 1;
    void *thr_arg = malloc(sizeof(int));
    if (thr_arg) 
      memcpy(thr_arg, &thr_id, sizeof(int));

    pthread_create(&(workers[i]), NULL, worker_fn, thr_arg);
  }
  for (int i = 0; i < g_worker_count; i++) {
    pthread_join(workers[i], NULL);
  }

  if (g_socket_str) free(g_socket_str);
  if (g_logfile_name) free(g_logfile_name);

  return 0;
}

// ----------------------------------------------------------------------------
