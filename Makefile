OBJ=main.o car.o   
LIBS=./externals/lz4/lib/liblz4.a ./externals/zlib/libz.a ./externals/zstd/lib/libzstd.a
SUBMODULE=submodule_zlib submodule_lz4 submodule_zstd
all:${OBJ} ${SUBMODULE} 
	gcc -ocar ${LIBS} ${OBJ} 
main.o:main.c car.h
	gcc main.c -c
car.o:car.c
	gcc car.c -c 
car.o:car.h
clean:
	rm -f *.o car
	cd externals/lz4/lib && ${MAKE} clean 
	cd externals/zlib && ./configure && ${MAKE} clean 
	cd externals/zstd/lib && ${MAKE} clean  
submodule_zlib:
	cd externals/zlib && ./configure && ${MAKE} libz.a
submodule_lz4:
	cd externals/lz4/lib && ${MAKE} liblz4.a
submodule_zstd:
	cd externals/zstd/lib && ${MAKE} libzstd.a 
