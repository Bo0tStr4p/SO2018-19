#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KCYN  "\x1B[36m"

DirectoryHandle* FileSystem_StartUp(const char* filename, DiskDriver* disk, SimpleFS* simple_fs){
	
	DiskDriver_init(disk,filename,1000); 
	DiskDriver_flush(disk);
	
	DirectoryHandle* current_dir = SimpleFS_init(simple_fs,disk);
	if(current_dir == NULL){
		SimpleFS_format(simple_fs);
		current_dir = SimpleFS_init(simple_fs,disk);
		return current_dir;
	}
	else{
		current_dir = SimpleFS_init(simple_fs,disk);
		return current_dir;
	}
	return NULL;
}

int main(int argc, char** argv){
	
	const char* filename = "./file_system.txt";
	
	SimpleFS* simple_fs = (SimpleFS*)malloc(sizeof(SimpleFS));
	if(simple_fs == NULL){
		fprintf(stderr,"%sError: malloc on file_system.\n%s",KRED,KNRM);
		return -1;
	}
	
	DiskDriver* disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(disk == NULL){
		fprintf(stderr,"%sError: malloc on disk.\n%s",KRED,KNRM);
		free(simple_fs);
		return -1;
	}
	
	DirectoryHandle* current_dir = FileSystem_StartUp(filename, disk, simple_fs);
	if(current_dir == NULL){
		fprintf(stderr,"Error: could not start this file_system.\n");
		return -1;
	}
	
	DiskDriver_print_information(disk,filename);
	
	printf("\n\n%s >", current_dir->dcb->fcb.name);
	
	//Libero le strutture e chiudo il programma
	if(current_dir != NULL)
		SimpleFS_close_directory(current_dir);
	if(simple_fs != NULL)
		free(simple_fs);
	if(disk != NULL)
		free(disk);
	return 0;
}
