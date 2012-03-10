all:
	gcc -DALIGN_8 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 ewah5.c -o ewah8.so
	gcc -DALIGN_16 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 ewah5.c -o ewah16.so
	gcc -DALIGN_32 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 ewah5.c -o ewah32.so
	gcc -DALIGN_64 -Wall -Wextra -O -ansi -pedantic -shared -std=c99 ewah5.c -o ewah64.so

