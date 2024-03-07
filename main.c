#include "api.h"
#include "mongoose.h"
#include <stdio.h>
#include <stdlib.h>

static void handle (struct mg_connection *conn, int ev, void *ev_data);

int
main ()
{
  struct mg_mgr mgr;
  mg_mgr_init (&mgr);

  mg_http_listen (&mgr, "http://127.0.0.1:8000", handle, NULL);

  for (;;)
    mg_mgr_poll (&mgr, 1000);

  mg_mgr_free (&mgr);
}

static void
handle (struct mg_connection *conn, int ev, void *ev_data)
{
  if (ev != MG_EV_HTTP_MSG)
    return;

  struct mg_http_message *msg = ev_data;
  api_ret ret = api_handle (msg);

  mg_http_reply (conn, 200, "Content-Type: application/json\r\n",
                 "{\"code\": %d, \"data\": \"%s\"}", ret.status, ret.content);
}
