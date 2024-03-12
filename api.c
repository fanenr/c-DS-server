#include "api.h"
#include "mongoose.h"
#include "table.h"

#include <jansson.h>
#include <stdbool.h>
#include <string.h>

static void student_new (api_ret *ret, json_t *rdat);
static void student_log (api_ret *ret, json_t *rdat);
static void student_mod (api_ret *ret, json_t *rdat);
static void student_del (api_ret *ret, json_t *rdat);

static void merchant_new (api_ret *ret, json_t *rdat);
static void merchant_log (api_ret *ret, json_t *rdat);
static void merchant_mod (api_ret *ret, json_t *rdat);
static void merchant_del (api_ret *ret, json_t *rdat);

static void menu_list (api_ret *ret, json_t *rdat);
static void menu_new (api_ret *ret, json_t *rdat);
static void menu_mod (api_ret *ret, json_t *rdat);
static void menu_del (api_ret *ret, json_t *rdat);

static void eva_new (api_ret *ret, json_t *rdat);

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
  API_MATCH (menu, mod);
  API_MATCH (menu, del);

#undef API_MATCH

  ret.status = API_ERR_UNKNOWN;
  ret.content = RET_STR ("未知 API");

ret:
  json_decref (rdat);
  return ret;
}

#define ISSEQ(S1, S2) (strcmp (S1, S2) == 0)

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

  if (!ISSEQ (pass_str, rpass_str))
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

  if (!ISSEQ (pass_str, rpass_str))
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

  if (!ISSEQ (pass_str, rpass_str))
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

  if (!ISSEQ (pass_str, rpass_str))
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

  json_t *ruser = GET (find.item, "user", string, err2);
  json_t *rname = GET (find.item, "name", string, err2);
  const char *ruser_str = json_string_value (ruser);
  const char *rname_str = json_string_value (rname);
  const char *nname_str = json_string_value (nname);

  if (ISSEQ (nname_str, rname_str) && !ISSEQ (user_str, ruser_str))
    {
      ret->status = API_ERR_DUPLICATE;
      ret->content = RET_STR ("名称已存在");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);

  if (!ISSEQ (pass_str, rpass_str))
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

  if (!ISSEQ (pass_str, rpass_str))
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
menu_list (api_ret *ret, json_t *rdat)
{
  json_t *user = json_object_get (rdat, "user");

  json_t *arr, *temp;
  size_t num = json_array_size (table_menu);

  if (!(arr = json_array ()))
    goto err;

  for (size_t i = 0; i < num; i++)
    {
      json_t *item = json_array_get (table_menu, i);

      json_t *id = GET (item, "id", integer, err);
      json_t *name = GET (item, "name", string, err);
      json_t *user = GET (item, "user", string, err);
      json_t *price = GET (item, "price", number, err);

      const char *user_str = json_string_value (user);
      find_ret find = find_by (table_merchant, "user", TYP_STR, user_str);

      if (!find.item)
        goto err;

      json_t *uname = GET (find.item, "name", string, err);
      json_t *position = GET (find.item, "position", string, err);

      if (!(temp = json_object ()))
        goto err2;

      SET (temp, "id", id, err3);
      SET (temp, "name", name, err3);
      SET (temp, "user", user, err3);
      SET (temp, "price", price, err3);
      SET (temp, "uname", uname, err3);
      SET (temp, "position", position, err3);

      if (0 != json_array_append_new (arr, temp))
        goto err3;
    }

  char *list_str = json_dumps (arr, 0);
  if (!list_str)
    goto err2;

  ret->status = API_OK;
  ret->need_free = true;
  ret->content = list_str;
  return;

err3:
  json_decref (temp);

err2:
  json_decref (arr);

err:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;
}

static inline void
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

  if (!ISSEQ (pass_str, rpass_str))
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  json_t *id, *new;
  json_int_t id_int = 0;
  size_t size = json_array_size (table_menu);

  if (size)
    {
      json_t *last = json_array_get (table_menu, size - 1);
      json_t *last_id = GET (last, "id", integer, err2);
      id_int = json_integer_value (last_id) + 1;
    }

  if (!(id = json_integer (id_int)))
    goto err2;

  if (!(new = json_object ()))
    goto err3;

  SET_NEW (new, "id", id, err4);
  SET (new, "name", name, err4);
  SET (new, "user", user, err4);
  SET (new, "price", price, err4);

  if (0 != json_array_append_new (table_menu, new))
    goto err4;

  if (!save (table_menu, PATH_TABLE_MENU))
    goto err4;

  ret->status = API_OK;
  ret->content = RET_STR ("添加成功");
  return;

err4:
  json_decref (new);

err3:
  json_decref (id);

err2:
  ret->status = API_ERR_INNER;
  ret->content = RET_STR ("内部错误");
  return;

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}

static inline void
menu_mod (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *id = GET (rdat, "id", integer, err);
  json_t *nname = GET (rdat, "nname", string, err);
  json_t *nprice = GET (rdat, "nprice", number, err);

  const char *user_str = json_string_value (user);
  find_ret find = find_by (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_int_t id_int = json_integer_value (id);
  find_ret find2 = find_by (table_menu, "id", TYP_INT, id_int);

  if (!find2.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("菜品不存在");
      return;
    }

  json_t *ruser = GET (find2.item, "user", string, err2);
  const char *ruser_str = json_string_value (ruser);

  if (!ISSEQ (user_str, ruser_str))
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("菜品非该商户所有");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  SET (find2.item, "name", nname, err2);
  SET (find2.item, "price", nprice, err2);

  if (!save (table_menu, PATH_TABLE_MENU))
    goto err2;

  ret->status = API_OK;
  ret->content = RET_STR ("修改成功");
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
menu_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);
  json_t *id = GET (rdat, "id", integer, err);

  const char *user_str = json_string_value (user);
  find_ret find = find_by (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("帐号不存在");
      return;
    }

  json_int_t id_int = json_integer_value (id);
  find_ret find2 = find_by (table_menu, "id", TYP_INT, id_int);

  if (!find2.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("菜品不存在");
      return;
    }

  json_t *ruser = GET (find2.item, "user", string, err2);
  const char *ruser_str = json_string_value (ruser);

  if (!ISSEQ (user_str, ruser_str))
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("菜品非该商户所有");
      return;
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    {
      ret->status = API_ERR_WRONG_PASS;
      ret->content = RET_STR ("密码错误");
      return;
    }

  if (0 != json_array_remove (table_menu, find2.index))
    goto err2;

  if (!save (table_menu, PATH_TABLE_MENU))
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
eva_new (api_ret *ret, json_t *rdat)
{
  json_t *id = GET (rdat, "id", integer, err);
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);
  json_t *grade = GET (rdat, "grade", number, err);
  json_t *evaluation = GET (rdat, "evaluation", string, err);

  json_int_t id_int = json_integer_value (id);
  find_ret find = find_by (table_menu, "id", TYP_INT, id_int);

  if (!find.item)
    {
      ret->status = API_ERR_NOT_EXIST;
      ret->content = RET_STR ("菜品不存在");
      return;
    }

err:
  ret->status = API_ERR_INCOMPLETE;
  ret->content = RET_STR ("数据不完整");
}
