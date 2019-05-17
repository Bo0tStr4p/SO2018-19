#include "disk_driver.h"
#include "bitmap.h"
#include "simplefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"

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
	
	printf("%s\nHello, with this program you can test the correct functioning of the disk_driver library.\n",KYEL);
	printf("Use cat on disk_driver_test.txt to see all the changes.\n\n");
	printf("Press any key to continue...");
	getchar();
	
	printf("%s-----------------------------------------------------\nTest starting...\n\n",KNRM);
	
	const char* filename = "./disk_driver_test.txt";
	
	//R. USARE SOLO DURANTE L'IMPLEMENTAZIONE DEI TEST, DOPO RIMUOVERE
	int status = remove(filename);
	if(status == -1){
		fprintf(stderr,"%s Error: remove file\n %s",KRED,KNRM);
	}
	
	//R. Inizializzo la memoria del mio disco
	DiskDriver* my_disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(my_disk == NULL){
		fprintf(stderr,"%sError: malloc of DiskDriver\n%s",KRED,KNRM);
		return -1;
	}
	
	printf("Initialization of 7 blocks ...\n");
	
	//R. Inizializzo 5 File Block per i test
	printf("Block 1 (Expected: Ok)... ");
	FileBlock* block1 = create_file_block('1');
	if(block1 == NULL){
		fprintf(stderr, "%sError: could not create block 1\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("Block 2 (Expected: Ok)... ");
	FileBlock* block2 = create_file_block('2');
	if(block2 == NULL){
		fprintf(stderr, "%sError: could not create block 2\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("Block 3 (Expected: Ok)... ");
	FileBlock* block3 = create_file_block('3');
	if(block3 == NULL){
		fprintf(stderr, "%sError: could not create block 3\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("Block 4 (Expected: Ok)... ");
	FileBlock* block4 = create_file_block('4');
	if(block4 == NULL){
		fprintf(stderr, "%sError: could not create block 4\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("Block 5 (Expected: Ok)... ");
	FileBlock* block5 = create_file_block('5');
	if(block5 == NULL){
		fprintf(stderr, "%sError: could not create block 5\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("Block 6 (Expected: Ok)... ");
	FileBlock* block6 = create_file_block('6');
	if(block6 == NULL){
		fprintf(stderr, "%sError: could not create block 6\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("Block 7 (Expected: Ok)... ");
	FileBlock* block7 = create_file_block('7');
	if(block7 == NULL){
		fprintf(stderr, "%sError: could not create block 7\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	
	printf("%sInitialization of blocks completed%s\n\n",KGRN,KNRM);
	
	/*=========================================================*/
	/*R. INIZIALIZZO IL MIO DISCO*/
	
	printf("Initialization of disk driver with 6 blocks (Expected: Ok)...");
	DiskDriver_init(my_disk,filename,6); 
	printf("%sOk\n%s",KGRN,KNRM);
	DiskDriver_flush(my_disk);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s",KNRM);
	
	/*=========================================================*/
	/*R. INIZIO A SCRIVERE I BLOCCHI SUL DISCO*/
	
	int free_block;
	
	printf("\nWriting block 1 to disk (Expected: Ok)... ");
	
	if(DiskDriver_writeBlock(my_disk, block1, 0) == -1){
		fprintf(stderr, "%sError: could not write block 1 to disk\n%s",KRED,KNRM);
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n\n%s",KGRN,KNRM);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	printf("Writing block 2 to disk (Expected: Ok)... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 0);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	printf("FREE_BLOCK IS: %d\n",free_block);

	if(DiskDriver_writeBlock(my_disk, block2, free_block) == -1){
		fprintf(stderr, "%sError: could not write block 2 to disk\n%s",KRED,KNRM);
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n\n%s",KGRN,KNRM);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	printf("Writing block 3 to disk (Expected: Ok)... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 1);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	printf("FREE_BLOCK IS: %d\n",free_block);
	
	if(DiskDriver_writeBlock(my_disk, block3, free_block) == -1){
		fprintf(stderr, "%sError: could not write block 3 to disk\n%s",KRED,KNRM);
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n\n%s",KGRN,KNRM);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	printf("Writing block 4 to disk (Expected: Ok)... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 2);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block4, free_block) == -1){
		fprintf(stderr, "%sError: could not write block 4 to disk\n%s",KRED,KNRM);
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n\n%s",KGRN,KNRM);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	printf("Writing block 5 to disk (Expected: Ok)... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 3);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block5, free_block) == -1){
		fprintf(stderr, "%sError: could not write block 5 to disk\n%s",KRED,KNRM);
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n\n%s",KGRN,KNRM);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	printf("Writing block 6 to disk (Expected: Ok)... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 4);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	if(DiskDriver_writeBlock(my_disk, block6, free_block) == -1){
		fprintf(stderr, "%sError: could not write block 6 to disk\n%s",KRED,KNRM);
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n\n%s",KGRN,KNRM);
	printf("%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	/*IN QUESTO CASO QUI NON DEVO RIUSCIRE A SCRIVERE, PERCHE' LA MEMORIA DEL DISCO E' PIENA*/
	printf("Writing block 7 to disk (Expected: Error)... ");
	
	free_block = DiskDriver_getFreeBlock(my_disk, 5);
	if(free_block == -1){
		printf("%sError\n%s",KGRN,KNRM);
	}
	else{
		printf("%sOk%s",KRED,KNRM);
		return -1;
	}
	
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "%sError: flush\n%s",KRED,KNRM);
		return -1;
	}
	printf("\n%s",KYEL);
	DiskDriver_print_information(my_disk,filename);
	printf("%s\n",KNRM);
	
	/*=========================================================*/
	/*R. INIZIO A LEGGERE I BLOCCHI SUL DISCO*/
	FileBlock* block_test = (FileBlock*)malloc(sizeof(FileBlock));
	if(block_test == NULL){
		fprintf(stderr, "%sError: malloc with block_test\n%s",KRED,KNRM);
		return -1;
	}
	
	printf("Reading block 1 to disk (Expected: Ok)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 0) == -1){
		fprintf(stderr, "%sError: could not read block 1 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOK%s\nDATA:\n",KGRN,KNRM);
	printf("%s%s%s\n\n", KYEL, block_test->data, KNRM);
	
	printf("Reading block 2 to disk (Expected: Ok)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 1) == -1){
		fprintf(stderr, "%sError: could not read block 2 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOK%s\nDATA:\n",KGRN,KNRM);
	printf("%s%s%s\n\n", KYEL, block_test->data, KNRM);
	
	printf("Reading block 3 to disk (Expected: Ok)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 2) == -1){
		fprintf(stderr, "%sError: could not read block 3 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOK%s\nDATA:\n",KGRN,KNRM);
	printf("%s%s%s\n\n", KYEL, block_test->data, KNRM);
	
	printf("Reading block 4 to disk (Expected: Ok)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 3) == -1){
		fprintf(stderr, "%sError: could not read block 4 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOK%s\nDATA:\n",KGRN,KNRM);
	printf("%s%s%s\n\n", KYEL, block_test->data, KNRM);
	
	printf("Reading block 5 to disk (Expected: Ok)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 4) == -1){
		fprintf(stderr, "%sError: could not read block 5 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOK%s\nDATA:\n",KGRN,KNRM);
	printf("%s%s%s\n\n", KYEL, block_test->data, KNRM);
	
	printf("Reading block 6 to disk (Expected: Ok)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 5) == -1){
		fprintf(stderr, "%sError: could not read block 6 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOK%s\nDATA:\n",KGRN,KNRM);
	printf("%s%s%s\n\n", KYEL, block_test->data, KNRM);
	
	printf("Reading block 7 to disk (Expected: Error)... ");
	if(DiskDriver_readBlock(my_disk, block_test, 6) == -1){
		printf("%sError\n%s",KGRN,KNRM);
	}
	else{
		fprintf(stderr,"%sOk\n%s",KRED,KNRM);
		return -1;
	}
	
	/*=========================================================*/
	/*R. LIBERO I VARI BLOCCHI*/
	
	printf("\nFree block 1 (Expected: Ok)...");
	if(DiskDriver_freeBlock(my_disk, 0) == -1){
		fprintf(stderr, "%sError: could not free block 1 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	if(DiskDriver_readBlock(my_disk, block_test, 0) != -1){
		fprintf(stderr,"Ok\n");
		return -1;
	}
	
	printf("\n\nFree block 2 (Expected: Ok)...");
	if(DiskDriver_freeBlock(my_disk, 1) == -1){
		fprintf(stderr, "%sError: could not free block 2 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	if(DiskDriver_readBlock(my_disk, block_test, 1) != -1){
		fprintf(stderr,"Ok\n");
		return -1;
	}
	
	printf("\n\nFree block 3 (Expected: Ok)...");
	if(DiskDriver_freeBlock(my_disk, 2) == -1){
		fprintf(stderr, "%sError: could not free block 3 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	if(DiskDriver_readBlock(my_disk, block_test, 2) != -1){
		fprintf(stderr,"Ok\n");
		return -1;
	}
	
	printf("\n\nFree block 4 (Expected: Ok)...");
	if(DiskDriver_freeBlock(my_disk, 3) == -1){
		fprintf(stderr, "%sError: could not free block 4 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	if(DiskDriver_readBlock(my_disk, block_test, 3) != -1){
		fprintf(stderr,"Ok\n");
		return -1;
	}
	
	printf("\n\nFree block 5 (Expected: Ok)...");
	if(DiskDriver_freeBlock(my_disk, 4) == -1){
		fprintf(stderr, "%sError: could not free block 5 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	if(DiskDriver_readBlock(my_disk, block_test, 4) != -1){
		fprintf(stderr,"Ok\n");
		return -1;
	}
	
	printf("\n\nFree block 6 (Expected: Ok)...");
	if(DiskDriver_freeBlock(my_disk, 5) == -1){
		fprintf(stderr, "%sError: could not free block 6 to disk\n%s",KRED,KNRM);
		return -1;
	}
	printf("%sOk\n%s",KGRN,KNRM);
	if(DiskDriver_readBlock(my_disk, block_test, 5) != -1){
		fprintf(stderr,"Ok\n");
		return -1;
	}
	/*
	printf("\n\nFree block 7 (Expected: Error)...");
	if(DiskDriver_freeBlock(my_disk, 6) != -1){
		fprintf(stderr, "%sOk\n%s",KRED,KNRM);
		return -1;
	}
	*/
	/*=========================================================*/
	/*R. END*/
	printf("\n\n\n");
	
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
