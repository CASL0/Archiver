OBJ=main.o car.o lzss.o lz4.o 
all:${OBJ}
	gcc ${OBJ} -ocar
main.o:
	gcc main.c -c
car.o:
	gcc car.c -c 
lzss.o:
	gcc lzss.c -c
lz4.o:
	gcc ./third-party/lz4.c -c
clean:
	rm -f *.o car 
