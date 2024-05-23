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

static void eva_list (api_ret *ret, json_t *rdat);
static void eva_new (api_ret *ret, json_t *rdat);
static void eva_mod (api_ret *ret, json_t *rdat);
static void eva_del (api_ret *ret, json_t *rdat);

#define QUOTE(STR) "\"" STR "\""

#define ISSEQ(S1, S2) (strcmp ((S1), (S2)) == 0)

#define GET(OBJ, KEY, TYP, ERR)                                               \
  ({                                                                          \
    json_t *ret = json_object_get ((OBJ), (KEY));                             \
    if (!json_is_##TYP (ret))                                                 \
      goto ERR;                                                               \
    ret;                                                                      \
  })

#define SET(OBJ, KEY, VAL, ERR)                                               \
  do                                                                          \
    if (0 != json_object_set ((OBJ), (KEY), (VAL)))                           \
      goto ERR;                                                               \
  while (0)

#define SET_NEW(OBJ, KEY, VAL, ERR)                                           \
  do                                                                          \
    if (0 != json_object_set_new ((OBJ), (KEY), (VAL)))                       \
      goto ERR;                                                               \
  while (0)

#define RET_STR(PRET, CODE, STR)                                              \
  do                                                                          \
    {                                                                         \
      (PRET)->status = (CODE);                                                \
      (PRET)->content = QUOTE (STR);                                          \
      return;                                                                 \
    }                                                                         \
  while (0)

#define FIND_BY1(TBL, KEY1, TYP1, VAL1)                                       \
  ({                                                                          \
    find_pair_t cnd[]                                                         \
        = { { .typ = (TYP1), .val = (find_val_t)(VAL1), .key = (KEY1) } };    \
    find_ret_t ret = find_by ((TBL), cnd, 1);                                 \
    ret;                                                                      \
  })

#define FIND_BY2(TBL, KEY1, TYP1, VAL1, KEY2, TYP2, VAL2)                     \
  ({                                                                          \
    find_pair_t cnd[]                                                         \
        = { { .typ = (TYP1), .val = (find_val_t)(VAL1), .key = (KEY1) },      \
            { .typ = (TYP2), .val = (find_val_t)(VAL2), .key = (KEY2) } };    \
    find_ret_t ret = find_by ((TBL), cnd, 2);                                 \
    ret;                                                                      \
  })

api_ret
api_handle (struct mg_http_message *msg)
{
  api_ret ret = { .need_free = false };

  json_error_t jerr;
  json_t *rdat = NULL;

  if (mg_strcmp (msg->method, mg_str ("POST")) != 0)
    {
      ret.status = API_ERR_NOT_POST;
      ret.content = QUOTE ("非 POST 请求");
      goto ret;
    }

  if (!(rdat = json_loadb (msg->body.buf, msg->body.len, 0, &jerr)))
    {
      ret.status = API_ERR_NOT_JSON;
      ret.content = QUOTE ("数据非 JSON 格式");
      goto ret;
    }

#define API_MATCH(TYPE, API)                                                  \
  do                                                                          \
    if (mg_match (msg->uri, mg_str ("/api/" #TYPE "/" #API), NULL))           \
      {                                                                       \
        TYPE##_##API (&ret, rdat);                                            \
        goto ret;                                                             \
      }                                                                       \
  while (0)

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

  API_MATCH (eva, list);
  API_MATCH (eva, new);
  API_MATCH (eva, mod);
  API_MATCH (eva, del);

#undef API_MATCH

  ret.status = API_ERR_UNKNOWN;
  ret.content = QUOTE ("未知 API");

ret:
  json_decref (rdat);
  return ret;
}

static inline void
student_new (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *id = GET (rdat, "id", string, err);
  json_t *name = GET (rdat, "name", string, err);
  json_t *number = GET (rdat, "number", string, err);

  const char *id_str = json_string_value (id);
  const char *user_str = json_string_value (user);

  if (FIND_BY1 (table_student, "user", TYP_STR, user_str).item)
    RET_STR (ret, API_ERR_DUPLICATE, "帐号已存在");

  if (FIND_BY1 (table_student, "id", TYP_STR, id_str).item)
    RET_STR (ret, API_ERR_DUPLICATE, "学号已存在");

  json_t *new;
  if (!(new = json_object ()))
    goto err2;

  SET (new, "id", id, err2);
  SET (new, "user", user, err2);
  SET (new, "pass", pass, err2);
  SET (new, "name", name, err2);
  SET (new, "number", number, err2);

  if (0 != json_array_append_new (table_student, new))
    goto err2;

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  RET_STR (ret, API_OK, "注册成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
student_log (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  find_ret_t find = FIND_BY1 (table_student, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  char *info_str = json_dumps (find.item, 0);
  if (!info_str)
    goto err2;

  ret->status = API_OK;
  ret->need_free = true;
  ret->content = info_str;
  return;

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
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
  find_ret_t find = FIND_BY1 (table_student, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  json_t *old = find.item;
  SET (old, "pass", npass, err2);
  SET (old, "name", nname, err2);
  SET (old, "number", nnumber, err2);

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  RET_STR (ret, API_OK, "修改成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
student_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  find_ret_t find = FIND_BY1 (table_student, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  if (0 != json_array_remove (table_student, find.index))
    goto err2;

  if (!save (table_student, PATH_TABLE_STUDENT))
    goto err2;

  RET_STR (ret, API_OK, "注销成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
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

  if (FIND_BY1 (table_merchant, "user", TYP_STR, user_str).item)
    RET_STR (ret, API_ERR_DUPLICATE, "帐号已存在");

  if (FIND_BY1 (table_merchant, "name", TYP_STR, name_str).item)
    RET_STR (ret, API_ERR_DUPLICATE, "店名已存在");

  json_t *new;
  if (!(new = json_object ()))
    goto err2;

  SET (new, "user", user, err2);
  SET (new, "pass", pass, err2);
  SET (new, "name", name, err2);
  SET (new, "number", number, err2);
  SET (new, "position", position, err2);

  if (0 != json_array_append_new (table_merchant, new))
    goto err2;

  if (!save (table_merchant, PATH_TABLE_MERCHANT))
    goto err2;

  RET_STR (ret, API_OK, "注册成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
merchant_log (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  char *info_str = json_dumps (find.item, 0);
  if (!info_str)
    goto err2;

  ret->status = API_OK;
  ret->need_free = true;
  ret->content = info_str;
  return;

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
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
  find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  const char *nname_str = json_string_value (nname);
  find_ret_t find2 = FIND_BY1 (table_merchant, "name", TYP_STR, nname_str);

  if (find2.item)
    {
      json_t *euser = GET (find2.item, "user", string, err2);
      const char *euser_str = json_string_value (euser);

      if (!ISSEQ (user_str, euser_str))
        RET_STR (ret, API_ERR_DUPLICATE, "店名已存在");
    }

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  json_t *old = find.item;
  SET (old, "pass", npass, err2);
  SET (old, "name", nname, err2);
  SET (old, "number", nnumber, err2);
  SET (old, "position", nposition, err2);

  if (!save (table_merchant, PATH_TABLE_MERCHANT))
    goto err2;

  RET_STR (ret, API_OK, "修改成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
merchant_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  const char *user_str = json_string_value (user);
  find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  if (0 != json_array_remove (table_merchant, find.index))
    goto err2;

  if (!save (table_merchant, PATH_TABLE_MERCHANT))
    goto err2;

  RET_STR (ret, API_OK, "注销成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
menu_list (api_ret *ret, json_t *rdat)
{
  json_t *user = json_object_get (rdat, "user");
  size_t num = json_array_size (table_menu);
  json_t *arr, *temp;

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
      find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

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

  ret->content = list_str;
  ret->need_free = true;
  ret->status = API_OK;
  json_decref (arr);
  return;

err3:
  json_decref (temp);

err2:
  json_decref (arr);

err:
  RET_STR (ret, API_ERR_INNER, "内部错误");
}

static inline void
menu_new (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *name = GET (rdat, "name", string, err);
  json_t *price = GET (rdat, "price", number, err);

  const char *user_str = json_string_value (user);
  find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

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

  RET_STR (ret, API_OK, "添加成功");

err4:
  json_decref (new);

err3:
  json_decref (id);

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
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
  find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_int_t id_int = json_integer_value (id);
  find_ret_t find2 = FIND_BY1 (table_menu, "id", TYP_INT, id_int);

  if (!find2.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品不存在");

  json_t *ruser = GET (find2.item, "user", string, err2);
  const char *ruser_str = json_string_value (ruser);

  if (!ISSEQ (user_str, ruser_str))
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品非该商户所有");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  json_t *old = find2.item;
  SET (old, "name", nname, err2);
  SET (old, "price", nprice, err2);

  if (!save (table_menu, PATH_TABLE_MENU))
    goto err2;

  RET_STR (ret, API_OK, "修改成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
menu_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);
  json_t *id = GET (rdat, "id", integer, err);

  const char *user_str = json_string_value (user);
  find_ret_t find = FIND_BY1 (table_merchant, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_int_t id_int = json_integer_value (id);
  find_ret_t find2 = FIND_BY1 (table_menu, "id", TYP_INT, id_int);

  if (!find2.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品不存在");

  json_t *ruser = GET (find2.item, "user", string, err2);
  const char *ruser_str = json_string_value (ruser);

  if (!ISSEQ (user_str, ruser_str))
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品非该商户所有");

  json_t *rpass = GET (find.item, "pass", string, err2);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  if (0 != json_array_remove (table_menu, find2.index))
    goto err2;

  if (!save (table_menu, PATH_TABLE_MENU))
    goto err2;

  RET_STR (ret, API_OK, "修改成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
eva_list (api_ret *ret, json_t *rdat)
{
  json_t *arr, *temp;
  json_t *id = GET (rdat, "id", integer, err);
  json_int_t id_int = json_integer_value (id);
  size_t size = json_array_size (table_evaluation);

  if (!(arr = json_array ()))
    goto err2;

  for (size_t i = 0; i < size; i++)
    {
      json_t *item = json_array_get (table_evaluation, i);

      json_t *item_id = GET (item, "id", integer, err2);
      json_int_t item_id_int = json_integer_value (item_id);

      if (item_id_int != id_int)
        continue;

      json_t *evaluation = GET (item, "evaluation", string, err2);
      json_t *grade = GET (item, "grade", number, err2);
      json_t *user = GET (item, "user", string, err2);
      const char *user_str = json_string_value (user);

      find_ret_t find = FIND_BY1 (table_student, "user", TYP_STR, user_str);
      json_t *uname = GET (find.item, "name", string, err2);
      const char *uname_str = json_string_value (uname);

      if (!(temp = json_object ()))
        goto err2;

      SET (temp, "id", id, err4);
      SET (temp, "user", user, err4);
      SET (temp, "uname", uname, err4);
      SET (temp, "grade", grade, err4);
      SET (temp, "evaluation", evaluation, err4);

      if (0 != json_array_append_new (arr, temp))
        goto err4;
    }

  char *list_str = json_dumps (arr, 0);
  if (!list_str)
    goto err3;

  ret->content = list_str;
  ret->need_free = true;
  ret->status = API_OK;
  json_decref (arr);
  return;

err4:
  json_decref (temp);

err3:
  json_decref (arr);

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
eva_new (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *id = GET (rdat, "id", integer, err);
  json_t *grade = GET (rdat, "grade", number, err);
  json_t *evaluation = GET (rdat, "evaluation", string, err);

  json_int_t id_int = json_integer_value (id);
  find_ret_t find = FIND_BY1 (table_menu, "id", TYP_INT, id_int);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品不存在");

  const char *user_str = json_string_value (user);
  find_ret_t find2 = FIND_BY1 (table_student, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find2.item, "pass", string, err);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  find_ret_t find3 = FIND_BY2 (table_evaluation, "id", TYP_INT, id_int, "user",
                               TYP_STR, user_str);

  if (find3.item)
    RET_STR (ret, API_ERR_DUPLICATE, "已经评价过该菜品");

  json_t *new;
  if (!(new = json_object ()))
    goto err2;

  SET (new, "id", id, err3);
  SET (new, "user", user, err3);
  SET (new, "grade", grade, err3);
  SET (new, "evaluation", evaluation, err3);

  if (0 != json_array_append_new (table_evaluation, new))
    goto err3;

  if (!save (table_evaluation, PATH_TABLE_EVALUATION))
    goto err3;

  RET_STR (ret, API_OK, "评价成功");

err3:
  json_decref (new);

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
eva_mod (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *id = GET (rdat, "id", integer, err);
  json_t *ngrade = GET (rdat, "ngrade", number, err);
  json_t *nevaluation = GET (rdat, "nevaluation", string, err);

  json_int_t id_int = json_integer_value (id);
  find_ret_t find = FIND_BY1 (table_menu, "id", TYP_INT, id_int);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品不存在");

  const char *user_str = json_string_value (user);
  find_ret_t find2 = FIND_BY1 (table_student, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find2.item, "pass", string, err);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  find_ret_t find3 = FIND_BY2 (table_evaluation, "id", TYP_INT, id_int, "user",
                               TYP_STR, user_str);

  if (!find3.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "未评价过该菜品");

  json_t *old = find3.item;
  SET (old, "grade", ngrade, err2);
  SET (old, "evaluation", nevaluation, err2);

  if (!save (table_evaluation, PATH_TABLE_EVALUATION))
    goto err2;

  RET_STR (ret, API_OK, "修改成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}

static inline void
eva_del (api_ret *ret, json_t *rdat)
{
  json_t *user = GET (rdat, "user", string, err);
  json_t *pass = GET (rdat, "pass", string, err);

  json_t *id = GET (rdat, "id", integer, err);

  json_int_t id_int = json_integer_value (id);
  find_ret_t find = FIND_BY1 (table_menu, "id", TYP_INT, id_int);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "菜品不存在");

  const char *user_str = json_string_value (user);
  find_ret_t find2 = FIND_BY1 (table_student, "user", TYP_STR, user_str);

  if (!find.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "帐号不存在");

  json_t *rpass = GET (find2.item, "pass", string, err);
  const char *rpass_str = json_string_value (rpass);
  const char *pass_str = json_string_value (pass);

  if (!ISSEQ (pass_str, rpass_str))
    RET_STR (ret, API_ERR_WRONG_PASS, "密码错误");

  find_ret_t find3 = FIND_BY2 (table_evaluation, "id", TYP_INT, id_int, "user",
                               TYP_STR, user_str);

  if (!find3.item)
    RET_STR (ret, API_ERR_NOT_EXIST, "未评价过该菜品");

  if (0 != json_array_remove (table_evaluation, find3.index))
    goto err2;

  if (!save (table_evaluation, PATH_TABLE_EVALUATION))
    goto err2;

  RET_STR (ret, API_OK, "删除成功");

err2:
  RET_STR (ret, API_ERR_INNER, "内部错误");

err:
  RET_STR (ret, API_ERR_INCOMPLETE, "数据不完整");
}
