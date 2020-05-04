#include "car.h"
#include <stdio.h>
#include <stdlib.h>


//32ビットCRC値を計算するためのCRCテーブル
static unsigned int C32Table[256];

 void usage(void){
	fprintf(stderr,"CAR -- Compressed ARchiver\n"); 
	fprintf(stderr,"usage: car command car-file [file ...]\n");
	fprintf(stderr,"Commands: \n");
	fprintf(stderr,"  a：ファイルをアーカイブに追加する\n");
	fprintf(stderr,"  x：ファイルをアーカイブから取り出す\n");
	fprintf(stderr,"  r：アーカイブのファイルを置換する\n");
	fprintf(stderr,"  d：アーカイブのファイルを削除する\n");
	fprintf(stderr,"  p：アーカイブのファイルを表示する\n");
	fprintf(stderr,"\n");
	exit(1);
}

void BuildCRCTable(void){
	for(int i=0;i<=256;i++){
		unsigned int value=i;
		for(int j=8;j>0;j--){
			value=(value & 1)?(value >> 1)^CRC32_POLYNOMIAL:value>>1;
		}
		C32Table[i]=value;
	}
}

unsigned int CalculateCRC32(unsigned int count, unsigned int crc, void *buffer){
	unsigned char *p=(unsigned char*)buffer;
	unsigned int tmp1,tmp2;
	while(count--!=0){
		tmp1=(crc>>8) & 0x00ffffff;	
		tmp2=C32Table[((int)crc^*p++) & 0xff];
		crc=tmp1^tmp2;
	}
	return crc;
}
