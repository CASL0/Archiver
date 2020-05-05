#include "car.h"
#include <stdio.h>

int main(int argc,char *argv[]){

	setbuf(stdout,NULL);
	setbuf(stderr,NULL);
	fprintf(stderr,"CAR 1.0：");	
	BuildCRCTable();
	int command=ParseArguments(argc,argv);	
	fprintf(stderr,"\n");
	OpenArchiveFiles(argv[2],command);
	BuildFileList(argc-3,argv+3,command);	
	return 0;
}
