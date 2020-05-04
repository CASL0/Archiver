#ifndef CAR_H
#define CAR_H

#define BASE_HEADER_SIZE 19
#define CRC_MASK 0xffffffffL
#define CRC32_POLYNOMIAL 0xedb88320L

#ifndef FILENAME_MAX
#define FILENAME_MAX 128
#endif //FILENAME_MAX

struct Header{
	char file_name[FILENAME_MAX];
	char compression_method;
	unsigned int original_size;
	unsigned int compressed_size;
	unsigned int original_crc;
	unsigned int header_crc;	
};

typedef struct Header Header;

extern void usage(void);


#endif //CAR_H
