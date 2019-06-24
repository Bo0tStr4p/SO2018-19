#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//R. Funzione per creare il primo blocco file
FileBlock* create_first_file_block(char value, int index_block){
	int i;
	FileBlock* file_block = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block == NULL){
		fprintf(stderr,"Error: malloc create_file_block\n");
		return NULL;
	}
	//R. Valori casuali
	file_block->index_block = index_block;
	file_block->position = 0;
	
	//R. Scrivo dei valori per riempire i blocchi
	char data_block[BLOCK_SIZE - sizeof(int) - sizeof(int)];
	for(i = 0; i < (BLOCK_SIZE - sizeof(int) - sizeof(int)); i++) data_block[i] = value;
	data_block[BLOCK_SIZE - sizeof(int) - sizeof(int)-1] = '\0';
	strcpy(file_block->data, data_block);
	return file_block;
}

int main(int argc, char** argv){
	int free_block;
	int first_free_block_position;
	
	printf("Start: \n");
	
	const char* filename = "./index_test.txt";
	
	//R. Inizializzo la memoria del mio disco
	DiskDriver* my_disk = (DiskDriver*)malloc(sizeof(DiskDriver));
	if(my_disk == NULL){
		fprintf(stderr,"Error: malloc of DiskDriver\n");
		return -1;
	}
	
	printf("Initialization of disk driver with 50 blocks \n");
	DiskDriver_init(my_disk,filename,50); 
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	DiskDriver_print_information(my_disk,filename);
	
	//R. Creo il first file block e aggiungo il blocco index
	printf("\n\n");
	printf("Creo il FirstFileBlock con un blocco index annesso...\n");
	
	BlockIndex index_file = create_block_index(-1);	
	
	//R. Riempio con valori casuali. Attualmente non sono necessari
	FileControlBlock file_control_block = {
		.directory_block = -1,
		.block_in_disk = -1,
		.name = "test",
		.written_bytes = -1,
		.size_in_bytes = -1,
		.size_in_blocks = -1,
		.is_dir = -1,
	};
	
	FirstFileBlock ffb = {
		.index = index_file,
		.fcb = file_control_block,
	};
	
	//R. Scrivo il FirstFileBlock sul disco
	
	free_block = DiskDriver_getFreeBlock(my_disk, 0);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}
	
	first_free_block_position = free_block; //R. Salvo la posizione per recuperarlo successivamente

	if(DiskDriver_writeBlock(my_disk, &ffb, free_block, sizeof(FirstFileBlock)) == -1){
		fprintf(stderr, "Error: could not write block 2 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	
	printf("\nInizio l'operazione di scrittura su 15 file block\n");
	
	//R. Il primo file block va creato manualmente
	
	FileBlock* block1 = create_first_file_block('1',free_block);
	
	printf("Scrivo il blocco 1\n");
	
	//R. Scrivo il primo blocco sul disco
	
	free_block = DiskDriver_getFreeBlock(my_disk, free_block);
	if(free_block == -1){
		fprintf(stderr, "Error: getFreeBlock\n");
		return -1;
	}

	if(DiskDriver_writeBlock(my_disk, block1, free_block, sizeof(FileBlock)) == -1){
		fprintf(stderr, "Error: could not write block 2 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	
	free(block1);
	
	//R. Aggiorno il primo blocco index
	FirstFileBlock* ffb_read = (FirstFileBlock*)malloc(sizeof(FirstFileBlock));
	
	if(DiskDriver_readBlock(my_disk, (void *)ffb_read, first_free_block_position, sizeof(FirstFileBlock)) == -1){
		fprintf(stderr, "Error: could not write block 2 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	
	ffb_read->index.blocks[0] = free_block;
	
	if(DiskDriver_updateBlock(my_disk, ffb_read, 0, sizeof(FirstFileBlock)) == -1){
		fprintf(stderr, "Error: could not write block 2 to disk\n");
		return -1;
	}
	if(DiskDriver_flush(my_disk) == -1){	
		fprintf(stderr, "Error: flush\n");
		return -1;
	}
	
	printf("ffb position: %d\n",first_free_block_position);
	print_index_block(&ffb_read->index);
	
	//R. Creo gli altri 14 blocchi con la forma automatizzata
	
	int i,j, block_position;
	FileBlock* current = block1;
	
	printf("\n Son qui\n");
	
	FileBlock* file_block_tmp = (FileBlock*)malloc(sizeof(FileBlock));
	
	for(i=1;i<15;i++){
		
		printf("Scrivo il blocco: %d\n",i+1);
		
		
		block_position = create_next_file_block(current, file_block_tmp, my_disk);
		free(current);
		
		//R. Scrivo dei valori per riempire i blocchi
		char data_block[BLOCK_SIZE - sizeof(int) - sizeof(int)];
		for(j = 0; j < (BLOCK_SIZE - sizeof(int) - sizeof(int)); j++) data_block[j] = 49 + (char)i;
		data_block[BLOCK_SIZE - sizeof(int) - sizeof(int)-1] = '\0';
		strcpy(file_block_tmp->data, data_block);
		
		
		if(DiskDriver_writeBlock(my_disk, file_block_tmp, block_position, sizeof(FileBlock)) == -1){
			fprintf(stderr, "Error: could not write block %d to disk\n", i+1);
			return -1;
		}
		
			
		if(DiskDriver_flush(my_disk) == -1){
			fprintf(stderr, "Error: flush\n");
			return -1;
		}
		
		current = file_block_tmp;
		
	}
	
	printf("\n\n Inizio la lettura\n\n");
	
	//R. Estraggo il ffb
	
	if(DiskDriver_readBlock(my_disk, ffb_read, first_free_block_position, sizeof(FirstFileBlock)) == -1){
		fprintf(stderr, "Error: could not read block 1 to disk\n");
		return -1;
	}
	
	
	//R. Estraggo il primo FileBlock Manualmente
	
	FileBlock* read_block = (FileBlock*)malloc(sizeof(FileBlock));
	if(read_block == NULL){
		fprintf(stderr,"Errore nella creazione del file block");
		return -1;
	}
	
	printf("Leggo il blocco 1:\n");
	
	if(DiskDriver_readBlock(my_disk, read_block, ffb_read->index.blocks[0], sizeof(FileBlock)) == -1){
		fprintf(stderr, "Error: could not read block 1 to disk\n");
		return -1;
	}
	printf("DATA:\n");
	printf("%s\n\n", read_block->data);
	
	current = read_block;
	
	for(i=1;i<15;i++){
		printf("Leggo il blocco: %d\n",i+1);
		
		file_block_tmp = get_next_block_file(current,my_disk);
		free(current);
		printf("DATA:\n");
		printf("%s\n\n", file_block_tmp->data);
		
		current = file_block_tmp;
		
	}
	
	free(file_block_tmp);
	free(ffb_read);
	
	//Continuare qui con la directory test
	
	
	free(my_disk);
	
	return 0;
}
