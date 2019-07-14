#include "simplefs.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KCYN  "\x1B[36m"

//Funzione per leggere la directory corrente
int readDirectory(DirectoryHandle* current_dir){
	int i;
	int* flag = (int*)malloc(current_dir->dcb->num_entries * sizeof(int));
	if(flag == NULL){
		fprintf(stderr,"%sError: malloc of flag.\n%s",KRED,KNRM);
		return -1;
	}
	
	char** contents = (char**)malloc(current_dir->dcb->num_entries * sizeof(char*));    
	if(contents == NULL){
		fprintf(stderr,"%sError: malloc of contents.\n%s",KRED,KNRM);
		return -1;
	}
	
    if(SimpleFS_readDir(contents, flag, current_dir) == -1){
        fprintf(stderr,"%sError: could not use readDir.\n%s",KRED,KNRM);
        return -1;
    }
	
	printf("content of %s\n\n",current_dir->dcb->fcb.name);

    for (i = 0; i < current_dir->dcb->num_entries; i++) {
		//IsDir
		if(flag[i]){
			printf("%s%s%s ",KCYN,contents[i],KNRM);
		}
		//IsFile
		else{
			printf("%s%s%s ",KGRN,contents[i],KNRM);
		} 
        free(contents[i]); 
	} 
    free(contents);
    free(flag);
    return 0;
}

int main(int agc, char** argv) { 	
	
	printf("%s\nHello, with this program you can test the correct functioning of the simpleFS library with inode.\n",KYEL);
	printf("Use cat on simplefs_test.txt to see all the changes.\n\n");
	printf("Press any key to continue...");
	getchar();
	
	printf("%s\n-----------------------------------------------------\nTest starting...\n\n",KNRM);					
	
	const char* filename = "./simple_fs_test.txt";
	
	remove(filename);
	
	SimpleFS* simple_fs = (SimpleFS*)malloc(sizeof(SimpleFS));
	if(simple_fs == NULL){
		fprintf(stderr,"%sError: malloc on simple_fs.\n%s",KRED,KNRM);
		return -1;
	}
	
	DiskDriver* disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(disk == NULL){
		fprintf(stderr,"%sError: malloc on disk.\n%s",KRED,KNRM);
		free(simple_fs);
		return -1;
	}
	
	printf("Initialization of disk driver with 200 blocks (Expected: OK)...");
	DiskDriver_init(disk,filename,200); 
	printf("%s OK\n\n%s",KGRN,KYEL);
	DiskDriver_flush(disk);
	DiskDriver_print_information(disk,filename);
	printf("%s",KNRM);
	
	DirectoryHandle* current_dir = SimpleFS_init(simple_fs,disk);
	if(current_dir == NULL){
		printf("\nFileSystem not initialized. Format the FileSystem (Expected: OK)...");
		SimpleFS_format(simple_fs);
		current_dir = SimpleFS_init(simple_fs,disk);
		printf("%s OK%s\n",KGRN,KNRM);
	}
	else{
		printf("FileSystem initialized previously. Recovery of structures.\n");
		current_dir = SimpleFS_init(simple_fs,disk);
	}
	
	printf("\nCurrent directory: %s\n\n", current_dir->dcb->fcb.name);
	
	printf("-----------------------------------------------------\n");
	
	//Testo le create file
	
	printf("\nCreation of casa.txt (Expected: OK)... ");
	FileHandle* file1 = SimpleFS_createFile(current_dir,"casa.txt");
	if(file1 == NULL){
		fprintf(stderr,"%sError: Could not create file casa.txt\n%s",KRED,KNRM);
		free(simple_fs);
		free(disk);
		return -1;
	}
	printf("%sOK%s",KGRN,KNRM);
	
	printf("\nCreation of mare.txt (Expected: OK)... ");
	FileHandle* file2 = SimpleFS_createFile(current_dir,"mare.txt");
	if(file2 == NULL){
		fprintf(stderr,"Error: Could not create file mare.txt\n");
		free(simple_fs);
		free(disk);
		return -1;
	}
	printf("%sOK%s",KGRN,KNRM);
	
	printf("\nCreation of luna.txt (Expected: OK)... ");
	FileHandle* file3 = SimpleFS_createFile(current_dir,"luna.txt");
	if(file3 == NULL){
		fprintf(stderr,"Error: Could not create file luna.txt");
		free(simple_fs);
		free(disk);
		return -1;
	}
	printf("%sOK%s",KGRN,KNRM);
	
	printf("\nCreation of sole.txt (Expected: OK)... ");
	FileHandle* file4 = SimpleFS_createFile(current_dir,"sole.txt");
	if(file4 == NULL){
		fprintf(stderr,"Error: Could not create file sole.txt");
		free(simple_fs);
		free(disk);
		return -1;
	}
	printf("%sOK%s",KGRN,KNRM);
	
	printf("\nCreation of casa.txt (Expected: Error)... ");
	FileHandle* file5 = SimpleFS_createFile(current_dir,"casa.txt");
	if(file5 == NULL){
		printf("%sError: Could not create file casa.txt\n%s",KGRN,KNRM);
	}
	else{
		fprintf(stderr,"%sOK%s",KRED,KNRM);
		free(simple_fs);
		free(disk);
		return -1;
	}
	
	printf("\n-----------------------------------------------------\n\n");
	
	//Leggo il contenuto della directory /
	if(readDirectory(current_dir) == -1){
		fprintf(stderr,"%sError: could not read current dir.\n%s",KRED,KNRM);
		free(simple_fs);
        free(disk);
		return -1;
	}
	printf("\n\n-----------------------------------------------------\n\n");
	
	printf("Open file casa.txt (Expected: OK)... ");
	FileHandle* casa_file_handle = SimpleFS_openFile(current_dir, "casa.txt"); 
    if (casa_file_handle == NULL) {
        fprintf(stderr,"%sError: openFile on casa.txt.\n%s",KRED,KNRM);
        free(simple_fs);
        free(disk);
		return -1;
    }
    printf("%sOK%s, opened file: %s\n", KGRN,KNRM, casa_file_handle->fcb->fcb.name);
    if(casa_file_handle != NULL)
		SimpleFS_close_file(casa_file_handle);
    
    printf("Open file mare.txt (Expected: OK)... ");
	FileHandle* mare_file_handle = SimpleFS_openFile(current_dir, "mare.txt"); 
    if (casa_file_handle == NULL) {
        fprintf(stderr,"%sError: openFile on mare.txt.\n%s",KRED,KNRM);
        free(simple_fs);
        free(disk);
		return -1;
    }
    printf("%sOK%s, opened file: %s\n", KGRN,KNRM, mare_file_handle->fcb->fcb.name);
    if(mare_file_handle != NULL)
		SimpleFS_close_file(mare_file_handle);
	
	printf("Open file auto.txt (Expected: Error)... ");
	FileHandle* auto_file_handle = SimpleFS_openFile(current_dir, "auto.txt"); 
    if (auto_file_handle != NULL){ 
		printf("%sOK%s, opened file: %s\n", KRED,KNRM, auto_file_handle->fcb->fcb.name);
		free(simple_fs);
        free(disk);
		return -1;
	}
	printf("%sError: could not open file auto.txt\n%s",KGRN,KNRM);
    if(auto_file_handle != NULL)
		SimpleFS_close_file(auto_file_handle);
	
	printf("\n-----------------------------------------------------\n\n");
	
	printf("%s\nTESTATO FINO A QUI\n%s",KRED,KNRM);
		
	// Chiudo tutto
	
	//Chiudo i FileHandle Aperti
	if(file1 != NULL)
		SimpleFS_close_file(file1);
	if(file2 != NULL)
		SimpleFS_close_file(file2);
	if(file3 != NULL)
		SimpleFS_close_file(file3);
	if(file4 != NULL)
		SimpleFS_close_file(file4);
	if(file5 != NULL)
		SimpleFS_close_file(file5);
	if(current_dir != NULL)
		SimpleFS_close_directory(current_dir);
	
	//Faccio le free delle strutture create
	free(simple_fs);
	free(disk);
}
