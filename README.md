# ck_rhs-oom-reproducer
Project to reproduce possible bug in ck_rhs on OOM

## Simply compile and run.

```bash
make
./debug-dynamic
```
## Various options you can pass

```bash
USAGE:
    ./debug-dynamic [FLAGS]

FLAGS:
    --nofail         don't fail to allocate
    --read-mostly    set read mostly mode
    --silent         don't print status messages
    --reverse        Delete from back to front
    --djb3           Use the djb3 hash function
```
