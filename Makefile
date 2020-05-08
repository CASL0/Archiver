FILES=main.c car.c lzss.c

all:${FILES}
	gcc ${FILES} -ocar
clean:
	rm -f *.o car 
