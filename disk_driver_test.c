#include "disk_driver.h"
#include "bitmap.h"
#include "simplefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	
	int i;
	
	BlockHeader header = create_block_header((int)value);
	FileBlock* file_block = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block == NULL){
		fprintf(stderr,"Error: malloc create_file_block\n");
		return NULL;
	}
	file_block->header = header;
	
	//R. Scrivo dei valori per riempire i blocchi
	char data_block[BLOCK_SIZE-sizeof(BlockHeader)];
	for(i = 0; i < BLOCK_SIZE-sizeof(BlockHeader); i++) data_block[i] = value;
	data_block[BLOCK_SIZE-sizeof(BlockHeader)-1] = '\0';
	strcpy(file_block->data, data_block);
	
	return file_block;
}

int main(int argc, char** argv){
	
	printf("Hello, with this program you can test the correct functioning of the disk_driver library.\n");
	printf("Use cat on disk_driver_test.txt to see all the changes.\n\n");
	printf("Press any key to continue...");
	getchar();
	
	printf("-----------------------------------------------------\nTEST STARTING...\n\n");
	
	const char* filename = "./disk_driver_test.txt";
	
	//R. USARE SOLO DURANTE L'IMPLEMENTAZIONE DEI TEST, DOPO RIMUOVERE
	int status = remove(filename);
	if(status == -1){
		fprintf(stderr,"Error: remove file\n");
	}
	
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
	if(block2 == NULL){
		fprintf(stderr, "Error: could not create block 2\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 3... ");
	FileBlock* block3 = create_file_block('3');
	if(block3 == NULL){
		fprintf(stderr, "Error: could not create block 3\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 4... ");
	FileBlock* block4 = create_file_block('4');
	if(block4 == NULL){
		fprintf(stderr, "Error: could not create block 4\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 5... ");
	FileBlock* block5 = create_file_block('5');
	if(block5 == NULL){
		fprintf(stderr, "Error: could not create block 5\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 6... ");
	FileBlock* block6 = create_file_block('6');
	if(block6 == NULL){
		fprintf(stderr, "Error: could not create block 6\n");
		return -1;
	}
	printf("Ok\n");
	
	printf("Block 7... ");
	FileBlock* block7 = create_file_block('7');
	if(block7 == NULL){
		fprintf(stderr, "Error: could not create block 7\n");
		return -1;
	}
	printf("Ok\n");
	
	/*=========================================================*/
	/*R. INIZIALIZZO IL MIO DISCO*/
	
	printf("INITIALIZATION OF BLOCKS COMPLETED\n\n");
	
	printf("INITIALIZATION OF DISK DRIVER WITH 6 BLOCKS ...");
	
	DiskDriver_init(my_disk,filename,6);
	DiskDriver_flush(my_disk);
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	/*=========================================================*/
	/*R. INIZIO A SCRIVERE I BLOCCHI SUL DISCO*/
	
	int free_block;
	
	printf("WRITING BLOCK 1 TO DISK... ");
	
	if(DiskDriver_writeBlock(my_disk, block1, 0) == -1){
		fprintf(stderr, "Error: could not write block 1 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	printf("WRITING BLOCK 2 TO DISK... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 0);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}

	if(DiskDriver_writeBlock(my_disk, block2, free_block) == -1){
		fprintf(stderr, "Error: could not write block 2 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	printf("WRITING BLOCK 3 TO DISK... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 1);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block3, free_block) == -1){
		fprintf(stderr, "Error: could not write block 3 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	printf("WRITING BLOCK 4 TO DISK... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 2);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block4, free_block) == -1){
		fprintf(stderr, "Error: could not write block 4 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	printf("WRITING BLOCK 5 TO DISK... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 3);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block5, free_block) == -1){
		fprintf(stderr, "Error: could not write block 5 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	printf("WRITING BLOCK 6 TO DISK... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 4);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block6, free_block) == -1){
		fprintf(stderr, "Error: could not write block 6 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	/*IN QUESTO CASO QUI NON DEVO RIUSCIRE A SCRIVERE, PERCHE' LA MEMORIA DEL DISCO E' PIENA*/
	printf("WRITING BLOCK 7 TO DISK (Expected: Error) ... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 5);
	if(free_block == -1){
		printf("Error");
	}
	else{
		printf("OK");
		return -1;
	}
	
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	printf("OK\n");
	
	/*=========================================================*/
	/*R. INIZIO A LEGGERE I BLOCCHI SUL DISCO*/
	FileBlock* block_test = (FileBlock*)malloc(sizeof(FileBlock));
	if(block_test == NULL){
		fprintf(stderr, "Error: malloc with block_test\n");
		return -1;
	}
	
	printf("READING BLOCK 1 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 0) == -1){
		fprintf(stderr, "Error: could not read block 1 to disk\n");
		return -1;
	}
	printf("OK\nDATA:\n");
	printf("%s\n\n", block_test->data);
	
	printf("READING BLOCK 2 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 1) == -1){
		fprintf(stderr, "Error: could not read block 2 to disk\n");
		return -1;
	}
	printf("OK\nDATA:\n");
	printf("%s\n\n", block_test->data);
	
	printf("READING BLOCK 3 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 2) == -1){
		fprintf(stderr, "Error: could not read block 2 to disk\n");
		return -1;
	}
	printf("OK\nDATA:\n");
	printf("%s\n\n", block_test->data);
	
	printf("READING BLOCK 4 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 3) == -1){
		fprintf(stderr, "Error: could not read block 4 to disk\n");
		return -1;
	}
	printf("OK\nDATA:\n");
	printf("%s\n\n", block_test->data);
	
	printf("READING BLOCK 5 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 4) == -1){
		fprintf(stderr, "Error: could not read block 5 to disk\n");
		return -1;
	}
	printf("OK\nDATA:\n");
	printf("%s\n\n", block_test->data);
	
	printf("READING BLOCK 6 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 5) == -1){
		fprintf(stderr, "Error: could not read block 6 to disk\n");
		return -1;
	}
	printf("OK\nDATA:\n");
	printf("%s\n\n", block_test->data);
	
	printf("READING BLOCK 7 TO DISK... ");
	if(DiskDriver_readBlock(my_disk, block_test, 6) == -1){
		printf("Error\n");
	}
	else{
		fprintf(stderr,"OK\n");
		return -1;
	}
	
	/*=========================================================*/
	/*R. LIBERO I VARI BLOCCHI*/
	
	printf("FREE BLOCK 0...");
	if(DiskDriver_freeBlock(my_disk, 0) == -1){
		fprintf(stderr, "Error: could not free block 1 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 0:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 0) == -1){
		fprintf(stderr,"ERROR\n");
	}
	else{
		printf("OK\n");
	}
	
	printf("FREE BLOCK 1...");
	if(DiskDriver_freeBlock(my_disk, 1) == -1){
		fprintf(stderr, "Error: could not free block 1 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 1:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 1) == -1){
		fprintf(stderr,"ERROR");
	}
	else{
		printf("OK\n");
	}
	
	printf("FREE BLOCK 2...");
	if(DiskDriver_freeBlock(my_disk, 2) == -1){
		fprintf(stderr, "Error: could not free block 2 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 2:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 2) == -1){
		fprintf(stderr,"ERROR");
	}
	else{
		printf("OK\n");
	}
	
	printf("FREE BLOCK 3...");
	if(DiskDriver_freeBlock(my_disk, 3) == -1){
		fprintf(stderr, "Error: could not free block 3 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 3:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 3) == -1){
		fprintf(stderr,"ERROR");
	}
	else{
		printf("OK\n");
	}
	
	printf("FREE BLOCK 4...");
	if(DiskDriver_freeBlock(my_disk, 4) == -1){
		fprintf(stderr, "Error: could not free block 4 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 4:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 4) == -1){
		fprintf(stderr,"ERROR");
	}
	else{
		printf("OK\n");
	}
	
	printf("FREE BLOCK 5...");
	if(DiskDriver_freeBlock(my_disk, 5) == -1){
		fprintf(stderr, "Error: could not free block 5 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 5:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 5) == -1){
		fprintf(stderr,"ERROR");
	}
	else{
		printf("OK\n");
	}
	
	printf("FREE BLOCK 6...");
	if(DiskDriver_freeBlock(my_disk, 6) == -1){
		fprintf(stderr, "Error: could not free block 6 to disk\n");
		return -1;
	}
	printf("OK\n");
	printf("TRYING TO READ BLOCK 6:\n");
	if(DiskDriver_readBlock(my_disk, block_test, 6) == -1){
		fprintf(stderr,"ERROR");
	}
	else{
		printf("OK\n");
	}
	
	/*=========================================================*/
	/*R. END*/
	
	free(my_disk);
	free(block_test);
	free(block1);
	free(block2);
	free(block3);
	free(block4);
	free(block5);
	free(block6);
	free(block7);
	
	printf("END\n\n");
	
	return 0;
	
}
