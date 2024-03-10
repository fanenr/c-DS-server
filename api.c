#include "api.h"
#include "mongoose.h"
#include "table.h"

#include <jansson.h>
#include <stdbool.h>
#include <string.h>

static void student_new (api_ret *ret, json_t *rdat);
static void student_log (api_ret *ret, json_t *rdat);
static void student_del (api_ret *ret, json_t *rdat);
static void student_mod (api_ret *ret, json_t *rdat);

static void merchant_new (api_ret *ret, json_t *rdat);
static void merchant_log (api_ret *ret, json_t *rdat);
static void merchant_del (api_ret *ret, json_t *rdat);
static void merchant_mod (api_ret *ret, json_t *rdat);

static void menu_list (api_ret *ret, json_t *rdat);
static void menu_new (api_ret *ret, json_t *rdat);

#define RET_STR(STR) "\"" STR "\""

api_ret
api_handle (struct mg_http_message *msg)
{
  api_ret ret = { .need_free = false };

  json_error_t jerr;
  json_t *rdat = NULL;

  if (mg_vcmp (&msg->method, "POST") != 0)
    {
      ret.status = API_ERR_NOT_POST;
      ret.content = RET_STR ("非 POST 请求");
      goto ret;
    }

  if (!(rdat = json_loadb (msg->body.ptr, msg->body.len, 0, &jerr)))
    {
      ret.status = API_ERR_NOT_JSON;
      ret.content = RET_STR ("数据非 JSON 格式");
      goto ret;
    }

#define API_MATCH(TYPE, API)                                                  \
  if (mg_http_match_uri (msg, "/api/" #TYPE "/" #API))                        \
    {                                                                         \
      TYPE##_##API (&ret, rdat);                                              \
      goto ret;                                                               \
    }

  API_MATCH (student, new);
  API_MATCH (student, log);
  API_MATCH (student, del);
  API_MATCH (student, mod);

  API_MATCH (merchant, new);
  API_MATCH (merchant, log);
  API_MATCH (merchant, del);
  API_MATCH (merchant, mod);

  API_MATCH (menu, list);
  API_MATCH (menu, new);

#undef API_MATCH

  ret.status = API_ERR_UNKNOWN;
  ret.content = RET_STR ("未知 API");

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
  if (0 != json_object_set (DAT, KEY, VAL))                                   \
  goto ERR

#define SET_NEW(DAT, KEY, VAL, ERR)                                           \
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

  const char *user_str = json_string_value (user);
  const char *id_str = json_string_value (id);

  if (find_by (table_student, "user", TYP_STR, user_str).item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = RET_STR ("帐号已存在");
      return;
    }

  if (find_by (table_student, "id", TYP_STR, id_str).item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = RET_STR ("学号已存在");
      return;
    }

  json_t *new = json_object ();

  SET (new, "user", user, err2);
  SET (new, "pass", pass, err2);

  SET (new, "id", id, err2);
  SET (new, "name", name, err2);
  SET (new, "number", number, err2);

  if (0 != json_array_append_new (table_student, new))
    goto err2;

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("注册成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
student_log (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);

  json_t *find = find_by (table_student, "user", TYP_STR, user_str).item;
  if (!find)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  char *info_str = json_dumps (find, 0);
  if (!info_str)
    goto err2;

  ret->status = API_OK;
  ret->need_free = true;
  ret->content = info_str;
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
merchant_new (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *name = GET (rdat, "name", string, err);
  json_t *number = GET (rdat, "number", string, err);
  json_t *position = GET (rdat, "position", string, err);

  const char *user_str = json_string_value (user);
  const char *name_str = json_string_value (name);

  if (find_by (table_merchant, "user", TYP_STR, user_str).item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = RET_STR ("帐号已存在");
      return;
    }

  if (find_by (table_merchant, "name", TYP_STR, name_str).item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = RET_STR ("名称已存在");
      return;
    }

  json_t *new = json_object ();

  SET (new, "user", user, err2);
  SET (new, "pass", pass, err2);

  SET (new, "name", name, err2);
  SET (new, "number", number, err2);
  SET (new, "position", position, err2);

  if (0 != json_array_append_new (table_merchant, new))
    goto err2;

  if (!save (table_merchant, PATH_TABLE_MERCHANT))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("注册成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
merchant_log (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);
  json_t *find = find_by (table_merchant, "user", TYP_STR, user_str).item;

  if (!find)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  char *info_str = json_dumps (find, 0);
  if (!info_str)
    goto err2;

  ret->status = API_OK;
  ret->need_free = true;
  ret->content = info_str;
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
student_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);

  find_ret find = find_by (table_student, "user", TYP_STR, user_str);
  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  if (0 != json_array_remove (table_student, find.index))
    goto err2;

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("删除成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
merchant_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);

  find_ret find = find_by (table_merchant, "user", TYP_STR, user_str);
  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  if (0 != json_array_remove (table_merchant, find.index))
    goto err2;

  if (!save (table_merchant, PATH_TABLE_MERCHANT))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("删除成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
student_mod (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *npass = GET (rdat, "npass", string, err);
  json_t *nname = GET (rdat, "nname", string, err);
  json_t *nnumber = GET (rdat, "nnumber", string, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);

  find_ret find = find_by (table_student, "user", TYP_STR, user_str);
  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  SET (find.item, "pass", npass, err2);
  SET (find.item, "name", nname, err2);
  SET (find.item, "number", nnumber, err2);

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("更新成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
merchant_mod (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *npass = GET (rdat, "npass", string, err);
  json_t *nname = GET (rdat, "nname", string, err);
  json_t *nnumber = GET (rdat, "nnumber", string, err);
  json_t *nposition = GET (rdat, "nposition", string, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);
  find_ret find = find_by (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  SET (find.item, "pass", npass, err2);
  SET (find.item, "name", nname, err2);
  SET (find.item, "number", nnumber, err2);
  SET (find.item, "position", nposition, err2);

  if (!save (table_merchant, PATH_TABLE_MERCHANT))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("更新成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static void
menu_list (api_ret *ret, json_t *rdat)
{
  json_t *user = json_object_get (rdat, "user");

  char *list_str = json_dumps (table_menu, 0);
  if (!list_str)
    goto err2;

  ret->status = API_OK;
  ret->need_free = true;
  ret->content = list_str;
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;
}

static void
menu_new (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *name = GET (rdat, "name", string, err);
  json_t *price = GET (rdat, "price", number, err);

  const char *user_str = json_string_value (user);
  const char *pass_str = json_string_value (pass);
  find_ret find = find_by (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (strcmp (pass_str, rpass_str) != 0)
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  const char *name_str = json_string_value (name);
  json_t *position = GET (find.item, "position", string, err2);
  find_ret find2 = find_by (table_menu, "name", TYP_STR, name_str);

  if (find2.item)
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = RET_STR ("菜品已存在");
      return;
    }

  json_t *id;
  json_t *new;

  if (!(new = json_object ()))
    goto err2;

  json_t *nuser = GET (find.item, "name", string, err2);
  if (!(id = json_integer (json_array_size (table_menu))))
    goto err2;

  SET_NEW (new, "id", id, err2);
  SET (new, "name", name, err2);
  SET (new, "user", nuser, err2);
  SET (new, "price", price, err2);
  SET (new, "position", position, err2);

  if (0 != json_array_append_new (table_menu, new))
    goto err2;

  if (!save (table_menu, PATH_TABLE_MENU))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("添加成功");
  return;

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}
