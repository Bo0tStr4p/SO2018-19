#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){
	if(fs == NULL || disk == NULL) return NULL;			//A. innanzitutto controllo che fs e disk non siano vuoti
	
	FirstDirectoryBlock* first_directory_block = malloc(sizeof(FirstDirectoryBlock));
	
	int res = DiskDriver_readBlock(disk,first_directory_block,0);
	if(res == -1){ 										//A. controllo che il blocco sia disponibile. Se non è disponibile, non possiamo andare avanti
		//printf("Blocco non disponibile\n");
		free(first_directory_block);
		return NULL;
	};				
	
	fs->disk = disk;
	
	DirectoryHandle* directory_handle = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));		//A. Il blocco è disponibile, quindi posso allocare la struttura
	directory_handle->sfs = fs;
	directory_handle->dcb = first_directory_block;
	directory_handle->directory = NULL;
	directory_handle->current_block = NULL;
	directory_handle->pos_in_dir = 0;
	directory_handle->pos_in_block = 0;
	
	return directory_handle;
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
	if(fs == NULL) return;
	
	//A. Creiamo le strutture iniziali
	FirstDirectoryBlock root_directory = {}; 				//A. Top Level Directory
	root_directory.index.previous = NULL;					//A. Ci troviamo nel primo blocco index, non ci sono predecessori 
	root_directory.index.first_position_free = 0;			//A. imposto che il primo index libero nell'array degli index è 0
	root_directory.index.next = NULL;						//A. Essendo appena inizializzato, ancora non c'è un successivo blocco index, quello corrente non è pieno
	
	root_directory.fcb.directory_block = -1; 				//A. root non ha genitori
	root_directory.fcb.block_in_disk = 0;					//A. root ha il primo blocco nel disco
	root_directory.fcb.is_dir = 1; 							//A. è una directory
	strcpy(root_directory.fcb.name, "/"); 					//A. il suo nome è "/"
	
	//A. puliamo la bitmap dei blocchi occupati nel disco
	fs->disk->header->free_blocks = fs->disk->header->num_blocks;	//A. il numero dei blocchi liberi è uguale a quello dei blocchi totali sul disco. Cioè il disco è vuoto
	fs->disk->header->first_free_block = 0;							//A. il primo blocco libero ovviamento è quello 0
	
	int bitmap_size = fs->disk->header->bitmap_entries; 			//A. numero di blocchi della bitmap
	memset(fs->disk->bitmap_data,'\0', bitmap_size); 				//A. mappo con 0 (blocco libero) l'array bitmap_data
	
	int ret = DiskDriver_writeBlock(fs->disk, &root_directory, 0);		//A. vado a scrivere sul disco la root directory
	if (ret == -1)
		printf("Error: impossibile formattare\n");
	
	return;
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename){
	return NULL;
}

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d){
	return 0;
}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
	return 0;
}


// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){
	return 0;
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
	return 0;
}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){
	return 0;
}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){
	return 0;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){
 	return 0;
 }

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
	return 0;
}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename){
	return 0;
}


  
