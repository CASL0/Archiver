#ifndef CAR_H
#define CAR_H

#include <stdio.h>
#include <stdint.h>

#define BASE_HEADER_SIZE 19
#define CRC_MASK 0xffffffff
#define CRC32_POLYNOMIAL 0xedb88320
#define FILE_LIST_MAX 100
#define HEADER_BLOCK_SIZE 17
#define BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 17 




#ifndef FILENAME_MAX
#define FILENAME_MAX 128
#endif //FILENAME_MAX

struct HEADER{
	char file_name[FILENAME_MAX];
	char compression_method;
	uint32_t original_size;
	uint32_t compressed_size;
	uint32_t original_crc;
	uint32_t header_crc;	
};

typedef struct HEADER HEADER;


extern void usage(void);
extern void BuildCRCTable(void);
extern uint32_t CalculateCRC32(uint32_t count, uint32_t crc, void *buffer);
extern uint32_t UpdateCharacterCRC32(uint32_t crc, int c);
extern int ParseArguments(int argc,char *argv[]);
extern void OpenArchiveFiles(char *name, int command);
extern void BuildFileList(int argc, char *argv[], int command);
extern int AddFileList(void); 
extern void insert(FILE *input_text_file,char *operation);
extern void WriteFileHeader(void);
//ファイルに出力する前にbufferに書き込むデータを格納する
extern void pack(int num_bytes,uint32_t number, unsigned char *buffer);
extern uint32_t unpack(int num_bytes,unsigned char *buffer);
extern int ProcessAllFiles(int command,int count);
extern void SkipOverFile(void);
extern void CopyFile(void);
extern int SearchFileList(char *file_name);
extern int WildCardMatch(char *string,char *wild_string);
extern int ReadFileHeader(void);
extern void extract(FILE *destination);
extern void WriteEndOfCarHeader(void);
extern int store(FILE *input_text_file);
extern uint32_t unstore(FILE *destination);


//-- 以下，圧縮処理関係 --
typedef unsigned long long ull;

#define INDEX_BIT 12
#define LENGTH_BIT 4
#define WINDOW_SIZE ((ull)1<<(INDEX_BIT))
#define RAW_LOOK_AHEAD_SIZE ((ull)1<<(LENGTH_BIT))
#define BREAK_EVEN ((1+(INDEX_BIT)+(LENGTH_BIT))/9)
#define LOOK_AHEAD_SIZE ((RAW_LOOK_AHEAD_SIZE)+(BREAK_EVEN))
#define TREE_ROOT WINDOW_SIZE
#define END_OF_STREAM 0
#define UNUSED 0
#define MOD_WINDOW(x) ((x)&((WINDOW_SIZE)-1))

struct Tree{
	int parent;
	int smaller_child;
	int larger_child;	
};

typedef struct Tree Tree;

extern void InitOutputBuffer(void);
extern int FlushOutputBuffer(void);
extern int OutputChar(int data);
extern int OutputPair(int position,int length);
extern void InitInputBuffer(void);
extern int InputBit(void);
extern int LZSSCompress(FILE *input_text_file);
extern int AddString(int new_node,int *match_position);
extern void InitTree(int root);
extern void DeleteString(int node);
extern void RaiseNode(int old_node,int new_node);
extern void ReplaceNode(int old_node,int new_node);
extern int LeftMost(int node);
extern uint32_t LZSSExpand(FILE *output);

#endif //CAR_H
