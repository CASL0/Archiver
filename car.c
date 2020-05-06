#include "car.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

//32ビットCRC値を計算するためのCRCテーブル
static uint32_t C32Table[256];
//コマンドラインで指定されたCARファイルの名前
char CarFileName[FILENAME_MAX];
//入力CARファイル
FILE *InputCarFile;
//出力用CARファイルはまず一時的な名前でオープンされる
char TmpFileName[FILENAME_MAX];
//出力用CARファイル
FILE *OutputCarFile;
//アーカイブするファイルのリスト
static char *FileList[FILE_LIST_MAX];
//現在処理中のファイルのヘッダ
static HEADER Header;

 void usage(void){
	fprintf(stderr,"CAR -- Compressed ARchiver\n\n"); 
	fprintf(stderr,"usage: car command car-file [file ...]\n\n");
	fprintf(stderr,"commands: \n");
	fprintf(stderr,"  a：ファイルをアーカイブに追加する\n");
	fprintf(stderr,"  x：ファイルをアーカイブから取り出す\n");
	fprintf(stderr,"  r：アーカイブのファイルを置換する\n");
	fprintf(stderr,"  d：アーカイブのファイルを削除する\n");
	fprintf(stderr,"  p：アーカイブのファイルを表示する\n");
	fprintf(stderr,"\n");
	exit(1);
}

void BuildCRCTable(void){
	for(int i=0;i<256;i++){
		uint32_t value=i;
		for(int j=8;j>0;j--){
			value=(value & 1)?(value >> 1)^CRC32_POLYNOMIAL:value>>1;
		}
		C32Table[i]=value;
	}
}

uint32_t CalculateCRC32(uint32_t count, uint32_t crc, void *buffer){
	unsigned char *p=(unsigned char*)buffer;
	uint32_t tmp1,tmp2;
	while(count--!=0){
		tmp1=(crc>>8) & 0x00ffffff;	
		tmp2=C32Table[((int)crc^*p++) & 0xff];
		crc=tmp1^tmp2;
	}
	return crc;
}

uint32_t UpdateCharacterCRC32(uint32_t crc, int c){
	uint32_t tmp1=(crc>>8) & 0x00ffffff;
	uint32_t tmp2=C32Table[((int)crc^c)&0xff];
	crc=tmp1^tmp2;
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

void BuildFileList(int argc, char *argv[], int command){
	int count=0;
	if(argc==0){
		FileList[count++]="*";	
	}else{
		for(int i=0;i<argc;i++){
			FileList[count]=malloc(strlen(argv[i])+2);	
			if(FileList[count]==NULL){
				fprintf(stderr,"メモリの割当に失敗しました\n");
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
			insert(input_text_file,"Adding");	
		}
	}
	return i;
}

void insert(FILE *input_text_file,char *operation){
	fprintf(stderr,"%s %s\n",operation,Header.file_name);
	//圧縮後が元のサイズよりも大きくなった場合に戻ってこれるようにカーソルを保存	
	long saved_pos_header=ftell(OutputCarFile);
	Header.compression_method=2;	
	WriteFileHeader();
	long saved_pos_file=ftell(OutputCarFile);
	fseek(input_text_file,0,SEEK_END);
	Header.original_size=ftell(input_text_file);
	fseek(input_text_file,0,SEEK_SET);
	if(!LZSSCompress(input_text_file)){
		Header.compression_method=1;
		fseek(OutputCarFile,saved_pos_file,SEEK_SET);
		rewind(input_text_file);
		store(input_text_file);	
	}
	fclose(input_text_file);
	fseek(OutputCarFile,saved_pos_header,SEEK_SET);
	WriteFileHeader();
	fseek(OutputCarFile,0,SEEK_END);

}

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

uint32_t unstore(FILE *destination){
	uint32_t crc=CRC_MASK;
	unsigned char buffer[BUFFER_SIZE];
	int count;	
	while(Header.original_size!=0){
		if(Header.original_size>BUFFER_SIZE){
			count=BUFFER_SIZE;	
		}else{
			count=(int)Header.original_size;	
		}
		
		if(fread(buffer,1,count,InputCarFile)!=count){
			fprintf(stderr,"CARファイルの読み込みに失敗しました\n");
			exit(1);	
		}
		if(fwrite(buffer,1,count,destination)!=count){
			fprintf(stderr,"書き込みに失敗しました\n");
			return ~Header.original_crc;	
		}
		crc=CalculateCRC32(count,crc,buffer);
		Header.original_size-=count;	
	}
	return crc^CRC_MASK;
}

void WriteEndOfCarHeader(void){
	fputc(0,OutputCarFile);
}


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
	int num_byte=1;
	pack(num_byte,(uint32_t)Header.compression_method,header_data+0);
	num_byte=4;
	pack(num_byte,Header.original_size,header_data+1);
	pack(num_byte,Header.compressed_size,header_data+5);
	pack(num_byte,Header.original_crc,header_data+9);
	Header.header_crc=CalculateCRC32(13,Header.header_crc,header_data);
	Header.header_crc^=CRC_MASK;
	pack(num_byte,Header.header_crc,header_data+13);
	fwrite(header_data,1,HEADER_BLOCK_SIZE,OutputCarFile);	
}

void pack(int num_bytes,uint32_t number, unsigned char *buffer){
	while(num_bytes-->0){
		//8ビットずつバッファに出力
		//書き込むデータnumberの下位8ビットをバッファに出力	
		*buffer++=(unsigned char)(number & 0xff);	
		number>>=8;
	}
}

uint32_t unpack(int num_bytes,unsigned char *buffer){
	uint32_t result=0;
	int shift_count=0;
	while(num_bytes-->0){
		result|=(uint32_t)*buffer++ << shift_count;
		shift_count+=8;	
	}
	return result;
}

int ProcessAllFiles(int command,int count){
	FILE *destination;
	if(command=='p'){
		destination=stdout;	
	}else{
		destination=NULL;	
	}
	int matched;	
	FILE *input_text_file;	
	while(InputCarFile!=NULL && ReadFileHeader()!=0){
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
		}
	}
	return count;
}

void SkipOverFile(void){
	fseek(InputCarFile,Header.compressed_size,SEEK_CUR);
}

void CopyFile(void){
	char buffer[BUFFER_SIZE];
	unsigned int count=0;
	WriteFileHeader();	
	while(Header.compressed_size!=0){
		count=Header.compressed_size<BUFFER_SIZE?(unsigned int)Header.compressed_size:BUFFER_SIZE;
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
					//wild_stringが終端の場合はstringだけ進める	
					string++;	
				}
			}
		}else if(*wild_string=='?'){
			//次の文字を指す	
			wild_string++;
			//空文字は任意の一文字には含まれない	
			if(*string++=='\0'){
				return 0;	
			}
		}else{
			if(*string!=*wild_string){
				return 0;	
			}
			if(*string=='\0'){
				return 1;	
			}
			string++;
			wild_string++;
		}
			
	}

}

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
		return 0;	
	}
	uint32_t header_crc=CalculateCRC32(i+1,CRC_MASK,Header.file_name);
	unsigned char header_data[BUFFER_SIZE];	
	fread(header_data,1,BUFFER_SIZE,InputCarFile);
	int num_byte=1;	
	Header.compression_method=(char)unpack(num_byte,header_data+0);
	num_byte=4;
	Header.original_size=unpack(num_byte,header_data+1);
	Header.compressed_size=unpack(num_byte,header_data+5);
	Header.original_crc=unpack(num_byte,header_data+9);
	Header.header_crc=unpack(num_byte,header_data+13);
	header_crc=CalculateCRC32(13,header_crc,header_data);
	header_crc^=CRC_MASK;
	if(Header.header_crc!=header_crc){
		fprintf(stderr,"ヘッダー%s：チェックサムエラー",Header.file_name);	
		exit(1);	
	}
	return 1;
}

void extract(FILE *destination){
}


//-- 以下，圧縮処理関係 -- 
static char data_buffer[DATA_BUFFER_SIZE];
static int flag_bit_mask;
static int buffer_offset;

static unsigned char window[WINDOW_SIZE]; 
static Tree tree[WINDOW_SIZE+1];

void InitOutputBuffer(void){
	data_buffer[0]=0;
	flag_bit_mask=1;
	buffer_offset=1;
}

int FlushOutputBuffer(void){
	if(buffer_offset==1){
		return 1;	
	}
	Header.compressed_size+=buffer_offset;
	if((Header.compressed_size)>=Header.original_size){
		return 0;	
	}
	if(fwrite(data_buffer,1,buffer_offset,OutputCarFile)!=buffer_offset){
		fprintf(stderr,"圧縮データの書き込みに失敗しました\n");
		exit(1);	
	}
	InitOutputBuffer();
}

int OutputChar(int data){
	data_buffer[buffer_offset++]=(char)data;
	data_buffer[0]|=flag_bit_mask;
	flag_bit_mask<<=1;
	if(flag_bit_mask==0x100){
		return FlushOutputBuffer();	
	}else{
		return 1;	
	}
}

int OutputPair(int position,int length){
	//1バイトの先頭4ビットはlength，残りの4ビットをpositionの語頭4ビットを格納	
	data_buffer[buffer_offset]=(char)(length<<LENGTH_BIT);
	data_buffer[buffer_offset++]|=(position>>(INDEX_BIT-LENGTH_BIT));
	data_buffer[buffer_offset++]|=(char)(position & 0xff);
	flag_bit_mask<<=1;
	if(flag_bit_mask==0x100){
		return FlushOutputBuffer();	
	}else{
		return 1;	
	}
}

void InitInputBuffer(void){
	flag_bit_mask=1;
	data_buffer[0]=(char)getc(InputCarFile);
}

int InputBit(void){
	if(flag_bit_mask==0x100){
		InitInputBuffer();	
	}
	flag_bit_mask<<=1;
	return data_buffer[0]&(flag_bit_mask>>1);
}



int LZSSCompress(FILE *input_text_file){
	Header.compressed_size=0;
	Header.original_crc=CRC_MASK;
	InitOutputBuffer();	

	int current_position=1;
	int c;
	int i=0;
	for(;i<LOOK_AHEAD_SIZE;i++){
		if((c=getc(input_text_file))==EOF){
			break;
		}
		window[current_position+i]=(unsigned char)c;
		Header.original_crc=UpdateCharacterCRC32(Header.original_crc,c);	
	}
	int look_ahead_size=i;
	InitTree(current_position);
	int match_length=0;
	int match_position=0;
	while(look_ahead_size>0){
		if(match_length>look_ahead_size){
			match_length=look_ahead_size;
		}
		int replace_count;
		if(match_length<=BREAK_EVEN){
			replace_count=1;
			if(!OutputChar(window[current_position])){
				return 0;	
			}
		}else{
			replace_count=match_length;
			if(!OutputPair(match_position,match_length-(BREAK_EVEN+1))){
				return 0;	
			}
		}
		for(i=0;i<replace_count;i++){
			DeleteString(MOD_WINDOW(current_position+LOOK_AHEAD_SIZE));
			if((c=getc(input_text_file))==EOF){
				look_ahead_size--;
			}else{
				window[MOD_WINDOW(current_position+LOOK_AHEAD_SIZE)]=(unsigned char)c;
			}
			current_position=MOD_WINDOW(current_position+1);
			if(look_ahead_size){
				match_length=AddString(current_position,&match_position);
			}
		}
	}
	Header.original_crc^=CRC_MASK;
	return FlushOutputBuffer();
}

int AddString(int new_node,int *match_position){
	if(new_node==END_OF_STREAM){
		return 0;
	}
	int current_node=tree[TREE_ROOT].larger_child;
	int match_length=0;
	for(;;){
		int i=0;
		int diff;
		for(;i<LOOK_AHEAD_SIZE;i++){
			diff=window[MOD_WINDOW(new_node+i)]-window[MOD_WINDOW(current_node+i)];
			if(diff!=0){
				break;
			}
		}
		if(i>=match_length){
			match_length=i;
			*match_position=current_node;
		
			if(match_length>=LOOK_AHEAD_SIZE){
				ReplaceNode(current_node,new_node);
				return match_length;
			}
		}
		int *child=(diff>=0)?&tree[current_node].larger_child:&tree[current_node].smaller_child;
		if(*child==UNUSED){
			*child=new_node;
			tree[new_node].parent=current_node;
			tree[new_node].smaller_child=tree[new_node].larger_child=UNUSED;
			return match_length;
		}
		current_node=*child;
	}
}

void InitTree(int root){
	tree[TREE_ROOT].larger_child=root;
	tree[root].parent=TREE_ROOT;
	tree[root].smaller_child=UNUSED;
	tree[root].larger_child=UNUSED;
}

void DeleteString(int node){
	if(tree[node].parent==UNUSED){
		return ;
	}
	if(tree[node].smaller_child==UNUSED){
		RaiseNode(node,tree[node].larger_child);
	}else if(tree[node].larger_child==UNUSED){
		RaiseNode(node,tree[node].smaller_child);
	}else{
		int replace=LeftMost(node);
		DeleteString(replace);
		ReplaceNode(node,replace);
	}
}

void RaiseNode(int old_node,int new_node){
	tree[new_node].parent=tree[old_node].parent;
	if(tree[tree[old_node].parent].smaller_child==old_node){
		tree[tree[old_node].parent].smaller_child=new_node;
	}else{
		tree[tree[old_node].parent].larger_child=new_node;
	}
	tree[old_node].parent=UNUSED;
}

void ReplaceNode(int old_node,int new_node){
	int parent=tree[old_node].parent;
	if(tree[parent].smaller_child==old_node){
		tree[parent].smaller_child=new_node;
	}else{
		tree[parent].larger_child=new_node;
	}
	tree[new_node]=tree[old_node];
	tree[tree[new_node].smaller_child].parent=tree[tree[new_node].larger_child].parent=new_node;
	tree[old_node].parent=UNUSED;
}

int LeftMost(int node){
	node=tree[node].smaller_child;
	while(tree[node].larger_child!=UNUSED){
		node=tree[node].larger_child;
	}
	return node;
}

uint32_t LZSSExpand(FILE *output){
	uint32_t crc=CRC_MASK;
	ull output_count=0;
	InitInputBuffer();
	int current_position=1;
	while(output_count<Header.original_size){
		int c;
		//フラグビットを確認して，1なら文字を読み込み，0ならインデックスと長さの組を読み込む
		if(InputBit()){
			c=getc(InputCarFile);
			putc(c,output);
			output_count++;
			crc=UpdateCharacterCRC32(crc,c);
			window[current_position]=(unsigned char)c;
			current_position=MOD_WINDOW(current_position+1);
		}else{
			//最初の1バイトは，上位4ビットは長さで下位4ビットはインデックスの語頭4ビット
			int match_length=getc(InputCarFile);
			int match_position=getc(InputCarFile);
			match_position|=(match_length & 0xf)<<(INDEX_BIT-LENGTH_BIT);
			match_length>>=LENGTH_BIT;	
			match_length+=BREAK_EVEN;
			output_count+=match_length+1;	
			for(int i=0;i<=match_length;i++){
				c=window[MOD_WINDOW(match_position+i)];
				putc(c,output);
				window[current_position]=(unsigned char)c;
				current_position=MOD_WINDOW(current_position+1);
			}
		}

	}
	return crc^CRC_MASK;
}
