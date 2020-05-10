#include "car.h"
#include "lzss.h"
#include <stdio.h>
#include <stdlib.h>

static char data_buffer[DATA_BUFFER_SIZE];
static int flag_bit_mask;
static unsigned int buffer_offset;

static unsigned char window[WINDOW_SIZE]; 
static Tree tree[WINDOW_SIZE+1];

void InitOutputBuffer(void){
	data_buffer[0]=0;
	flag_bit_mask=1;
	buffer_offset=1;
}

//バッファを出力アーカイブに書き込む
//元のデータよりも圧縮後のほうが大きくなった場合は，圧縮を中止するため0を返す
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
	return 1;
}

//単一の文字を出力バッファに加える
//フラグビットをシフトし，8ビット全て使われた場合バッファを書き出す
//元のデータよりも圧縮後の方が大きくなった場合は0を返す
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

//インデックスと長さの組をバッファに出力する
//フラグビットをシフトしていき8ビット全て使われた場合バッファを書き出す
int OutputPair(int position,int length){
	//1バイトの先頭4ビットはlength，残りの4ビットはpositionの語頭4ビットを格納	
	data_buffer[buffer_offset]=(char)(length<<LENGTH_BIT);
	data_buffer[buffer_offset++]|=(position>>(INDEX_BIT-LENGTH_BIT));
	data_buffer[buffer_offset++]=(char)(position & 0xff);
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


//LZSSにより入力ファイルを圧縮する
//圧縮が成功した場合は1を返す
//元のデータよりも圧縮後の方が大きくなった場合は0を返す
int LZSSCompress(FILE *input_text_file){
	Header.compressed_size=0;
	Header.original_crc=CRC_MASK;
	InitOutputBuffer();	

	int current_position=1;
	int c;
	int i=0;
	//窓に先読みしておく	
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
			
			//圧縮後の方が元より大きくなった場合は0を返す	
			if(!OutputChar(window[current_position])){
				return 0;	
			}
		}else{
			replace_count=match_length;
			
			//圧縮後の方が元より大きくなった場合は0を返す	
			if(!OutputPair(match_position,match_length-(BREAK_EVEN+1))){
				return 0;	
			}
		}
		
		//窓をスライドさせる
		//窓の終端の文字列を二分探索木から削除する
		for(i=0;i<replace_count;i++){
			DeleteString(MOD_WINDOW(current_position+LOOK_AHEAD_SIZE));
			if((c=getc(input_text_file))==EOF){
				look_ahead_size--;
			}else{
				Header.original_crc=UpdateCharacterCRC32(Header.original_crc,c);	
				window[MOD_WINDOW(current_position+LOOK_AHEAD_SIZE)]=(unsigned char)c;
			}
			current_position=MOD_WINDOW(current_position+1);
			
			//最長一致文字列の長さとインデックスを受け取る	
			if(look_ahead_size){
				match_length=AddString(current_position,&match_position);
			}
		}
	}
	Header.original_crc^=CRC_MASK;
	return FlushOutputBuffer();
}

//二分探索木に文字列を追加する
//現在の文字列と最長一致する文字列を探して，その長さを返す
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

//二分探索木からノードを削除する
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

//LZSSによりファイルを展開する
//CRCチェックサムの値を返す
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
				crc=UpdateCharacterCRC32(crc,c);
				window[current_position]=(unsigned char)c;
				current_position=MOD_WINDOW(current_position+1);
			}
		}

	}
	return crc^CRC_MASK;
}

