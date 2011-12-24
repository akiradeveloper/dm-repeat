sscanf:
	gcc -Wall sscanf.c
	./a.out

arg:
	gcc -g -O0 -Wall arg.c
	./a.out 0xff 0
	./a.out 0xff 1
	./a.out 0xff00 1
	./a.out 0xff00 0
	./a.out 0xa53E 0 # if capitalized included

eq:	
	gcc eq.c
	./a.out

s:
	gcc s.c
	./a.out
	
clean:
	rm ./a.out
