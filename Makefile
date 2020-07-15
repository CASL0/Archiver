OBJS=main.o car.o   
INCLUDE=-I./external/lz4/lib/ -I./external/zlib/ -I./external/zstd/lib/
LIBDIR=-L./external/lz4/lib/ -L./external/zlib/ -L./external/zstd/lib/
LIBS=-llz4 -lz -lzstd
SUBMODULE=submodule_zlib submodule_lz4 submodule_zstd
all:$(OBJS) $(SUBMODULE) 
	gcc -Wall -ocar $(OBJS) $(INCLUDE) $(LIBDIR) $(LIBS)  
main.o:main.c car.h
	gcc main.c -Wall -c $(INCLUDE) 
car.o:car.c
	gcc car.c -Wall -c $(INCLUDE) 
car.o:car.h
clean:
	rm -f *.o car
	cd external/lz4/lib && $(MAKE) clean 
	cd external/zlib && ./configure && $(MAKE) clean 
	cd external/zstd/lib && $(MAKE) clean  
submodule_zlib:
	cd external/zlib && ./configure && $(MAKE) libz.a
submodule_lz4:
	cd external/lz4/lib && $(MAKE) liblz4.a
submodule_zstd:
	cd external/zstd/lib && $(MAKE) libzstd.a 
