CC = clang
CFLAGS = -g -Wall -Wextra -Werror -Wpedantic -O2

um: emulator.o virt.o
	$(CC) $(CFLAGS) -o um emulator.o virt.o

emulator.o: emulator.c
	$(CC) $(CFLAGS) -c emulator.c

virt.o: virt.c
	$(CC) $(CFLAGS) -c virt.c


clean:
	rm -f *.o um