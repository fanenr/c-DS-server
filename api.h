#ifndef API_H
#define API_H

#include "mongoose.h"

enum
{
  API_OK,
  API_ERR_NOT_POST,
  API_ERR_NOT_JSON,
  API_ERR_UNKNOWN,
  API_ERR_INCOMPLETE,
  API_ERR_DUPLICATE,
  API_ERR_INNER,
  API_ERR_NOT_EXIST,
  API_ERR_WRONG_PASS,
};

typedef struct
{
  int status;
  const char *content;
} api_ret;

extern api_ret api_handle (struct mg_http_message *msg);

#endif