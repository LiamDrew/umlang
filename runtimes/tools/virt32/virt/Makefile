CC = clang
CFLAGS = -g -Wall -Wextra -Werror -Wpedantic -O2

um: emulator.o driver.o
	$(CC) $(CFLAGS) -o um emulator.o driver.o

all: virt

emulator.o: emulator.c
	$(CC) $(CFLAGS) -c emulator.c

driver.o: driver.c
	$(CC) $(CFLAGS) -c driver.c


clean:
	rm -f *.o virt test_stack test_recycler test_mem

# test: test_stack test_recycler test_mem

# virt: main.o driver.o recycler.o
# 	$(CC) $(CFLAGS) -o virt main.o driver.o recycler.o

# test_stack: stack_test.o
# 	$(CC) $(CFLAGS) -o test_stack stack_test.o

# test_recycler: recycler_test.o recycler.o driver.o
# 	$(CC) $(CFLAGS) -o test_recycler recycler_test.o recycler.o driver.o

# test_mem: mem_test.o driver.o recycler.o
# 	$(CC) $(CFLAGS) -o test_mem mem_test.o driver.o recycler.o

# mem_test.o: mem_test.c
# 	$(CC) $(CFLAGS) -c mem_test.c

# stack_test.o: stack_test.c
# 	$(CC) $(CFLAGS) -c stack_test.c

# testall:
# 	./test_stack 
# 	./test_recycler


