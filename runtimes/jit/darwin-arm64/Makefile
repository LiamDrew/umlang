CC = clang
CFLAGS = -g -Wall -Wextra -Werror -Wpedantic -O2
LDFLAGS =

jit: jit.o utility.o virt.o
	$(CC) $(CFLAGS) -o jit jit.o utility.o virt.o $(LDFLAGS)

jit.o: jit.c utility.h
	$(CC) $(CFLAGS) -c jit.c

utility.o: utility.S utility.h
	$(CC) -c utility.S

virt.o: ../../virt/virt.c ../../virt/virt.h
	$(CC) -c ../../virt/virt.c

.PHONY: clean
clean:
	rm -f *.o jit