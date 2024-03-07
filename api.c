#include "api.h"
#include "mongoose.h"
#include "table.h"
#include <jansson.h>
#include <string.h>

static void student_new (api_ret *ret, json_t *rdat);
static void student_log (api_ret *ret, json_t *rdat);

api_ret
api_handle (struct mg_http_message *msg)
{
  api_ret ret = { .content = "" };

  json_error_t jerr;
  json_t *rdat = NULL;

  if (mg_vcmp (&msg->method, "POST") != 0)
    {
      ret.status = API_ERR_NOT_POST;
      ret.content = "非 POST 请求";
      goto ret;
    }

  if (!(rdat = json_loadb (msg->body.ptr, msg->body.len, 0, &jerr)))
    {
      ret.status = API_ERR_NOT_JSON;
      ret.content = "数据非 JSON 格式";
      goto ret;
    }

  if (mg_http_match_uri (msg, "/api/student/new"))
    {
      student_new (&ret, rdat);
      goto ret;
    }

  if (mg_http_match_uri (msg, "/api/student/log"))
    {
      student_log (&ret, rdat);
      goto ret;
    }

  ret.status = API_ERR_UNKNOWN;
  ret.content = "未知 API";

ret:
  json_decref (rdat);
  return ret;
}

#define GET(DAT, KEY, TYP, ERR)                                               \
  ({                                                                          \
    json_t *ret = json_object_get (DAT, KEY);                                 \
    if (!json_is_##TYP (ret))                                                 \
      goto ERR;                                                               \
    ret;                                                                      \
  })

#define SET(DAT, KEY, VAL, ERR)                                               \
  if (0 != json_object_set_new (DAT, KEY, VAL))                               \
  goto ERR

static inline void
student_new (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *id = GET (rdat, "id", string, err);
  json_t *name = GET (rdat, "name", string, err);
  json_t *number = GET (rdat, "number", string, err);

  if (find_by (table_student, "user", TYP_STR, json_string_value (user)).item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = "帐号已存在";
      return;
    }

  if (find_by (table_student, "id", TYP_STR, json_string_value (id)).item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = "学号已存在";
      return;
    }

  json_t *new;
  if (!(new = json_object ()))
    goto err2;

  SET (new, "user", user, err2);
  SET (new, "pass", pass, err2);
  SET (new, "name", name, err2);
  SET (new, "id", id, err2);
  SET (new, "number", number, err2);

  if (0 != json_array_append_new (table_student, new))
    goto err2;

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  ret->status = API_OK;
  ret->content = "注册成功";
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = "内部错误";
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = "数据不完整";
}

static inline void
student_log (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *find
      = find_by (table_student, "user", TYP_STR, json_string_value (user))
            .item;
  if (!find)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = "帐号不存在";
      return;
    }

  json_t *real = GET (find, "pass", string, err2);
  const char *real_pass = json_string_value (real);

  if (strcmp (json_string_value (pass), real_pass) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = "密码错误";
      return;
    }

  ret->status = API_OK;
  ret->content = "登录成功";
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = "内部错误";
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = "数据不完整";
}
