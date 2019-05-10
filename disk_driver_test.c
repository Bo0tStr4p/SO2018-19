#include "disk_driver.h"
#include "bitmap.h"
#include "simplefs.h"

/* Importo anche simplefs in modo tale da utilizzare FileBlock con BlockHeader
 * e simulare la scrittura dei blocchi del file system 
 * */

//R. Funzione per creare dei blockHeader example usati per ogni FileBlock
BlockHeader create_block_header(int value){
	BlockHeader header = {
		.previous_block = value - 1, //R. Valori random, solo per test
		.next_block = value ,
		.block_in_file = value + 1
	};
	
	return header;
}

//R. Funzione utilizzata per creare dei FileBlock example per testare il disk driver
FileBlock* create_file_block(char value){
	
	BlockHeader header = create_block_header((int)value);
	FileBlock* file_block = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block == NULL){
		fprintf(stderr,"Error: malloc create_file_block\n");
		return;
	}
	file_block->header = header;
	
	//R. Scrivo dei valori per riempire i blocchi
	char data_block[BLOCK_SIZE-sizeof(BlockHeader)];
	for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++) data_block[i] = value;
	data_block[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0';
	strcpy(file_block->data, data_block);
	
	return fileblock;
}

int main(int argc, char** argv){
	printf("DISK_DRIVER_TEST STARTING...\n\n");
	
	//R. Inizializzo la memoria del mio disco
	DiskDriver* my_disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(my_disk == NULL){
		fprintf(stderr,"Error: malloc of DiskDriver\n");
		return -1;
	}
	
	printf("INITIALIZATION OF 7 BLOCKS ...\n");
	
	//R. Inizializzo 5 File Block per i test
	printf("Block 1... ");
	FileBlock* block1 = create_file_block('1');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 1\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 2... ");
	FileBlock* block2 = create_file_block('2');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 2\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 3... ");
	FileBlock* block3 = create_file_block('3');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 3\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 4... ");
	FileBlock* block4 = create_file_block('4');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 4\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 5... ");
	FileBlock* block5 = create_file_block('5');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 5\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 6... ");
	FileBlock* block6 = create_file_block('6');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 6\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 7... ");
	FileBlock* block7 = create_file_block('7');
	if(block1 == NULL){
		fprintf(stderr, "Error: could not create block 7\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("INITIALIZATION OF BLOCKS COMPLETED\n\n");
	printf("INITIALIZATION OF DISK DRIVER WITH 6 BLOCKS ...\n");
	
	DiskDriver_init(my_disk,/*INSERIRE IL FILENAME*/,6);
	DiskDriver_flush(my_disk);
	DiskDriver_print_information(my_disk,/*INSERIRE IL FILENAME*/);
	
	
	
}
