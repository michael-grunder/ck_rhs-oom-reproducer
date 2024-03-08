#include <stdio.h>
#include <ck_rhs.h>
#include <stdlib.h>
#include <string.h>

typedef struct opts {
    bool fail_to_allocate;
    bool read_mostly;
    bool silent;
    bool reverse;
    bool djb3;
} opts;;

static const char *key[] = {
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
};

static opts g_opts = {true, false, false, false, false};

#define logStatus(fmt, ...) \
    do { \
        if (g_opts.silent != true) { \
            fprintf(stderr, fmt "\n", __VA_ARGS__); \
        } \
    } while (0)

/* Allow the initial map allocation, then fail */
static void *rhs_malloc(size_t size) {
    static int allocated = 0;

    if (g_opts.fail_to_allocate && allocated++) {
        logStatus("    Failing to allocate %zu bytes", size);
        return NULL;
    }

    return malloc(size);
}

static void rhs_free(void *ptr, size_t size, bool defer) {
    (void)size;
    (void)defer;
    free(ptr);
}

static struct ck_malloc rhs_allocators = {
    .malloc = rhs_malloc,
    .free = rhs_free,
    .realloc = NULL,
};

static bool rhs_cmp(const void *a, const void *b) {
    return !strcmp(a, b);
}

unsigned long adderHash(const void *p, unsigned long seed) {
    unsigned long h = 0;
    const char *key = p;
    (void)seed;

    if (strlen(key) != 1 || !strpbrk(key, "012345678")) {
        logStatus("    '%s' is not a proper toy key", (const char *)p);
        exit(1);
    }

    while (*key) {
        h += *key++ - '0';
    }

    return h;
}

unsigned long djb3(const void *p, unsigned long seed) {
    unsigned long hash = 5381;
    const char *key = p;

    while (*key) {
        hash = ((hash << 5) + hash) + *key++;
    }

    return hash ^ seed;
}

static unsigned long hashFn(const void *p, unsigned long seed) {
    unsigned long h = g_opts.djb3 ? djb3(p, seed) : adderHash(p, seed);

    //logStatus("    '%s' hashes to %lu (%s)", (const char *)p, h,
    //          g_opts.djb3 ? "djb3" : "adder");

    return h;
}

static void printUsage(const char *prog) {
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "    %s [FLAGS]\n\n", prog);
    fprintf(stderr, "FLAGS:\n");
    fprintf(stderr, "    --nofail         don't fail to allocate\n");
    fprintf(stderr, "    --read-mostly    set read mostly mode\n");
    fprintf(stderr, "    --silent         don't print status messages\n");
    fprintf(stderr, "    --reverse        Delete from back to front\n");
    fprintf(stderr, "    --djb3           Use the djb3 hash function\n");
    exit(1);
}

void parseOpts(int argc, char *argv[]) {
    const char *program = argv[0];

    argc--; argv++;

    while (argc > 0) {
        if (!strcasecmp(argv[0], "--nofail")) {
            g_opts.fail_to_allocate = false;
        } else if (!strcasecmp(argv[0], "--read-mostly")) {
            g_opts.read_mostly = true;
        } else if (!strcasecmp(argv[0], "--silent")) {
            g_opts.silent = true;
        } else if (!strcasecmp(argv[0], "--reverse")) {
            g_opts.reverse = true;
        } else if (!strcasecmp(argv[0], "--djb3")) {
            g_opts.djb3 = true;
        } else {
            printUsage(program);
            exit(1);
        }

        argc--; argv++;
    }
}

static void reverseKeys(char **key, size_t n) {
    for (size_t i = 0; i < n / 2; i++) {
        char *tmp = key[i];
        key[i] = key[n - 1 - i];
        key[n - 1 - i] = tmp;
    }
}

int main(int argc, char **argv) {
    char *old, *inset[10], *deleted;
    ck_rhs_iterator_t it;
    unsigned long h;
    ck_rhs_t rhs;
    size_t n = 0;
    int flags;

    parseOpts(argc, argv);

    logStatus("fail_to_allocate: %s, read_mostly: %s, reverse: %s",
              g_opts.fail_to_allocate ? "yes" : "no",
              g_opts.read_mostly ? "yes" : "no",
              g_opts.reverse ? "yes" : "no");

    flags = CK_RHS_MODE_OBJECT;
    if (g_opts.read_mostly)
        flags |= CK_RHS_MODE_READ_MOSTLY;

    if (ck_rhs_init(&rhs, flags, hashFn, rhs_cmp,
                    &rhs_allocators, 8, 0) == false)
    {
        logStatus("%s", "ck_rhs_init failed");
        exit(1);
    }

    logStatus("--- Adding %zu elements ---", sizeof(key) / sizeof(*key));
    for (size_t i = 0; i < sizeof(key) / sizeof(*key); i++) {
        struct ck_rhs_stat stat;
        ck_rhs_stat(&rhs, &stat);

        h = hashFn(key[i], 0);
        logStatus("[%zu] Attmpting to add '%s'@%p", i, key[i], key[i]);
        if (ck_rhs_set(&rhs, h, key[i], (void**)&old) == false)
            logStatus("    Failed to insert: '%s'@%p", key[i], key[i]);
        if (old != NULL) {
            logStatus("%s", "    We shouldn't see any duplicates!");
            exit(1);
        }
    }

    logStatus("--- Iterating over ck_rhs_t with %zu elements ---",
              ck_rhs_count(&rhs));
    ck_rhs_iterator_init(&it);
    while (ck_rhs_next(&rhs, &it, (void**)&inset[n]) == true) {
        logStatus("[%zu] '%s'@%p", n, inset[n], inset[n]);
        n++;
    }

    if (g_opts.reverse)
        reverseKeys(inset, n);

    logStatus("--- Removing %zu elements ---", n);
    for (size_t i = 0; i < n; i++) {
        h = hashFn(inset[i], 0);
        logStatus("[%zu] Attempting to remove '%s'@%p", i, inset[i], inset[i]);
        deleted = ck_rhs_remove(&rhs, h, inset[i]);
        if (deleted == inset[i]) {
            logStatus("    OK '%s'@%p == '%s'@%p",
                      deleted, deleted, inset[i], inset[i]);
        } else if (deleted == NULL) {
            logStatus("    ERR Failed to remove '%s'@%p (not found)",
                    inset[i], inset[i]);
            ck_rhs_destroy(&rhs);
            exit(1);
        } else {
            logStatus("    ERR Failed to remove '%s'@%p != '%s'@%p",
                    deleted, deleted, inset[i], inset[i]);
            ck_rhs_destroy(&rhs);
            exit(1);
        }
    }

    return 0;
}
