#include "spclib.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char *name;
    int enabled;
} Skill;

typedef struct {
    Skill *items;
    size_t len;
    size_t cap;
} SkillVec;

typedef struct {
    char *src;
    char *dst;
    char *name;
} Move;

typedef struct {
    Move *items;
    size_t len;
    size_t cap;
} MoveVec;

typedef struct {
    int list;
    int dry_run;
    int force;
    int has_cmd;
    int enable;
    int enable_all;
    int disable_all;
    char **patterns;
    int pattern_count;
} Cli;

static const char *GREEN = "\x1b[32m";
static const char *RED = "\x1b[31m";
static const char *DIM = "\x1b[2m";
static const char *BOLD = "\x1b[1m";
static const char *RESET = "\x1b[0m";

static void usage(FILE *out) {
    fprintf(out,
            "skill-gov - C clone for skills enable/disable\n\n"
            "Usage:\n"
            "  skill-gov --list\n"
            "  skill-gov [--dry-run] [--force] enable <pattern> [pattern ...]\n"
            "  skill-gov [--dry-run] [--force] disable <pattern> [pattern ...]\n"
            "  skill-gov [--dry-run] [--force] enableall\n"
            "  skill-gov [--dry-run] [--force] disableall\n"
            "\n"
            "Glob supports '*' and '?', case-insensitive.\n");
}

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static int is_dir_path(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static void skill_vec_init(SkillVec *vec) {
    vec->items = NULL;
    vec->len = 0;
    vec->cap = 0;
}

static void skill_vec_push(SkillVec *vec, char *name, int enabled) {
    if (vec->len == vec->cap) {
        size_t new_cap = vec->cap == 0 ? 16 : vec->cap * 2;
        Skill *next = realloc(vec->items, new_cap * sizeof(Skill));
        if (!next) {
            die("out of memory");
        }
        vec->items = next;
        vec->cap = new_cap;
    }
    vec->items[vec->len].name = name;
    vec->items[vec->len].enabled = enabled;
    vec->len++;
}

static void skill_vec_free(SkillVec *vec) {
    size_t i;
    for (i = 0; i < vec->len; i++) {
        free(vec->items[i].name);
    }
    free(vec->items);
}

static void move_vec_init(MoveVec *vec) {
    vec->items = NULL;
    vec->len = 0;
    vec->cap = 0;
}

static void move_vec_push(MoveVec *vec, char *src, char *dst, char *name) {
    if (vec->len == vec->cap) {
        size_t new_cap = vec->cap == 0 ? 16 : vec->cap * 2;
        Move *next = realloc(vec->items, new_cap * sizeof(Move));
        if (!next) {
            die("out of memory");
        }
        vec->items = next;
        vec->cap = new_cap;
    }
    vec->items[vec->len].src = src;
    vec->items[vec->len].dst = dst;
    vec->items[vec->len].name = name;
    vec->len++;
}

static void move_vec_free(MoveVec *vec) {
    size_t i;
    for (i = 0; i < vec->len; i++) {
        free(vec->items[i].src);
        free(vec->items[i].dst);
        free(vec->items[i].name);
    }
    free(vec->items);
}

static int skill_cmp(const void *a, const void *b) {
    const Skill *sa = (const Skill *)a;
    const Skill *sb = (const Skill *)b;
    return spc_strcasecmp(sa->name, sb->name);
}

static int parse_cli(int argc, char **argv, Cli *cli) {
    int i;
    memset(cli, 0, sizeof(*cli));

    if (argc == 1) {
        usage(stderr);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0) {
            cli->list = 1;
        } else if (strcmp(argv[i], "--dry-run") == 0) {
            cli->dry_run = 1;
        } else if (strcmp(argv[i], "--force") == 0) {
            cli->force = 1;
        } else if (strcmp(argv[i], "enable") == 0 || strcmp(argv[i], "disable") == 0 ||
                   strcmp(argv[i], "enableall") == 0 ||
                   strcmp(argv[i], "disableall") == 0) {
            if (cli->has_cmd) {
                fprintf(stderr, "only one subcommand is allowed\n");
                return 1;
            }
            cli->has_cmd = 1;
            cli->enable = (strcmp(argv[i], "enable") == 0);
            cli->enable_all = (strcmp(argv[i], "enableall") == 0);
            cli->disable_all = (strcmp(argv[i], "disableall") == 0);
            if (cli->enable_all) {
                cli->enable = 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(stdout);
            exit(0);
        } else if (cli->has_cmd) {
            char **next = realloc(cli->patterns, (size_t)(cli->pattern_count + 1) * sizeof(char *));
            if (!next) {
                die("out of memory");
            }
            cli->patterns = next;
            cli->patterns[cli->pattern_count++] = argv[i];
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 1;
        }
    }

    if (cli->has_cmd && !cli->disable_all && !cli->enable_all && cli->pattern_count <= 0) {
        fprintf(stderr, "missing patterns for '%s'\n", cli->enable ? "enable" : "disable");
        return 1;
    }
    if ((cli->disable_all || cli->enable_all) && cli->pattern_count > 0) {
        fprintf(stderr, "'%s' does not accept patterns\n", cli->enable_all ? "enableall" : "disableall");
        return 1;
    }

    if (!cli->list) {
        if (!cli->has_cmd) {
            fprintf(stderr, "specify --list or a subcommand\n");
            return 1;
        }
    }

    if (cli->list && cli->has_cmd) {
        fprintf(stderr, "--list cannot be combined with subcommands\n");
        return 1;
    }
    if (cli->list && cli->force) {
        fprintf(stderr, "--force cannot be combined with --list\n");
        return 1;
    }

    return 0;
}

static void scan_dir_skills(const char *dir, int enabled, SkillVec *skills) {
    DIR *d = opendir(dir);
    struct dirent *ent;

    if (!d) {
        return;
    }

    while ((ent = readdir(d)) != NULL) {
        char *path;
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (ent->d_name[0] == '.') {
            continue;
        }

        path = spc_path_join(dir, ent->d_name);
        if (is_dir_path(path)) {
            skill_vec_push(skills, spc_strdup(ent->d_name), enabled);
        }
        free(path);
    }

    closedir(d);
}

static void run_list(const SkillVec *skills) {
    size_t i;
    size_t enabled = 0;
    size_t disabled = 0;

    for (i = 0; i < skills->len; i++) {
        if (skills->items[i].enabled) {
            enabled++;
            printf("%s[x]%s %s\n", GREEN, RESET, skills->items[i].name);
        } else {
            disabled++;
            printf("%s[ ]%s %s\n", RED, RESET, skills->items[i].name);
        }
    }

    printf("\n%sEnabled:%s %zu  %sDisabled:%s %zu  %sTotal:%s %zu\n",
           BOLD,
           RESET,
           enabled,
           BOLD,
           RESET,
           disabled,
           BOLD,
           RESET,
           skills->len);
}

static int exists_path(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int rm_rf(const char *path) {
    struct stat st;

    if (lstat(path, &st) != 0) {
        return errno == ENOENT ? 0 : -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        struct dirent *ent;

        if (!d) {
            return -1;
        }

        while ((ent = readdir(d)) != NULL) {
            char *child;
            int rc;

            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            child = spc_path_join(path, ent->d_name);
            rc = rm_rf(child);
            free(child);
            if (rc != 0) {
                closedir(d);
                return -1;
            }
        }

        closedir(d);
        return rmdir(path);
    }

    return unlink(path);
}

static int atomic_batch_move(const MoveVec *moves, int *rolled_back, int force) {
    size_t i;
    *rolled_back = 0;

    for (i = 0; i < moves->len; i++) {
        if (!exists_path(moves->items[i].src)) {
            fprintf(stderr, "  %sabort%s source missing: %s\n", DIM, RESET, moves->items[i].src);
            return 1;
        }
        if (!force && exists_path(moves->items[i].dst)) {
            fprintf(stderr, "  %sabort%s destination already exists: %s\n", DIM, RESET, moves->items[i].dst);
            return 1;
        }
    }

    for (i = 0; i < moves->len; i++) {
        if (force && exists_path(moves->items[i].dst)) {
            if (rm_rf(moves->items[i].dst) != 0) {
                fprintf(stderr,
                        "  %sfailed%s remove existing destination %s (%s)\n",
                        RED,
                        RESET,
                        moves->items[i].dst,
                        strerror(errno));
                return 1;
            }
        }

        if (rename(moves->items[i].src, moves->items[i].dst) != 0) {
            size_t j;
            fprintf(stderr,
                    "  %sfailed%s %s -> %s (%s)\n",
                    RED,
                    RESET,
                    moves->items[i].src,
                    moves->items[i].dst,
                    strerror(errno));

            for (j = i; j > 0; j--) {
                if (rename(moves->items[j - 1].dst, moves->items[j - 1].src) == 0) {
                    (*rolled_back)++;
                }
            }
            return 1;
        }
    }

    return 0;
}

static void run_batch(const SkillVec *skills,
                      const char *skills_dir,
                      const char *disabled_dir,
                      const Cli *cli) {
    MoveVec moves;
    size_t i;
    int skipped = 0;

    move_vec_init(&moves);

    for (i = 0; i < skills->len; i++) {
        int matched = 0;
        int p;
        const Skill *s = &skills->items[i];

        if (cli->disable_all || cli->enable_all) {
            matched = 1;
        } else {
            for (p = 0; p < cli->pattern_count; p++) {
                if (spc_glob_match_ci(cli->patterns[p], s->name)) {
                    matched = 1;
                    break;
                }
            }
        }

        if (!matched) {
            continue;
        }

        if ((cli->enable && s->enabled) || (!cli->enable && !s->enabled)) {
            printf("  %sskip%s %s (already %s)\n",
                   DIM,
                   RESET,
                   s->name,
                   cli->enable ? "enabled" : "disabled");
            skipped++;
            continue;
        }

        {
            char *src = spc_path_join(cli->enable ? disabled_dir : skills_dir, s->name);
            char *dst = spc_path_join(cli->enable ? skills_dir : disabled_dir, s->name);

            if (exists_path(dst)) {
                if (cli->force) {
                    printf("  %sreplace%s %s (destination exists)\n", DIM, RESET, s->name);
                } else {
                    printf("  %sskip%s %s (destination already exists)\n", DIM, RESET, s->name);
                    skipped++;
                    free(src);
                    free(dst);
                    continue;
                }
            }

            move_vec_push(&moves, src, dst, spc_strdup(s->name));

            if (cli->dry_run) {
                printf("  %swould mv%s '%s' -> '%s'\n", DIM, RESET, src, dst);
            }
        }
    }

    if (moves.len == 0 && skipped == 0) {
        fprintf(stderr, "No skills matched the pattern(s).\n");
        move_vec_free(&moves);
        exit(1);
    }

    printf("\n");

    if (cli->dry_run) {
        printf("%sDry run:%s %zu would change, %d already correct\n", DIM, RESET, moves.len, skipped);
        move_vec_free(&moves);
        return;
    }

    if (moves.len == 0) {
        printf("%sNothing to do%s (%d already correct)\n", DIM, RESET, skipped);
        move_vec_free(&moves);
        return;
    }

    {
        int rolled_back = 0;
        if (atomic_batch_move(&moves, &rolled_back, cli->force) != 0) {
            if (cli->force) {
                fprintf(stderr,
                        "%sFailed%s - partial changes may have been made when using --force\n",
                        RED,
                        RESET);
            } else if (rolled_back > 0) {
                fprintf(stderr,
                        "%sFailed%s - rolled back %d move(s), no changes were made\n",
                        RED,
                        RESET,
                        rolled_back);
            } else {
                fprintf(stderr, "%sFailed%s - no changes were made\n", RED, RESET);
            }
            move_vec_free(&moves);
            exit(1);
        }
    }

    for (i = 0; i < moves.len; i++) {
        printf("  %s%s%s %s\n",
               cli->enable ? GREEN : RED,
               cli->enable ? "enabled" : "disabled",
               RESET,
               moves.items[i].name);
    }

    printf("%sApplied %zu %s%s, %d skipped\n",
           GREEN,
           moves.len,
           moves.len == 1 ? "change" : "changes",
           RESET,
           skipped);

    move_vec_free(&moves);
}

int main(int argc, char **argv) {
    Cli cli;
    const char *home;
    char *claude_dir;
    char *skills_dir;
    char *disabled_dir;
    SkillVec skills;

    if (parse_cli(argc, argv, &cli) != 0) {
        usage(stderr);
        free(cli.patterns);
        return 1;
    }

    home = getenv("HOME");
    if (!home || home[0] == '\0') {
        die("HOME is not set");
    }

    claude_dir = spc_path_join(home, ".claude");
    skills_dir = spc_path_join(claude_dir, "skills");
    disabled_dir = spc_path_join(skills_dir, ".disabled");

    free(claude_dir);

    if (!is_dir_path(skills_dir)) {
        fprintf(stderr, "Skills directory not found: %s\n", skills_dir);
        free(cli.patterns);
        free(skills_dir);
        free(disabled_dir);
        return 1;
    }

    if (!is_dir_path(disabled_dir)) {
        if (mkdir(disabled_dir, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "failed to create directory: %s (%s)\n", disabled_dir, strerror(errno));
            free(cli.patterns);
            free(skills_dir);
            free(disabled_dir);
            return 1;
        }
    }

    skill_vec_init(&skills);
    scan_dir_skills(skills_dir, 1, &skills);
    scan_dir_skills(disabled_dir, 0, &skills);

    if (skills.len == 0) {
        fprintf(stderr, "No skills found\n");
        free(cli.patterns);
        skill_vec_free(&skills);
        free(skills_dir);
        free(disabled_dir);
        return 1;
    }

    qsort(skills.items, skills.len, sizeof(Skill), skill_cmp);

    if (cli.list) {
        run_list(&skills);
    } else if (cli.has_cmd) {
        run_batch(&skills, skills_dir, disabled_dir, &cli);
    }

    skill_vec_free(&skills);
    free(cli.patterns);
    free(skills_dir);
    free(disabled_dir);
    return 0;
}
