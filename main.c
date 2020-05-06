#include "car.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char *argv[]){

	extern FILE *OutputCarFile;
	extern char *CarFileName, *TmpFileName;
	setbuf(stdout,NULL);
	setbuf(stderr,NULL);
	fprintf(stderr,"CAR 1.0：");	
	BuildCRCTable();
	int command=ParseArguments(argc,argv);	
	fprintf(stderr,"\n");
	OpenArchiveFiles(argv[2],command);
	BuildFileList(argc-3,argv+3,command);	
	int count=0;
	if(command=='a'){
		count=AddFileList();	
	}else{
		count=0;	
	}
	count=ProcessAllFiles(command,count);
	if(OutputCarFile!=NULL && count!=0){
		WriteEndOfCarHeader();
		if(ferror(OutputCarFile) || fclose(OutputCarFile)==EOF){
			fprintf(stderr,"書き込めませんでした\n");
			exit(1);	
		}
	}
	remove(CarFileName);
	rename(TmpFileName,CarFileName);
	return 0;
}
