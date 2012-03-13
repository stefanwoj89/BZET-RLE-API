all:
	gcc -DALIGN_8 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 gwah.c -o gwah8-1.so
	gcc -DALIGN_16 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 gwah.c -o gwah16-2.so
	gcc -DALIGN_32 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 gwah.c -o gwah32-4.so
	gcc -DALIGN_64 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 gwah.c -o gwah64-8.so

