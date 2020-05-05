#ifndef CAR_H
#define CAR_H

#include <stdio.h>
#include <stdint.h>

#define BASE_HEADER_SIZE 19
#define CRC_MASK 0xffffffff
#define CRC32_POLYNOMIAL 0xedb88320
#define FILE_LIST_MAX 100
#define HEADER_BLOCK_SIZE 17

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
extern int ParseArguments(int argc,char *argv[]);
extern void OpenArchiveFiles(char *name, int command);
extern void BuildFileList(int argc, char *argv[], int command);
extern int AddFileList(void); 
extern void insert(FILE *input_text_file,char *operation);
extern void WriteFileHeader(void);
//ファイルに出力する前にbufferに書き込むデータを格納する
extern void pack(int num_bytes,uint32_t number, unsigned char *buffer);
extern uint32_t unpack(int num_bytes,unsigned char *buffer);
#endif //CAR_H
