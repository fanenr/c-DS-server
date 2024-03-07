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

find_ret
find_by (json_t *tbl, const char *key, int typ, ...)
{
  find_ret ret = { .item = NULL };

  int ival;
  char *sval;
  size_t size;

  va_list ap;
  va_start (ap, typ);

  if (!(size = json_array_size (tbl)))
    goto ret;

  switch (typ)
    {
    case TYP_INT:
      ival = va_arg (ap, int);
      break;
    case TYP_STR:
      sval = va_arg (ap, char *);
      break;
    default:
      error ("未知数据类型 %d", typ);
    }

  for (size_t i = 0; i < size; i++)
    {
      json_t *item = json_array_get (tbl, i);
      json_t *jval = json_object_get (item, key);
      if (!jval)
        error ("json 数据格式错误: 缺少 %s 键值", key);

      switch (typ)
        {
        case TYP_INT:
          if (ival != json_integer_value (jval))
            continue;
          break;
        case TYP_STR:
          if (strcmp (sval, json_string_value (jval)) != 0)
            continue;
          break;
        }

      ret.item = item;
      ret.index = i;
      goto ret;
    }

ret:
  va_end (ap);
  return ret;
}
