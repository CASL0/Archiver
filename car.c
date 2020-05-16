#include "third-party/lz4.h"
#include "car.h"
#include "lzss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>

void usage(void){
	fprintf(stderr,"CAR -- Compressed ARchiver\n\n"); 
	fprintf(stderr,"usage: car [command] [car-file] [file ...]\n\n");
	fprintf(stderr,"commands: \n");
	fprintf(stderr,"  a：アーカイブにファイルを追加\n");
	fprintf(stderr,"  x：アーカイブからファイルを取り出す\n");
	fprintf(stderr,"  r：アーカイブ内のファイルを置換\n");
	fprintf(stderr,"  d：アーカイブ内のファイルを削除\n");
	fprintf(stderr,"  p：アーカイブ内のファイルを閲覧\n");
	fprintf(stderr,"  l：アーカイブの一覧表示\n");
	fprintf(stderr,"\n");
	exit(1);
}

void BuildCRCTable(void){
	for(int i=0;i<CRC_TABLE_SIZE;i++){
		uint32_t value=i;
		for(int j=0;j<8;j++){
			value=(value & 1)?(value >> 1)^CRC32_POLYNOMIAL:value>>1;
		}
		C32Table[i]=value;
	}
}

//CCITT-32の計算に基づきCRCを計算する
//引数crcをseedとして計算する
uint32_t CalculateCRC32(uint32_t count, uint32_t crc, void *buffer){
	unsigned char *p=(unsigned char*)buffer;
	while(count--!=0){
		crc=UpdateCharacterCRC32(crc,*p++);	
	}
	return crc;
}

uint32_t UpdateCharacterCRC32(uint32_t crc, int c){
	uint32_t tmp1=(crc>>8) & 0x00ffffff;
	uint32_t tmp2=C32Table[((int)crc^c)&0xff];
	crc=tmp1^tmp2;
	return crc;
}

//コマンドライン引数の解析をして，コマンドの種別を判定する
//引数の数に問題が合った場合はusageを表示する
int ParseArguments(int argc,char *argv[]){
	if(argc<3 || strlen(argv[1])>1){
		usage();	
	}
	
	int command;
	switch(command=tolower(argv[1][0])){
		case 'x':
			fprintf(stderr,"アーカイブからファイルを取り出す\n");	
			break;
		case 'r':
			fprintf(stderr,"アーカイブ内のファイルを置換\n");	
			break;
		case 'p':
			fprintf(stderr,"アーカイブ内のファイルを閲覧\n");	
			break;
		case 'd':
			if(argc<=3){
				usage();	
			}
			fprintf(stderr,"アーカイブからファイルを削除\n");
			break;
		case 'a':
			if(argc<=3){
				usage();	
			}
			fprintf(stderr,"アーカイブにファイルを追加\n");
			break;
		case 'l':
			fprintf(stderr,"アーカイブの一覧表示\n");
			break;	

		default:
			usage();
	}
	return command;
}

//アーカイブファイルをオープンする
//名前はnameで，どのようにオープンするかはcommandで指定する
//.car拡張子が指定されていなければ補完する
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
		//ファイル名にドットが無ければ，末尾に.carを連接する	
		if(strrchr(s,'.')==NULL){
			if(strlen(CarFileName)<(FILENAME_MAX - 4)){
				strcat(CarFileName,".car");
				InputCarFile=fopen(CarFileName,"rb");	
			}
		}
	}
	
	//追加コマンドの場合は新しくアーカイブファイルを作成することもあるのでNULLが許容される	
	if(InputCarFile==NULL && command != 'a'){
		fprintf(stderr,"アーカイブ%sを開けませんでした\n",CarFileName);	
		exit(1);
	}

	if(command=='a' || command=='r' || command=='d'){
		
		//出力アーカイブは一時的な名前でオープンする
		//拡張子.tmpとする
		//すべての処理後に.carにする		
		strcpy(TmpFileName,CarFileName);
		s=strrchr(TmpFileName,'.');
		if(s==NULL){
			s=TmpFileName+strlen(TmpFileName);
		}
		sprintf(s,".tmp");	
		OutputCarFile=fopen(TmpFileName,"wb");	
		if(OutputCarFile==NULL){
			fprintf(stderr,"一時ファイル%sを開けませんでした\n",TmpFileName);
			exit(1);	
		}
	}
}

//コマンドライン引数から，アーカイブへ書き込むファイルリストを作成する
void BuildFileList(int argc, char *argv[], int command){
	int count=0;
	if(argc==0){
		FileList[count++]="*";	
	}else{
		for(int i=0;i<argc;i++){
			FileList[count]=(char*)malloc(strlen(argv[i])+1);	
			if(FileList[count]==NULL){
				fprintf(stderr,"メモリの確保に失敗しました\n");
				exit(1);	
			}
			strcpy(FileList[count++],argv[i]);	
			if(count>=FILE_LIST_MAX){
				fprintf(stderr,"ファイルが多すぎます\n");
				exit(1);	
			}
		}
	}
	FileList[count]=NULL;	
}

//リスト内のファイルを出力アーカイブファイルに追加する
//パス情報はすべて取り除く
//ファイルが重複していた場合スキップする
//追加したファイル数を返す
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
		//パスで入力されていた場合，パス情報を消去	
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
			insert(input_text_file,"Adding");	
		}
	}
	return i;
}

//MS-DOS date formatに準拠して，最終更新日時を4バイトで保存
uint32_t TimeStamp(void){
	time_t now=time(NULL);
	struct tm *pnow=localtime(&now);
	int year=(pnow->tm_year+1900)-2000;
	int month=pnow->tm_mon+1;
	int day=pnow->tm_mday;
	int hour=pnow->tm_hour;
	int min=pnow->tm_min;
	int sec=pnow->tm_sec;

	uint32_t last_mod_time=0;
	last_mod_time=year;
	last_mod_time<<=TIMESTAMP_MON_BIT;
	last_mod_time|=month;
	last_mod_time<<=TIMESTAMP_DAY_BIT;
	last_mod_time|=day;
	last_mod_time<<=TIMESTAMP_HOUR_BIT;
	last_mod_time|=hour;
	last_mod_time<<=TIMESTAMP_MIN_BIT;
	last_mod_time|=min;
	last_mod_time<<=TIMESTAMP_SEC_BIT;
	last_mod_time|=sec;
	
	return last_mod_time;
}

//MS-DOS date formatから文字列への変換
//形式はyyyy-mm-dd hh:mm:ss
char *TransformMSDOSdate2str(uint32_t last_mod_time){
	char *date=(char*)malloc(sizeof(char)*DATE_LENGTH);
	int sec=last_mod_time & ((1<<TIMESTAMP_SEC_BIT)-1);	
	last_mod_time>>=TIMESTAMP_SEC_BIT;
	int min=last_mod_time & ((1<<TIMESTAMP_MIN_BIT)-1);
	last_mod_time>>=TIMESTAMP_MIN_BIT;
	int hour=last_mod_time & ((1<<TIMESTAMP_HOUR_BIT)-1);
	last_mod_time>>=TIMESTAMP_HOUR_BIT;
	int day=last_mod_time & ((1<<TIMESTAMP_DAY_BIT)-1);
	last_mod_time>>=TIMESTAMP_DAY_BIT;
	int month=last_mod_time & ((1<<TIMESTAMP_MON_BIT)-1);
	last_mod_time>>=TIMESTAMP_MON_BIT;
	int year=last_mod_time;
	year+=2000;
	sprintf(date,"%4d-%02d-%02d %02d:%02d:%02d",year,month,day,hour,min,sec);
	return date;
}
//入力ファイルに対して圧縮処理を施したデータを出力アーカイブに書き込む
void insert(FILE *input_text_file,char *operation){
	fprintf(stderr,"%s %s\n",operation,Header.file_name);
	
	int method_offset=2;
	int selected_method=DEFAULT_METHOD-method_offset;	
	Header.compression_method=DEFAULT_METHOD;

	//圧縮後が元のサイズよりも大きくなった場合，書き込みをやり直す
	//やり直す際に戻ってこれるようにカーソルを保存
	long saved_pos_header=ftell(OutputCarFile);
	WriteFileHeader();
	long saved_pos_file=ftell(OutputCarFile);
	fseek(input_text_file,0,SEEK_END);
	Header.original_size=ftell(input_text_file);
	fseek(input_text_file,0,SEEK_SET);
	
	Header.last_mod_time=TimeStamp();

	int (*compress[])()={LZSSCompress,lz4CompressRequest};

	//圧縮後にサイズが拡大した場合は，元のデータを書き込む
	if(!(*compress[selected_method])(input_text_file)){
		Header.compression_method=STORED;
		fseek(OutputCarFile,saved_pos_file,SEEK_SET);
		fseek(input_text_file,0,SEEK_SET);	
		store(input_text_file);	
	}
	fclose(input_text_file);
	
	//サイズやCRCはここで確定するのでヘッダ情報として書き込む
	fseek(OutputCarFile,saved_pos_header,SEEK_SET);
	WriteFileHeader();
	fseek(OutputCarFile,0,SEEK_END);

}

//入力ファイルを圧縮せずにそのまま出力アーカイブに書き込む
//bufferに読み込んでから，読み込んだ分だけ書き込む
int store(FILE *input_text_file){
	char buffer[BUFFER_SIZE];
	Header.original_crc=CRC_MASK;
	int n;
	while((n=fread(buffer,1,BUFFER_SIZE,input_text_file))!=0){
		fwrite(buffer,1,n,OutputCarFile);
		Header.original_crc=CalculateCRC32(n,Header.original_crc,buffer);
	}
	Header.compressed_size=Header.original_size;
	Header.original_crc^=CRC_MASK;
	return 1;
}

//入力ファイルを展開せずにそのまま出力ファイルに書き込む
uint32_t unstore(FILE *destination){
	uint32_t crc=CRC_MASK;
	unsigned char buffer[BUFFER_SIZE];
	unsigned int count;	
	while(Header.original_size!=0){
		count=Header.original_size>BUFFER_SIZE?BUFFER_SIZE:(int)Header.original_size;	
		if(fread(buffer,1,count,InputCarFile)!=count){
			fprintf(stderr,"CARファイルの読み込みに失敗しました\n");
			exit(1);	
		}
		if(fwrite(buffer,1,count,destination)!=count){
			fprintf(stderr,"書き込みに失敗しました\n");
			exit(1);	
		}
		crc=CalculateCRC32(count,crc,buffer);
		Header.original_size-=count;	
	}
	return crc^CRC_MASK;
}

//ファイル名の長さが0のヘッダを終端としている
void WriteEndOfCarHeader(void){
	fputc(0,OutputCarFile);
}

//現在処理中のファイルのヘッダ情報を出力アーカイブに書き込む
void WriteFileHeader(void){
	int i=0;	
	for(;;){
		putc(Header.file_name[i],OutputCarFile);
		if(Header.file_name[i++]=='\0'){
			break;	
		}
	}
	unsigned char header_data[HEADER_BLOCK_SIZE];
	Header.header_crc=CalculateCRC32(i,CRC_MASK,Header.file_name);
	int header_offset=0;
	int num_byte=1;
	pack(num_byte,(uint32_t)Header.compression_method,header_data+header_offset);
	header_offset+=num_byte;
	num_byte=4;
	pack(num_byte,Header.original_size,header_data+header_offset);
	header_offset+=num_byte;
	pack(num_byte,Header.compressed_size,header_data+header_offset);
	header_offset+=num_byte;
	pack(num_byte,Header.last_mod_time,header_data+header_offset);
	header_offset+=num_byte;
	pack(num_byte,Header.original_crc,header_data+header_offset);
	header_offset+=num_byte;
	Header.header_crc=CalculateCRC32(17,Header.header_crc,header_data);
	Header.header_crc^=CRC_MASK;
	pack(num_byte,Header.header_crc,header_data+header_offset);
	fwrite(header_data,1,HEADER_BLOCK_SIZE,OutputCarFile);	
}

//書き込むデータをバッファに格納しておき，バッファがいっぱいになったらファイルに書き込む
void pack(int num_bytes,uint32_t number, unsigned char *buffer){
	while(num_bytes-->0){
		//8ビットずつバッファに出力
		//書き込むデータnumberの下位8ビットをバッファに出力	
		*buffer++=(unsigned char)(number & 0xff);	
		number>>=8;
	}
}

//バッファのデータをint型に変換する
unsigned int unpack(int num_bytes,unsigned char *buffer){
	uint32_t result=0;
	int shift_count=0;
	while(num_bytes-->0){
		result|=(uint32_t)*buffer++ << shift_count;
		shift_count+=8;	
	}
	return result;
}

//アーカイブのヘッダを順番に読み込み，指定された処理をする
int ProcessAllFiles(int command,int count){
	FILE *destination;
	if(command=='p'){
		destination=stdout;	
	}else{
		destination=NULL;	
	}
	int matched;	
	FILE *input_text_file;	
	while(InputCarFile!=NULL && ReadFileHeader()!=END_OF_CAR_HEADER){
		matched=SearchFileList(Header.file_name);
		switch(command){

			//ファイル名がマッチしていれば，そのファイルをアーカイブから削除する
			//出力アーカイブに含めないようにスキップする
			case 'd':
				if(matched){
					SkipOverFile();	
					count++;	
				}else{
					CopyFile();	
				}
				break;

			//ファイル名がマッチしていれば，すでに追加されている
			//出力アーカイブに含めないようにスキップする
			case 'a':
				if(matched){
					SkipOverFile();				
				}else{
					CopyFile();	
				}
				break;
	
			//ファイル名がマッチしていれば，各出力先に書き出す
			case 'p':
			case 'x':
				if(matched){
					extract(destination);
					count++;	
				}else{
					SkipOverFile();	
				}
				break;
			//ファイル名がマッチしていれば，アーカイブにあるそのファイルをカレントディレクトリの同名ファイルで置換する	
			case 'r':
				if(matched){
					input_text_file=fopen(Header.file_name,"rb");	
					if(input_text_file==NULL){
						fprintf(stderr,"%sが見つかりませんでした\n",Header.file_name);
						fprintf(stderr,"スキップします\n");
						CopyFile();
					}else{
						SkipOverFile();
						insert(input_text_file,"Replacing");
						count++;
						fclose(input_text_file);	
					}
				}else{
					CopyFile();	
				}
				break;

			//アーカイブを一覧表示する
			case 'l':
				if(matched){
					ListCarFile();
					count++;	
				}
				SkipOverFile();
				break;	
		}
	}
	return count;
}

//現在処理中のファイルを書き込まずにスキップする
//圧縮後のサイズだけカーソルを進める
//この関数を呼出したときには，ファイルポインタは現在のヘッダに対応するデータの先頭を指していると仮定している
void SkipOverFile(void){
	fseek(InputCarFile,Header.compressed_size,SEEK_CUR);
}

//入力CARファイルから出力CARファイルにコピーする
//先にReadFileHeader()で読み込んでいたヘッダ情報を書き込む
//データはbufferに読み込んでから出力CARファイルに書き込む
void CopyFile(void){
	char buffer[BUFFER_SIZE];
	unsigned int count=0;
	WriteFileHeader();	
	while(Header.compressed_size!=0){
		count=Header.compressed_size<BUFFER_SIZE?(int)Header.compressed_size:BUFFER_SIZE;
		if(fread(buffer,1,count,InputCarFile)!=count){
			fprintf(stderr,"%sの読み込みに失敗しました\n",Header.file_name);	
			exit(1);
		}
		Header.compressed_size-=count;
		if(fwrite(buffer,1,count,OutputCarFile)!=count){
			fprintf(stderr,"CARファイルの書き込みに失敗しました\n");
			exit(1);	
		}
	}
}

//現在読み込んでいるヘッダ情報に一致するファイルを引数のファイルリストから調べる
int SearchFileList(char *file_name){
	for(int i=0;FileList[i]!=NULL;i++){
		if(WildCardMatch(file_name,FileList[i])){
			return 1;	
		}
	}
	return 0;
}

//ワイルドカードを含むwild_stringと比較する
//対象とするワイルドカード
//  *：長さ0以上の任意の文字列
//  ?：任意の1文字
int WildCardMatch(char *string,char *wild_string){
	for(;;){
		//*の次の文字から比較する	
		if(*wild_string=='*'){
			wild_string++;	
	
			for(;;){
				//アスタリスク*の次の文字と一致するまでポインタを進める	
				while(*string!='\0' && *string!=*wild_string){
					string++;	
				}
				//アスタリスク*の次の文字から比較	
				if(WildCardMatch(string,wild_string)){
					return 1;	
				}else if(*string=='\0'){
					return 0;	
				}else{
					//wild_stringのみが終端の場合はstringだけ進める	
					string++;	
				}
			}
		
		//?の次の文字を見る	
		}else if(*wild_string=='?'){
			wild_string++;
			//空文字は任意の一文字には含まれない	
			if(*string++=='\0'){
				return 0;	
			}
		
		//ワイルドカード無しの単純な比較
		}else{
			if(*string!=*wild_string){
				return 0;	
			}else if(*string=='\0'){
				return 1;	
			}
			string++;
			wild_string++;
		}
			
	}

}

//ヘッダの読み込み
//ファイル名の長さが0の場合はCARの末尾
int ReadFileHeader(void){
	int i=0;
	for(;;){
		int c=getc(InputCarFile);
		Header.file_name[i]=(char)c;
		if(c=='\0'){
			break;	
		}
		if(++i==FILENAME_MAX){
			fprintf(stderr,"ファイル名が長すぎます\n");
			exit(1);
		}
	}
	//ファイル名の長さが0の場合はCARの末尾
	if(i==0){
		return END_OF_CAR_HEADER;	
	}
	uint32_t header_crc=CalculateCRC32(i+1,CRC_MASK,Header.file_name);
	unsigned char header_data[HEADER_BLOCK_SIZE];	
	fread(header_data,1,HEADER_BLOCK_SIZE,InputCarFile);
	int num_byte=1;
	int header_offset=0;	
	Header.compression_method=(char)unpack(num_byte,header_data+header_offset);
	header_offset+=num_byte;
	num_byte=4;
	Header.original_size=unpack(num_byte,header_data+header_offset);
	header_offset+=num_byte;
	Header.compressed_size=unpack(num_byte,header_data+header_offset);
	header_offset+=num_byte;
	Header.last_mod_time=unpack(num_byte,header_data+header_offset);
	header_offset+=num_byte;
	Header.original_crc=unpack(num_byte,header_data+header_offset);
	header_offset+=num_byte;
	Header.header_crc=unpack(num_byte,header_data+header_offset);
	header_crc=CalculateCRC32(17,header_crc,header_data);
	header_crc^=CRC_MASK;
	if(Header.header_crc!=header_crc){
		fprintf(stderr,"ヘッダー%s：チェックサムエラー",Header.file_name);	
		exit(1);	
	}
	return 1;
}

//入力CARファイルからファイルを取り出し，destinationへ書き出す
void extract(FILE *destination){
	fprintf(stderr,"%s\n\n",Header.file_name);
	FILE *output_text_file;

	//出力先にNULLが指定されていれば，ヘッダ中の指定されたファイルを展開し取り出す	
	if(destination==NULL){
		if((output_text_file=fopen(Header.file_name,"wb"))==NULL){
			fprintf(stderr,"%sを開けませんでした\n",Header.file_name);
			SkipOverFile();
			return ;	
		}

	}else{
		output_text_file=destination;	
	}
	uint32_t crc;	
	int error=0;
	switch(Header.compression_method){
		
		//圧縮していない生のデータの場合はそのまま取り出す	
		case STORED:
			crc=unstore(output_text_file);
			break;	
	
		//圧縮データの場合は展開して取り出す	
		case LZSS:
			crc=LZSSExpand(output_text_file);
			break;	
	
		case LZ4:
			crc=lz4ExpandRequest(output_text_file);
			break;	
		default:
			fprintf(stderr,"不明なメソッド: %c\n",Header.compression_method);
			SkipOverFile();
			error=1;
			crc=Header.original_crc;
			break;
	}
	
	fprintf(stderr,"\n\n");

	if(crc!=Header.original_crc){
		fprintf(stderr,"CRCエラー\n");
		error=1;
	}
	if(destination==NULL){
		fclose(output_text_file);
		if(error){
			remove(Header.file_name);	
		}
	}


	if(!error){
		fprintf(stderr,"OK\n");	
	}
}

void PrintTitle(void){
	printf("\n");
	printf(
	"     名  前             最終更新          元のサイズ      圧縮後のサイズ   ratio    CRC      method \n");
	printf(
	"----------------  -------------------  ----------------  ----------------  -----  --------  --------\n");

}

void ListCarFile(void){
	static char *methods[]={"stored","LZSS","LZ4"};
	
	printf("%-16s  %19s  %10u bytes  %10u bytes  %4d%%  %08x  %s\n",Header.file_name,TransformMSDOSdate2str(Header.last_mod_time),Header.original_size,Header.compressed_size,CompressionRatio(Header.compressed_size,Header.original_size),Header.original_crc,methods[(int)Header.compression_method-1]);
}

int CompressionRatio(ull compressed,ull original){
	if(original==0){
		return 0;	
	}
	return (int)((1-(double)compressed/original)*100);
}

//LZ4圧縮処理APIを呼び出す
//圧縮後のサイズを返す
//圧縮後のサイズが元のサイズよりも大きくなった場合は0を返す
int lz4CompressRequest(FILE *input_text_file){
	char *src=(char*)malloc(sizeof(char)*Header.original_size);
	if(fread(src,1,Header.original_size,input_text_file)!=Header.original_size){
		fprintf(stderr,"入力ファイルの読み込みに失敗しました\n");
		exit(1);	
	}
	Header.original_crc=CRC_MASK;
	Header.original_crc=CalculateCRC32(Header.original_size,Header.original_crc,src);
	Header.original_crc^=CRC_MASK;
	int capacity=Header.original_size;
	char *dst=(char*)malloc(sizeof(char)*capacity);
	int compressed_size=LZ4_compress_default(src,dst,Header.original_size,capacity);	
	if(fwrite(dst,1,compressed_size,OutputCarFile)!=compressed_size){
		fprintf(stderr,"圧縮データの書き込みに失敗しました\n");
		exit(1);	
	}
	free(src);
	free(dst);
	return Header.compressed_size=compressed_size;	
}

//LZ4展開処理APIを呼び出す
//CRCチェックサムを返す
uint32_t lz4ExpandRequest(FILE *output){
	char *src=(char*)malloc(sizeof(char)*Header.compressed_size);
	if(fread(src,1,Header.compressed_size,InputCarFile)!=Header.compressed_size){
		fprintf(stderr,"入力ファイルの読み込みに失敗しました\n");
		exit(1);	
	}
	
	char *dst=(char*)malloc(sizeof(char)*Header.original_size);
	LZ4_decompress_safe(src,dst,Header.compressed_size,Header.original_size);
	if(fwrite(dst,1,Header.original_size,output)!=Header.original_size){
		fprintf(stderr,"展開データの書き込みに失敗しました\n");
		exit(1);	
	}

	uint32_t crc=CRC_MASK;
	crc=CalculateCRC32(Header.original_size,crc,dst);
	free(src);
	free(dst);

	return crc^CRC_MASK;
}
