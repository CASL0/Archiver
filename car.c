#include "car.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//32ビットCRC値を計算するためのCRCテーブル
static unsigned int C32Table[256];
//コマンドラインで指定されたCARファイルの名前
static char CarFileName[FILENAME_MAX];
//入力CARファイル
static FILE *InputCarFile;
//出力用CARファイルはまず一時的な名前でオープンされる
static char TmpFileName[FILENAME_MAX];
//出力用CARファイル
static FILE *OutputCarFile;
//アーカイブするファイルのリスト
static char *FileList[FILE_LIST_MAX];
//現在処理中のファイルのヘッダ
static HEADER Header;

 void usage(void){
	fprintf(stderr,"CAR -- Compressed ARchiver\n\n"); 
	fprintf(stderr,"usage: car command car-file [file ...]\n\n");
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

int ParseArguments(int argc,char *argv[]){
	if(argc<3 || strlen(argv[1])>1){
		usage();	
	}
	
	int command;
	switch(command=tolower(argv[1][0])){
		case 'x':
			fprintf(stderr,"ファイルの取り出し\n");	
			break;
		case 'r':
			fprintf(stderr,"ファイルの置換\n");	
			break;
		case 'p':
			fprintf(stderr,"ファイルの表示\n");	
			break;
		case 'd':
			if(argc<=3){
				usage();	
			}
			fprintf(stderr,"ファイルをアーカイブから削除\n");
			break;
		case 'a':
			if(argc<=3){
				usage();	
			}
			fprintf(stderr,"ファイルをアーカイブに追加\n");
			break;
		default:
			usage();
	}
	return command;
}

void OpenArchiveFiles(char *name, int command){
	strncpy(CarFileName,name,FILENAME_MAX-1);
	CarFileName[FILENAME_MAX-1]='\0';	
	InputCarFile=fopen(CarFileName,"rb");

	char *s;
	//拡張子を補完する
	if(InputCarFile==NULL){
		//ファイル名までパスを下る
		s=strrchr(CarFileName,'/');	
		if(s==NULL){
			s=CarFileName;
		}
		if(strrchr(s,'.')==NULL){
			if(strlen(CarFileName)<(FILENAME_MAX - 4)){
				strcat(CarFileName,".car");
				InputCarFile=fopen(CarFileName,"rb");	
			}
		}
	}
	if(InputCarFile==NULL && command != 'a'){
		fprintf(stderr,"アーカイブ%sを開けませんでした\n",CarFileName);	
		exit(1);
	}
	if(command=='a' || command=='r' || command=='d'){
		strcpy(TmpFileName,CarFileName);
		strcat(TmpFileName,".tmp");	
		OutputCarFile=fopen(TmpFileName,"wb");	
		if(OutputCarFile==NULL){
			fprintf(stderr,"一時ファイル%sを開けませんでした\n",TmpFileName);
			exit(1);	
		}
	}
}

void BuildFileList(int argc, char *argv[], int command){
	int count=0;
	if(argc==0){
		FileList[count++]="*";	
	}else{
		for(int i=0;i<argc;i++){
			FileList[count]=malloc(strlen(argv[i])+2);	
			if(FileList[count]==NULL){
				fprintf(stderr,"ファイル名が長すぎます\n");
				exit(1);	
			}
			strcpy(FileList[count++],argv[i]);	
			if(count>=FILE_LIST_MAX){
				fprintf(stderr,"ファイルが多すぎます");
				exit(1);	
			}
		}
	}
	FileList[count]=NULL;	
}

int AddFileList(void){
	FILE *input_text_file;	
	int i=0;
	for(;FileList[i]!=NULL;i++){
		input_text_file=fopen(FileList[i],"rb");
		if(input_text_file==NULL){
			fprintf(stderr,"入力ファイル%sを開けませんでした\n",FileList[i]);	
			exit(1);	
		}
		//ファイル名の先頭を指すようにパスを下る	
		char *s=strrchr(FileList[i],'/');
		if(s!=NULL){
			s++;	
		}else{
			s=FileList[i];	
		}
		//パスで入力されていた場合ファイル名以外を消去	
		if(s!=FileList[i]){
			int j=0;
			for(;s[j]!='\0';j++){
				FileList[i][j]=s[j];	
			}
			FileList[i][j]='\0';
		}
		//ファイルの重複をチェックする
		int skip=0;
		for(int j=0;j<i;j++){
			if(strcmp(FileList[j],s)==0){
				fprintf(stderr,"ファイル%sが重複しています\n",FileList[i]);	
				skip=1;	
				break;	
			}
		}
		if(skip){
			fclose(input_text_file);	
		}else{
			strcpy(Header.file_name,FileList[i]);
			insert(input_text_file,"追加");	
		}
	}
	return i;
}

void insert(FILE *input_text_file,char *operation){
}
