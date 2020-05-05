#ifndef CAR_H
#define CAR_H

#define BASE_HEADER_SIZE 19
#define CRC_MASK 0xffffffff
#define CRC32_POLYNOMIAL 0xedb88320
#define FILE_LIST_MAX 100


#ifndef FILENAME_MAX
#define FILENAME_MAX 128
#endif //FILENAME_MAX

struct HEADER{
	char file_name[FILENAME_MAX];
	char compression_method;
	unsigned int original_size;
	unsigned int compressed_size;
	unsigned int original_crc;
	unsigned int header_crc;	
};

typedef struct HEADER HEADER;

extern void usage(void);
extern void BuildCRCTable(void);
extern unsigned int CalculateCRC32(unsigned int count, unsigned int crc, void *buffer);
extern int ParseArguments(int argc,char *argv[]);
extern void OpenArchiveFiles(char *name, int command);
extern void BuildFileList(int argc, char *argv[], int command);


#endif //CAR_H
