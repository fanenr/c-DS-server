#include "table.h"
#include "util.h"
#include <string.h>

json_t *table_menu;
json_t *table_student;
json_t *table_merchant;
json_t *table_evaluation;

static inline FILE *
load_file (const char *path)
{
  FILE *file = fopen (path, "r+");
  if (file)
    return file;

  file = fopen (path, "w+");
  if (!file)
    error ("数据表 %s 创建失败", path);

  if (fputs ("[]", file) == EOF)
    error ("数据表 %s 初始化失败", path);

  return file;
}

static inline json_t *
load_table (FILE *file)
{
  if (fseek (file, 0, SEEK_SET) != 0)
    error ("文件流重定位失败");

  json_t *json;
  json_error_t jerr;

  json = json_loadf (file, 0, &jerr);
  if (!json)
    error ("json 解析失败");

  if (!json_is_array (json))
    error ("json 格式错误");

  fclose (file);
  return json;
}

void
table_init ()
{
  table_menu = load_table (load_file (PATH_TABLE_MENU));
  table_student = load_table (load_file (PATH_TABLE_STUDENT));
  table_merchant = load_table (load_file (PATH_TABLE_MERCHANT));
  table_evaluation = load_table (load_file (PATH_TABLE_EVALUATION));
}

find_ret_t
find_by (json_t *tbl, find_pair_t *cnd, size_t num)
{
  find_ret_t ret = { .item = NULL };
  size_t size = json_array_size (tbl);

  json_int_t ival;
  const char *sval;
  json_t *temp, *item;

  for (size_t i = 0; i < size; i++)
    {
      if (!(item = json_array_get (tbl, i)))
        error ("json 格式损坏");

      for (size_t j = 0; j < num; j++)
        {
          find_pair_t *pair = cnd + j;

          if (!(temp = json_object_get (item, pair->key)))
            error ("不存在键 %s", pair->key);

          if (pair->typ == TYP_INT)
            {
              if (!json_is_integer (temp))
                error ("类型不匹配");
              ival = json_integer_value (temp);
              if (pair->val.ival != ival)
                goto next;
            }
          else if (pair->typ == TYP_STR)
            {
              if (!json_is_string (temp))
                error ("类型不匹配");
              sval = json_string_value (temp);
              if (0 != strcmp (pair->val.sval, sval))
                goto next;
            }
          else
            error ("未知类型 %d", pair->typ);
        }

      ret.item = item;
      ret.index = i;
      goto ret;

    next:
      continue;
    }

ret:
  return ret;
}

bool
save (json_t *from, const char *to)
{
  char *str = json_dumps (from, JSON_INDENT (2));
  if (!str)
    return false;

  FILE *file = fopen (to, "w+");
  if (!file)
    return false;

  size_t len = strlen (str);
  if (fwrite (str, len, 1, file) != 1)
    return false;

  fclose (file);
  free (str);
  return true;
}
