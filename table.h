#ifndef TABLE_H
#define TABLE_H

#include <jansson.h>
#include <stdbool.h>

#define PATH_TABLE_MENU "./data/menu.json"
#define PATH_TABLE_STUDENT "./data/student.json"
#define PATH_TABLE_MERCHANT "./data/merchant.json"
#define PATH_TABLE_EVALUATION "./data/evaluation.json"

extern json_t *table_menu;
extern json_t *table_student;
extern json_t *table_merchant;
extern json_t *table_evaluation;

enum
{
  TYP_INT,
  TYP_STR,
};

typedef union
{
  json_int_t ival;
  const char *sval;
} find_val_t;

typedef struct
{
  int typ;
  find_val_t val;
  const char *key;
} find_pair_t;

typedef struct
{
  json_t *item;
  size_t index;
} find_ret_t;

extern void table_init (void);
extern bool save (json_t *from, const char *to);
extern find_ret_t find_by (json_t *tbl, find_pair_t *cnd, size_t num);

#endif
