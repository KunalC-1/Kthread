CC = cc
CFLAGS = -Wall

test: test.o kthread.o spinlock.o
	$(CC) $(CFLAGS) -o test test.o kthread.o spinlock.o

testmany: test.o kthread.o 
	$(CC) $(CFLAGS) -o test test.o kthread.o 


test.o: 
	$(CC) $(CFLAGS) -c testing/test.c

kthread.o : many-one/kthread.c many-one/kthread.h
	$(CC) $(CFLAGS) -c many-one/kthread.c

spinlock.o : many-one/spinlock.c many-one/spinlock.h
	$(CC) $(CFLAGS) -c many-one/spinlock.c

clean:
	rm -f core *.o test