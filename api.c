#include "api.h"
#include "mongoose.h"

api_ret
api_handle (struct mg_http_message *msg)
{
  api_ret ret = { .content = "" };

  if (mg_vcmp (&msg->method, "POST") != 0)
    {
      ret.status = ERR_NOT_POST;
      ret.content = "非 POST 请求";
      goto ret;
    }

ret:
  return ret;
}
