#ifndef TABLE_H
#define TABLE_H

#include <jansson.h>

#define PATH_TABLE_MENU "./menu.json"
#define PATH_TABLE_STUDENT "./student.json"
#define PATH_TABLE_MERCHANT "./merchant.json"
#define PATH_TABLE_EVALUATION "./evaluation.json"

extern json_t *table_menu;
extern json_t *table_student;
extern json_t *table_merchant;
extern json_t *table_evaluation;

enum
{
  TYP_INT,
  TYP_STR,
};

typedef struct
{
  json_t *item;
  size_t index;
} find_ret;

extern void table_init (void);
extern void save (json_t *from, const char *to);
extern find_ret find_by (json_t *tbl, const char *key, int typ, ...);

#endif
