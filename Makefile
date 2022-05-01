CC = cc
CFLAGS = -Wall

default: init
.PHONY: init oneone manyone manymany tests runtest clean
init:
	if [ ! -d bin ]; \
	then \
		mkdir bin; \
	fi \
	
	if [ ! -d test ]; \
	then \
		mkdir test; \
	fi \
	
	if [ ! -d bin/one-one ]; \
	then \
		mkdir bin/one-one; \
	fi \

	if [ ! -d bin/many-one ]; \
	then \
		mkdir bin/many-one; \
	fi \

	if [ ! -d bin/many-many ]; \
	then \
		mkdir bin/many-many; \
	fi \

	if [ ! -d test/one-one ]; \
	then \
		mkdir test/one-one; \
	fi \

	if [ ! -d test/many-one ]; \
	then \
		mkdir test/many-one; \
	fi \

	if [ ! -d test/many-many ]; \
	then \
		mkdir test/many-many; \
	fi \

	@make oneone
	@make manyone
	@make manymany
	@make tests

oneone:
	$(CC) -c $(CFLAGS) one-one/*.c
	$(CC) -c $(CFLAGS) one-one/tests/*.c
	@mv *.o bin/one-one

manyone:
	$(CC) -c $(CFLAGS) many-one/*.c
	$(CC) -c $(CFLAGS) many-one/tests/*.c
	@mv *.o bin/many-one

manymany:
	$(CC) -c $(CFLAGS) many-many/*.c
	$(CC) -c $(CFLAGS) many-many/tests/*.c
	@mv *.o bin/many-many

tests: one-one/*.c many-one/*.c many-one/*.c one-one/tests/*.c many-one/tests/*.c many-many/tests/*.c 
	$(CC) $(CFLAGS) bin/one-one/matrixmulti.o bin/one-one/kthread.o bin/one-one/spinlock.o -o matrixmulti
	$(CC) $(CFLAGS) bin/one-one/mergeSort.o bin/one-one/kthread.o bin/one-one/spinlock.o -o mergeSort
	$(CC) $(CFLAGS) bin/one-one/test.o bin/one-one/kthread.o bin/one-one/spinlock.o -o tests
	@mv tests matrixmulti mergeSort test/one-one
	
	$(CC) $(CFLAGS) bin/many-one/matrixmulti.o bin/many-one/kthread.o -o matrixmulti
	$(CC) $(CFLAGS) bin/many-one/mergeSort.o bin/many-one/kthread.o -o mergeSort
	$(CC) $(CFLAGS) bin/many-one/test.o bin/many-one/kthread.o -o tests
	@mv tests matrixmulti mergeSort test/many-one

	$(CC) $(CFLAGS) bin/many-many/matrixmulti.o bin/many-many/kthread.o bin/many-many/spinlock.o -o matrixmulti
	$(CC) $(CFLAGS) bin/many-many/mergeSort.o bin/many-many/kthread.o bin/many-many/spinlock.o -o mergeSort
	$(CC) $(CFLAGS) bin/many-many/test.o bin/many-many/kthread.o bin/many-many/spinlock.o -o tests
	@mv tests matrixmulti mergeSort test/many-many

runtest:
	chmod +x testrun.sh
	@make init
	./testrun.sh

clean:
	@rm -r bin
	@rm -r test