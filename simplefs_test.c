#include "simplefs.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>

int main(int agc, char** argv) { 						
	
	const char* filename = "./simple_fs_test.txt";
	
	SimpleFS* simple_fs = (SimpleFS*)malloc(sizeof(SimpleFS));
	if(simple_fs == NULL){
		fprintf(stderr,"Error: malloc on simple_fs.\n");
		return -1;
	}
	
	DiskDriver* disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(disk == NULL){
		fprintf(stderr,"Error: malloc on disk.\n");
		return -1;
	}
	
	printf("Initialization of disk driver with 200 blocks");
	DiskDriver_init(disk,filename,200); 
	DiskDriver_flush(disk);
	DiskDriver_print_information(disk,filename);
	
	DirectoryHandle* current_dir = SimpleFS_init(simple_fs,disk);
	if(current_dir == NULL){
		printf("\nsimple_fs_1 non inizializzato. Formatto simple_fs\n");
		SimpleFS_format(simple_fs);
		current_dir = SimpleFS_init(simple_fs,disk);
	}
	else{
		printf("FileSystem inizializzato precedentemente, recupero strutture.\n");
	}
	
	printf("Current directory: %s\n", current_dir->dcb->fcb.name);
	
	printf("\nCreo primo file nella directory / \n");
	
	FileHandle* file1 = SimpleFS_createFile(current_dir,"casa.txt");
	if(file1 == NULL){
		fprintf(stderr,"Error: Could not create file casa.txt\n");
		return -1;
	}
	
	FileHandle* file2 = SimpleFS_createFile(current_dir,"mare.txt");
	if(file2 == NULL){
		fprintf(stderr,"Error: Could not create file mare.txt\n");
		return -1;
	}
	
	FileHandle* file3 = SimpleFS_createFile(current_dir,"luna.txt");
	if(file3 == NULL){
		fprintf(stderr,"Error: Could not create file luna.txt");
		return -1;
	}
	
	FileHandle* file4 = SimpleFS_createFile(current_dir,"sole.txt");
	if(file4 == NULL){
		fprintf(stderr,"Error: Could not create file sole.txt");
		return -1;
	}
	
	FileHandle* file5 = SimpleFS_createFile(current_dir,"casa.txt");
	if(file5 == NULL){
		fprintf(stderr,"Error: Could not create file casa.txt\n");
		return -1;
	}
	
	free(simple_fs);
	free(disk);
	free(current_dir->dcb);
	free(current_dir);
}
