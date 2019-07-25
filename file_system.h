#pragma once
#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KCYN  "\x1B[36m"

#define MAX_ARGUMENTS 2

//Use it to start up the file system
DirectoryHandle* FileSystem_StartUp(const char* filename, DiskDriver* disk, SimpleFS* simple_fs);

//Usiamo questa funzione per creare una nuova cartella nella directory corrente
void FileSystem_mkdir(int arguments_number,char* arguments[MAX_ARGUMENTS]);

//Funzione per scrivere all'interno di un file
void FileSystem_write(int arguments_number,char* arguments[MAX_ARGUMENTS]);

//Funzione per leggere il contenuto di un file
void FileSystem_more(int arguments_number, char* arguments[MAX_ARGUMENTS]);

//Funzione per creare un nuovo file
void FileSystem_touch(int arguments_number,char* arguments[MAX_ARGUMENTS]);

//Funzione per cambiare directory
void FileSystem_cd(int arguments_number, char* arguments[MAX_ARGUMENTS]);

//Funzione per leggere la directory corrente
void FileSystem_ls(void);

//Funzione per eliminare file o cartelle
void FileSystem_rm(int arguments_number, char* arguments[MAX_ARGUMENTS]);

//Funzione per stampare le istruzione
void FileSystem_help(void);

//Funzione per leggere le info del sistema
void FileSystem_info(const char* filename);
