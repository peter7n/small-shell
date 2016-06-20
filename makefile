all: smallsh

smallsh: dynamicArray.o smallsh.o
	gcc -g -Wall -o smallsh dynamicArray.o smallsh.o
	
smallsh.o: smallsh.c dynamicArray.h
	gcc -g -Wall -c smallsh.c
	
dynamicArray.o: dynamicArray.c dynamicArray.h
	gcc -g -Wall -c dynamicArray.c

clean:	
	rm dynamicArray.o
	rm smallsh.o
	rm smallsh
