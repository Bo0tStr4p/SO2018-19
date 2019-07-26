#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>	
#include "file_system.h"

SimpleFS* simple_fs;
DiskDriver* disk;
DirectoryHandle* current_dir;

char my_path[256] = "/";

DirectoryHandle* FileSystem_StartUp(const char* filename, DiskDriver* disk, SimpleFS* simple_fs){
	
	DiskDriver_init(disk,filename,1000); 
	DiskDriver_flush(disk);
	
	DirectoryHandle* current = SimpleFS_init(simple_fs,disk);
	if(current == NULL){
		SimpleFS_format(simple_fs);
		current = SimpleFS_init(simple_fs,disk);
		return current;
	}
	else{
		SimpleFS_close_directory(current);
		current = SimpleFS_init(simple_fs,disk);
		return current;
	}
	return NULL;
}

void FileSystem_updatePath(const char* path){
	 if(strcmp(path, "..") != 0){
		if(strlen(my_path) != 1)
			strcat(my_path,"/");
		strcat(my_path, path);
		return;
	}
	else{
		char * pch;
		pch=strrchr(my_path,'/');
		*pch = 0x00;
		if(strlen(my_path) == 0)
			strcat(my_path,"/");
		return;
	}
	return;
}

void my_function(int sig){ 
	//Libero le strutture e chiudo il programma
	printf("\nBye!\n\n");
	if(current_dir != NULL)
		SimpleFS_close_directory(current_dir);
	if(simple_fs != NULL)
		free(simple_fs);
	if(disk != NULL)
		free(disk);
	exit(EXIT_SUCCESS);
}

void FileSystem_mkdir(int arguments_number,char* arguments[MAX_ARGUMENTS]){
	if (arguments_number != 2) {
        printf("%sUsage: mkdir directory_name\n%s",KRED,KNRM);
        return;
    }

    if (SimpleFS_mkDir(current_dir, arguments[1]) == -1){
        fprintf(stderr, "%sError: could not create directory %s.\n%s",KRED, arguments[1], KNRM);
		return;
	}
	return;
}

void FileSystem_write(int arguments_number,char* arguments[MAX_ARGUMENTS]){
	 if(arguments_number != 2) {
        fprintf(stderr, "%sUsage write file_name\n%s", KRED, KNRM);
        return;
    }
    
    FileHandle* file_h = SimpleFS_openFile(current_dir, arguments[1]);
    if (file_h == NULL) {
        fprintf(stderr, "%sError in FileSystem_write: could not open file %s.\n%s", KRED, arguments[1], KNRM);
        return;
    }
    
    int start;
    printf("Enter starting point: ");
    scanf("%d", &start); 
    getchar();
    
    if (SimpleFS_seek(file_h,start) == -1) {
		fprintf(stderr, "%sError in FileSystem_write: could not seeking in file %s.\n%s", KRED, arguments[1], KNRM);
        SimpleFS_close_file(file_h);
        return;
    }
    
    printf("Enter text: \n");
    char* text = (char*) malloc(BLOCK_SIZE * sizeof(char));
    if(text == NULL){
		fprintf(stderr, "%sError in FileSystem_write: malloc on text.\n%s\n", KRED, KNRM);
		SimpleFS_close_file(file_h);
		return;
	}
    
    fgets(text, BLOCK_SIZE, stdin);
    text[strlen(text) - 1] = 0x00;
    
    if (SimpleFS_write(file_h, text, strlen(text)) == -1) {
        fprintf(stderr, "%sError in FileSystem_write: could not writing in file %s.\n%s", KRED, arguments[1], KNRM);
        free(text);
        SimpleFS_close_file(file_h);
        return;
    }

    free(text);
    SimpleFS_close_file(file_h);
    
	return;
}

void FileSystem_more(int arguments_number, char* arguments[MAX_ARGUMENTS]){
	if (arguments_number != 2) {
        fprintf(stderr, "%sUsage more file_name\n%s", KRED, KNRM);
        return;
    }
    FileHandle* file_h = SimpleFS_openFile(current_dir, arguments[1]);
    if (file_h == NULL) {
        fprintf(stderr, "%sError in FileSystem_more: could not open file %s.\n%s", KRED, arguments[1], KNRM);
        return;
    }

    int file_size = file_h->fcb->fcb.written_bytes;
    char* text = (char*) malloc((file_size+1)*sizeof(char));
     if(text == NULL){
		fprintf(stderr, "%sError in FileSystem_more: malloc on text.\n%s\n", KRED, KNRM);
		SimpleFS_close_file(file_h);
		return;
	}
    
    
    if (SimpleFS_read(file_h, text, file_size) == -1) {
        fprintf(stderr, "%sError in FileSystem_more: could not read file %s.\n%s", KRED, arguments[1], KNRM);
        free(text);
        SimpleFS_close_file(file_h);
        return;
    }
	
	text[file_size] = '\0';
    printf("%s \n",text);
    

    free(text);
    SimpleFS_close_file(file_h);
	
	return;
}

void FileSystem_touch(int arguments_number, char* arguments[MAX_ARGUMENTS]){
	 if (arguments_number != 2) {
		fprintf(stderr, "%sUsage touch file_name\n%s", KRED, KNRM);
        return;
    }
    
    FileHandle* file = SimpleFS_createFile(current_dir, arguments[1]);
   
    if (file == NULL) 
		fprintf(stderr, "%sError in FileSystem_touch: could not create file %s.\n%s", KRED, arguments[1], KNRM);
	
	SimpleFS_close_file(file);
}

void FileSystem_cd(int arguments_number, char* arguments[MAX_ARGUMENTS]){
	if (arguments_number != 2) {
        printf("%sUsage: cd directory_name\n%s",KRED,KNRM);
        return;
    }

    if (SimpleFS_changeDir(current_dir, arguments[1]) == -1){
        fprintf(stderr, "%sError: could not change directory.%s\n",KRED,KNRM);
		return;
	}
	
	FileSystem_updatePath(arguments[1]);
	
	return;
}

void FileSystem_ls(void){
	int i;
	
	int* flag = (int*)malloc((current_dir->dcb->num_entries) * sizeof(int));
	for (i = 0; i < current_dir->dcb->num_entries; i++) {
		flag[i] = -1;
	}
	if(flag == NULL){
		fprintf(stderr,"%sError in readDirectory: malloc of flag.\n%s",KRED,KNRM);
		return;
	}
	
	char** contents = (char**)malloc((current_dir->dcb->num_entries) * sizeof(char*));
	if(contents == NULL){
		fprintf(stderr,"%sError in readDirectory: malloc of contents.\n%s",KRED,KNRM);
		return;
	}
	
	for (i = 0; i < (current_dir->dcb->num_entries); i++) {
		contents[i] = (char*)malloc(128*sizeof(char));
	}
	
    if(SimpleFS_readDir(contents, flag, current_dir) == -1){
        //fprintf(stderr,"%sError: could not use readDir.\n%s",KRED,KNRM);
        free(flag);
        free(contents);
        return ;
    }

    for (i = 0; i < current_dir->dcb->num_entries; i++) {
		//IsDir
		if(flag[i] == 1){
			printf("%s%s%s ",KYEL,contents[i],KNRM);
		}
		//IsFile
		else if (flag[i] == 0){
			if(contents[i] != NULL){
				printf("%s%s%s ",KGRN,contents[i],KNRM);
			}
		}
        free(contents[i]); 
	} 
	printf("\n");
    free(contents);
    free(flag);
	return;
}

void FileSystem_rm(int arguments_number, char* arguments[MAX_ARGUMENTS]){
	if (arguments_number != 2) {
        fprintf(stderr, "%sUsage rm file_name or directory_name\n%s", KRED, KNRM);
        return;
    }

    if (SimpleFS_remove(current_dir, arguments[1]) == -1) 
        fprintf(stderr, "%sError in FileSystem_rm: could not remove %s.\n%s", KRED, arguments[1], KNRM);
	
}

void FileSystem_help(void){
	printf("\nUsage mkdir <directory_name> to create a new directory.\n");
	printf("Usage write <file_name> to write inside it.\n");
	printf("Usage more <file_name> to read inside it.\n");
	printf("Usage touch <file_name> to create a new file.\n");
	printf("Usage cd <directory_name> to go to this directory or cd .. to go to parent directory.\n");
	printf("Usage ls to show directory contents.\n");
	printf("Usage rm <file or directory name> to remove a file or directory with its contents.\n");
	printf("Usage info to show informations about your disk.\n");
	printf("Usage quit or press CTRL+C to stop the file_system.\n\n");
	return;
}

void FileSystem_info(const char* filename){
	printf("\n%s",KYEL);
	DiskDriver_print_information(disk,filename);
	printf("%s\n",KNRM);
	return;
}


int main(int argc, char *argv[]){
	
	//R. Uso CTRL+C per interrompere il programma
	signal(SIGINT, my_function);
	
	const char* filename = "./file_system.txt";
	
	char command[256];
	
	simple_fs = (SimpleFS*)malloc(sizeof(SimpleFS));
	if(simple_fs == NULL){
		fprintf(stderr,"%sError: malloc on file_system.\n%s",KRED,KNRM);
		return -1;
	}
	
	disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(disk == NULL){
		fprintf(stderr,"%sError: malloc on disk.\n%s",KRED,KNRM);
		free(simple_fs);
		return -1;
	}
	
	current_dir = FileSystem_StartUp(filename, disk, simple_fs);
	if(current_dir == NULL){
		fprintf(stderr,"Error: could not start this file_system.\n");
		return -1;
	}
	
	//ISTRUZIONI
	printf("Hi! This is our file_system, your disk is %s. If it is your first lauch, use help to show more informations. \nGood Luck! Created by Alessandro and Romeo.\n\n", filename);
	
	while(1){
		char* arguments[MAX_ARGUMENTS] = {NULL};

        printf("%s > ", my_path);
        fgets(command, 256, stdin);
        
        arguments[0] = strtok(command, " ");
        arguments[1] = strtok(NULL, "\n");
        int arguments_number = (arguments[1] == NULL) ? 1 : 2;
        if (arguments[1] == NULL) 
            arguments[0][strlen(arguments[0]) - 1] = 0x00;

        if (strcmp(arguments[0], "mkdir") == 0) {
            FileSystem_mkdir(arguments_number, arguments); 
        }
        else if (strcmp(arguments[0], "write") == 0) {
            FileSystem_write(arguments_number, arguments); 
        }
        else if (strcmp(arguments[0], "more") == 0) {
            FileSystem_more(arguments_number, arguments); 
        }
        else if (strcmp(arguments[0], "touch") == 0) {
            FileSystem_touch(arguments_number, arguments); 
        }
        else if (strcmp(arguments[0], "cd") == 0) {
            FileSystem_cd(arguments_number, arguments); 
        }
        else if (strcmp(arguments[0], "ls") == 0) {
            FileSystem_ls(); 
        }
        else if (strcmp(arguments[0], "rm") == 0) {
            FileSystem_rm(arguments_number, arguments); 
        }
        else if (strcmp(arguments[0], "help") == 0) {
            FileSystem_help(); 
        }
        else if (strcmp(arguments[0], "info") == 0) {
            FileSystem_info(filename); 
        }
        else if (strcmp(arguments[0], "quit") == 0){
			my_function(SIGINT);
		}
        else {
            printf("Invalid command\n");
        }
	}
	
	return 0;
}
