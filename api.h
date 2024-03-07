#ifndef API_H
#define API_H

#include "mongoose.h"

enum
{
  ERR_NOT_POST = 1,
};

typedef struct
{
  int status;
  const char *content;
} api_ret;

extern api_ret api_handle (struct mg_http_message *msg);

#endif