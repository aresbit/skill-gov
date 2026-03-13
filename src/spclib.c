#include "spclib.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *spc_xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return p;
}

void spc_vec_init(SpcStrVec *vec) {
    vec->items = NULL;
    vec->len = 0;
    vec->cap = 0;
}

void spc_vec_push_owned(SpcStrVec *vec, char *s) {
    if (vec->len == vec->cap) {
        size_t new_cap = vec->cap == 0 ? 8 : vec->cap * 2;
        vec->items = spc_xrealloc(vec->items, new_cap * sizeof(char *));
        vec->cap = new_cap;
    }
    vec->items[vec->len++] = s;
}

void spc_vec_free(SpcStrVec *vec) {
    size_t i;
    for (i = 0; i < vec->len; i++) {
        free(vec->items[i]);
    }
    free(vec->items);
    vec->items = NULL;
    vec->len = 0;
    vec->cap = 0;
}

char *spc_strdup(const char *s) {
    size_t n = strlen(s);
    char *out = malloc(n + 1);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(out, s, n + 1);
    return out;
}

char *spc_path_join(const char *a, const char *b) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    int need_slash = (alen > 0 && a[alen - 1] != '/');
    char *out = malloc(alen + blen + (size_t)need_slash + 1);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(out, a, alen);
    if (need_slash) {
        out[alen] = '/';
        memcpy(out + alen + 1, b, blen);
        out[alen + 1 + blen] = '\0';
    } else {
        memcpy(out + alen, b, blen);
        out[alen + blen] = '\0';
    }
    return out;
}

int spc_strcasecmp(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) {
            return ca - cb;
        }
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int spc_glob_match_ci(const char *pattern, const char *text) {
    size_t plen = strlen(pattern);
    size_t tlen = strlen(text);
    size_t i;
    size_t j;

    unsigned char *dp = calloc((plen + 1) * (tlen + 1), sizeof(unsigned char));
    if (!dp) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

#define DP(x, y) dp[(x) * (tlen + 1) + (y)]

    DP(0, 0) = 1;

    for (i = 1; i <= plen; i++) {
        if (pattern[i - 1] == '*') {
            DP(i, 0) = DP(i - 1, 0);
        }
    }

    for (i = 1; i <= plen; i++) {
        for (j = 1; j <= tlen; j++) {
            char p = pattern[i - 1];
            char c = text[j - 1];
            if (p == '*') {
                DP(i, j) = (unsigned char)(DP(i - 1, j) || DP(i, j - 1));
            } else if (p == '?' || tolower((unsigned char)p) == tolower((unsigned char)c)) {
                DP(i, j) = DP(i - 1, j - 1);
            }
        }
    }

    i = DP(plen, tlen);
    free(dp);
    return (int)i;

#undef DP
}
