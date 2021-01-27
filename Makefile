
progetto: main.o map.o source.o taxi.o utils.o
	gcc main.o map.o source.o taxi.o utils.o -o progetto


main.o: main.c
	gcc -c main.c

map.o: map.c map.h
	gcc -c map.c

source.o: source.c source.h
	gcc -c source.c

taxi.o: taxi.c taxi.h
	gcc -c taxi.c

utils.o: utils.c utils.h
	gcc -c utils.c

clean:
	rm *.o progetto
