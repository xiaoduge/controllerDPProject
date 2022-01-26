// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include "mongoose.h"
#include "web_server.h"
#include <iostream>
using namespace std;


static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static const struct mg_str s_get_method = MG_MK_STR("GET");
static const struct mg_str s_put_method = MG_MK_STR("PUT");
static const struct mg_str s_delele_method = MG_MK_STR("DELETE");
static const struct mg_str s_post_method = MG_MK_STR("POST");

static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
  return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

static int is_equal(const struct mg_str *s1, const struct mg_str *s2) {
  return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}

static void op_get(struct mg_connection *nc, const struct http_message *hm, const struct mg_str *key) 
{
  const char *data = NULL;
  int result;

  string strJson;
  string strUrl(key->p,key->len);
  string strQry(hm->query_string.p,hm->query_string.len);
  
  if (webserver_query(strUrl,strQry,strJson)) {

    data = strJson.c_str();
    mg_printf(nc,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/json;charset=UTF-8\r\n"
              "Content-Length: %d\r\n\r\n%s",
              (int) strlen(data), data);
  } else {
    mg_printf(nc, "%s",
              "HTTP/1.1 500 Server Error\r\n"
              "Content-Length: 0\r\n\r\n");
  }
}

static void op_post(struct mg_connection *nc, const struct http_message *hm, const struct mg_str *key) 
{
  const char *data = NULL;
  int result;

  string strJsonReq(hm->body.p,hm->body.len);
  string strUrl(key->p,key->len);
  string strJsonRsp;
  if (webserver_post(strUrl,strJsonReq,strJsonRsp)) {
    data = strJsonRsp.c_str();
    mg_printf(nc,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/json;charset=UTF-8\r\n"
              "Content-Length: %d\r\n\r\n%s",
              (int) strlen(data), data);
  } else {
    mg_printf(nc, "%s",
              "HTTP/1.1 500 Server Error\r\n"
              "Content-Length: 0\r\n\r\n");
  }
}


static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  if (ev == MG_EV_HTTP_REQUEST) {

    static const struct mg_str api_prefix = MG_MK_STR("/operation");

    struct http_message *hm = (struct http_message *) p;

    struct mg_str key;

    cout <<"ev_handler " << hm->uri.p << endl;

    if (has_prefix(&hm->uri, &api_prefix)) {
        key.p = hm->uri.p + api_prefix.len;
        key.len = hm->uri.len - api_prefix.len;
        cout <<"ev_handler key " << key.p << endl;
        if (is_equal(&hm->method, &s_get_method)) {
          cout <<"ev_handler method " << hm->method.p << endl;
          op_get(nc, hm, &key);
        } else if (is_equal(&hm->method, &s_post_method)) {
          cout <<"ev_handler method " << hm->method.p << endl;
          op_post(nc, hm, &key);
        }  else {
          mg_printf(nc, "%s",
                    "HTTP/1.0 501 Not Implemented\r\n"
                    "Content-Length: 0\r\n\r\n");
        }
      } else {
          mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
      }

  }
}

int webserver_main(void) 
{

  struct mg_mgr mgr;
  struct mg_connection *nc;

  
  mg_mgr_init(&mgr, NULL);
  printf("Starting web server on port %s\n", s_http_port);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL) {
    printf("Failed to create listener\n");
    return 1;
  }

  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
#ifdef QT_PC
  s_http_server_opts.document_root = "../web";  // Serve current directory
#else
  s_http_server_opts.document_root = "./web";  // Serve current directory
#endif  
  s_http_server_opts.enable_directory_listing = "yes";

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}

