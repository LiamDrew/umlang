CC = clang #gcc-14 is a little bit faster but not natively supported on macOS
CFLAGS = -Wall -Wextra -Werror -O3
LDFLAGS =

um: emulator.c
	$(CC) $(CFLAGS) -o um emulator.c $(LDFLAGS)

clean:
	rm -f *.o um
	