CC = clang
CFLAGS = -g -Wall -Wextra -Werror -Wpedantic -O2

um: mod_emulator.o virt.o
	$(CC) $(CFLAGS) -o um mod_emulator.o virt.o

mod_emulator.o: mod_emulator.c
	$(CC) $(CFLAGS) -c mod_emulator.c

virt.o: ../virt/virt.c ../virt/virt.h
	$(CC) $(CFLAGS) -c ../virt/virt.c

clean:
	rm -f *.o um