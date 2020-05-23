#include "car.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char *argv[]){

	setbuf(stdout,NULL);
	setbuf(stderr,NULL);
	fprintf(stderr,"CAR 1.3：");	
	BuildCRCTable();
	int command=ParseArguments(argc,argv);	
	fprintf(stderr,"\n");
	OpenArchiveFiles(argv[2],command);
	BuildFileList(argc-3,argv+3,command);	
	int count=0;
	if(command=='a'){
		count=AddFileList2Archive();	
	}
	
	if(command=='l'){
		PrintTitle();	
	}
	count=ProcessAllFiles(command,count);
	if(OutputCarFile!=NULL && count!=0){
		WriteEndOfCarHeader();
		if(ferror(OutputCarFile) || fclose(OutputCarFile)==EOF){
			fprintf(stderr,"書き込めませんでした\n");
			exit(1);	
		}
		remove(CarFileName);	
		rename(TmpFileName,CarFileName);
	}
	fprintf(stderr,"\n%d個のファイル\n",count);	

	return 0;
}
