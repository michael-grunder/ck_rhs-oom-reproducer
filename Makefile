CC = cc
CFLAGS ?= -g3 -Wall
SRC = reproducer.c
OBJ = $(SRC:.c=.o)
LIBCK_A=ck/src/libck.a

dynamic: debug-dynamic release-dynamic

all: debug-dynamic release-dynamic debug-static release-static

static: debug-static release-static

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug-dynamic: $(OBJ)
	$(CC) -O0 $(CFLAGS) -o $@ $^ -lck

release-dynamic: $(OBJ)
	$(CC) -O2 $(CFLAGS) -o $@ $^ -lck

debug-static: $(OBJ)
	$(CC) -O0 $(CFLAGS) -o $@ $^ $(LIBCK_A)

release-static: $(OBJ)
	$(CC) -O2 $(CFLAGS) -o $@ $^ $(LIBCK_A)

bear:
	bear -- make -j$(shell nproc)

clean:
	rm -rf $(OBJ) debug-* release-* *.o

