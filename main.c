#include "mongoose.h"
#include <stdio.h>
#include <stdlib.h>

static void
fn (struct mg_connection *conn, int ev, void *ev_data)
{
  if (ev != MG_EV_HTTP_MSG)
    return;

  struct mg_http_message *msg = ev_data;
  if (!mg_http_match_uri (msg, "/api"))
    mg_http_reply (conn, 404, "", "error\n");

  mg_http_reply (conn, 200, "", "success\n");
}

int
main ()
{
  struct mg_mgr mgr;
  mg_mgr_init (&mgr);

  mg_http_listen (&mgr, "http://127.0.0.1:8000", fn, NULL);

  for (;;)
    mg_mgr_poll (&mgr, 1000);

  mg_mgr_free (&mgr);
}