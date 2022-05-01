CC = cc
CFLAGS = -Wall

test: test.o kthread.o spinlock.o
	$(CC) $(CFLAGS) -o test test.o kthread.o spinlock.o

testmany: test.o kthread.o 
	$(CC) $(CFLAGS) -o testmany test.o kthread.o 

test.o: 
	$(CC) $(CFLAGS) -c testing/test.c

kthread.o : one-one/kthread.c one-one/kthread.h
	$(CC) $(CFLAGS) -c one-one/kthread.c

spinlock.o : one-one/spinlock.c one-one/spinlock.h
	$(CC) $(CFLAGS) -c one-one/spinlock.c

clean:
	rm -f *.o test