CFLAGS=-g
LIBS=~cpen212/Public/lab3/lib/lib212alloc.a -lm

%.o: %.c cpen212alloc.h cpen212common.h
	$(CC) $(CFLAGS) -c -o $@ $<

cpen212alloc: cpen212alloc.o cpen212debug.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	$(RM) *.o cpen212alloc