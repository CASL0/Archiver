#include "car.h"
#include <stdio.h>


 void usage(void){
	fprintf(stderr,"CAR -- Compressed ARchiver\n"); 
	fprintf(stderr,"usage: car command car-file [file ...]\n");
	fprintf(stderr,"Commands: \n");
	fprintf(stderr,"  a：ファイルをアーカイブに追加する\n");
	fprintf(stderr,"  x：ファイルをアーカイブから取り出す\n");
	fprintf(stderr,"  r：アーカイブのファイルを置換する\n");
	fprintf(stderr,"  d：アーカイブのファイルを削除する\n")
	fprintf(stderr,"  p：アーカイブのファイルを表示する\n");
	fprintf(stderr,"\n");
	exit(1);
}

