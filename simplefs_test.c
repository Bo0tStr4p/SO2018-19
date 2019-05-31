#include "simplefs.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>

int main(int agc, char** argv) { 						
	
	const char* filename = "./disk_driver_fs_test.txt";
	
	SimpleFS* simple_fs_1 = (SimpleFS*)malloc(sizeof(SimpleFS));
	
	DiskDriver* disk_1 = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(disk_1 == NULL){
		printf("Errore nella malloc di disk_1\n");
		return -1;
	}
	
	
	printf("Inizializzazione di disk_1\n");
	DiskDriver_init(disk_1,filename,6); 
	printf("Ok, disk_1 inizializzato\n");
	
	
	DirectoryHandle* directory_handle_1 = SimpleFS_init(simple_fs_1,disk_1);
	
	printf("directory_handle->pos_in_dir:%i\n",directory_handle_1->pos_in_dir);
	
	printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
	printf("DataBlock size %ld\n", sizeof(FileBlock));
	printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
	printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));
  
}
