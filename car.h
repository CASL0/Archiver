#ifndef CAR_H
#define CAR_H

#include <stdio.h>
#include <stdint.h>

#define CRC_MASK 0xffffffff
#define CRC32_POLYNOMIAL 0xedb88320
#define FILE_LIST_MAX 100
#define HEADER_BLOCK_SIZE 21
#define BUFFER_SIZE 256
#define END_OF_CAR_HEADER 0
#define CRC_TABLE_SIZE 256
#define TIMESTAMP_MON_BIT 4
#define TIMESTAMP_DAY_BIT 5
#define TIMESTAMP_HOUR_BIT 5
#define TIMESTAMP_MIN_BIT 6
#define TIMESTAMP_SEC_BIT 6
#define DATE_LENGTH 20
#define DEFAULT_METHOD LZ4

#ifndef FILENAME_MAX
#define FILENAME_MAX 128
#endif //FILENAME_MAX

enum METHOD{
	STORED=1,
	LZSS=2,
	LZ4=3
};


struct HEADER{
	char file_name[FILENAME_MAX];
	char compression_method;
	uint32_t original_size;
	uint32_t compressed_size;
	uint32_t last_mod_time;
	uint32_t original_crc;
	uint32_t header_crc;	
};

typedef struct HEADER HEADER;

//32ビットCRC値を計算するためのCRCテーブル
uint32_t C32Table[CRC_TABLE_SIZE];
//コマンドラインで指定されたCARファイルの名前
char CarFileName[FILENAME_MAX];
//入力CARファイル
FILE *InputCarFile;
//出力用CARファイルはまず一時的な名前でオープンされる 
char TmpFileName[FILENAME_MAX];
//出力用CARファイル
FILE *OutputCarFile;
//アーカイブするファイルのリスト
char *FileList[FILE_LIST_MAX];
//現在処理中のファイルのヘッダ
HEADER Header;

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
extern void PrintTitle(void);
extern void ListCarFile(void);
extern int CompressionRatio(unsigned long long compressed,unsigned long long original);
extern uint32_t TimeStamp(void);
extern char *TransformMSDOSdate2str(uint32_t last_mod_time);

extern int lz4CompressRequest(FILE *input_text_file);
extern uint32_t lz4ExpandRequest(FILE *output);
#endif //CAR_H
