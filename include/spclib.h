#ifndef SPCLIB_H
#define SPCLIB_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} SpcStrVec;

void spc_vec_init(SpcStrVec *vec);
void spc_vec_push_owned(SpcStrVec *vec, char *s);
void spc_vec_free(SpcStrVec *vec);

char *spc_strdup(const char *s);
char *spc_path_join(const char *a, const char *b);
int spc_glob_match_ci(const char *pattern, const char *text);
int spc_strcasecmp(const char *a, const char *b);

#endif
