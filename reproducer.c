#include <stdio.h>
#include <ck_rhs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

typedef struct opts {
    bool fail_to_allocate;
    bool read_mostly;
    size_t limit;
    bool silent;
    bool verbose;
} opts;

static const void *key[] = {
    (void*)0x1,
    (void*)0x2,
    (void*)0x3,
    (void*)0x4,
    (void*)0x5,
    (void*)0x6,
    (void*)0x9,
    (void*)0xa,
    (void*)0xb,
};

static opts g_opts = {
    .fail_to_allocate = true
};

#define logStatus(fmt, ...) \
    do { \
        if (g_opts.silent != true) { \
            fprintf(stderr, fmt "\n", __VA_ARGS__); \
        } \
    } while (0)

#define panic(fmt, ...) \
    do { \
        fprintf(stderr, fmt "\n", __VA_ARGS__); \
        exit(1); \
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
    if (g_opts.verbose)
        logStatus("    %p %s %p", a, a == b ? "==" : "!=", b);

    return a == b;
}

static unsigned long hashFn(const void *p, unsigned long seed) {
    (void)seed;
    return (uintptr_t)p;
}

static void printUsage(const char *prog) {
    fprintf(stderr, "USAGE:\n");
    fprintf(stderr, "    %s [FLAGS]\n\n", prog);
    fprintf(stderr, "FLAGS:\n");
    fprintf(stderr, "    --nofail         don't fail to allocate\n");
    fprintf(stderr, "    --read-mostly    set read mostly mode\n");
    fprintf(stderr, "    --silent         don't print status messages\n");
    fprintf(stderr, "    --limit          Limit keys added\n");
    fprintf(stderr, "    --verbose        print individual rhs comparisons\n");
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
        } else if (!strcasecmp(argv[0], "--verbose")) {
            g_opts.verbose = true;
        } else if (!strcasecmp(argv[0], "--limit") && argc > 1) {
            g_opts.limit = strtoul(argv[1], NULL, 10);
            argc--; argv++;
        } else {
            printUsage(program);
            exit(1);
        }

        argc--; argv++;
    }
}

int main(int argc, char **argv) {
    void *old, *inset[10], *deleted;
    ck_rhs_iterator_t it;
    size_t n = 0, limit;
    int flags, res = 0;
    unsigned long h;
    ck_rhs_t rhs;

    parseOpts(argc, argv);

    logStatus("fail_to_allocate: %s, read_mostly: %s, limit: %zu",
              g_opts.fail_to_allocate ? "yes" : "no",
              g_opts.read_mostly ? "yes" : "no",
              g_opts.limit);

    flags = CK_RHS_MODE_DIRECT;
    if (g_opts.read_mostly)
        flags |= CK_RHS_MODE_READ_MOSTLY;

    if (ck_rhs_init(&rhs, flags, hashFn, rhs_cmp,
                    &rhs_allocators, 8, 0) == false)
    {
        panic("%s", "ck_rhs_init failed");
        return 1;
    }

    limit = sizeof(key) / sizeof(*key);
    if (g_opts.limit && g_opts.limit < limit)
        limit = g_opts.limit;

    logStatus("--- Adding %zu elements ---", sizeof(key) / sizeof(*key));
    for (size_t i = 0; i < limit; i++) {
        struct ck_rhs_stat stat;
        ck_rhs_stat(&rhs, &stat);

        h = hashFn(key[i], 0);
        logStatus("[%zu] Attmpting to add %p", i, key[i]);
        if (ck_rhs_set(&rhs, h, key[i], (void**)&old) == false)
            logStatus("    Failed to insert: %p", key[i]);
        if (old != NULL) {
            panic("%s", "    We shouldn't see any duplicates!");
        }
    }

    ck_rhs_iterator_init(&it);
    while (ck_rhs_next(&rhs, &it, (void**)&inset[n]) == true) {
        n++;
    }

    logStatus("--- Removing %zu elements ---", n);

    for (size_t i = 0; res == 0 && i < n; i++) {
        logStatus("[%zu] Attempting to remove %p", i, inset[i]);
        deleted = ck_rhs_remove(&rhs, hashFn(inset[i], 0), inset[i]);
        if (deleted == inset[i]) {
            logStatus("    OK %p == %p", deleted, inset[i]);
        } else if (deleted == NULL) {
            logStatus("    ERR Failed to remove %p (not found)", inset[i]);
            res = 1;
        } else {
            logStatus("    ERR Failed to remove %p != %p", deleted, inset[i]);
            res = 1;
        }
    }

    ck_rhs_destroy(&rhs);
    return res;
}
