#include <unistd.h> //access, close
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> //posix_fallocate
#include <sys/types.h> //Open
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h> //mmap
#include <errno.h>

#include "disk_driver.h"

//R. Funzione ausiliaria utilizzata per inizializzare il disk_header
DiskHeader* disk_header_init(int file_descriptor, int size, DiskHeader* disk_header){
	//R. Utilizzo mmap per ottenere i primi sizeof(DiskHeader)+bitmap_size bit in modo da costruire il disk header
	disk_header = (DiskHeader*) mmap(0, size, PROT_READ|PROT_WRITE,MAP_SHARED,file_descriptor,0);
    if(disk_header == MAP_FAILED){
		fprintf(stderr,"Error: mmap failed \n");
        return NULL;
    }
    return disk_header;
}

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){

	int fd; //R. Qui salvo il file descriptor

	int bitmap_size = (num_blocks/8)+1;

	DiskHeader* disk_header = NULL;

	if(disk == NULL || num_blocks < 1){ //R. Verifico che le condizioni iniziali vengano rispettate
		fprintf(stderr,"Error: disk is null or minimum blocks number less than 1. \n");
		return;
	}

	if(!access(filename,F_OK)){
		//R. Caso in cui il file esiste già
		
		//R. debug printf("file already exists, opening in progress");

        fd = open(filename,O_RDWR,(mode_t)0666); //R. ATTENZIONE, Attualmente di default 0666 per apertura

        if(fd == -1){
			fprintf(stderr,"Error: Unable to open file");
			exit(-1);
		}

		//R. Inizializzo disk_header
		disk_header = disk_header_init(fd, sizeof(DiskHeader)+bitmap_size,disk_header); 
        if(disk_header == NULL){
			close(fd);
            exit(-1);
		}

		disk->header = disk_header;

        //disk->header->first_free_block = 0;
        disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);

	}
	else{
		//R. Caso in cui il file non esiste e bisogna crearlo
		
		//R. debug printf("file does not exist, creation in progress\n");

        fd = open(filename, O_RDWR|O_CREAT|O_TRUNC,(mode_t)0666); //R. ATTENZIONE, Attualmente di default 0666 per apertura

        if(fd == -1){
			fprintf(stderr,"Error: Unable to open file");
			exit(-1);
		}

		//R. Attraverso la posix_fallocate vado ad indicare al S.O. che nella memoria virtuale deve riservare sizeof(DiskHeader)+bitmap_size bit al mio file fd
        if(posix_fallocate(fd,0,sizeof(DiskHeader)+bitmap_size) > 0){
        	fprintf(stderr,"Errore posix f-allocate");
        	close(fd);
        	exit(-1);
        }

        //R. Inizializzo disk_header
        disk_header = disk_header_init(fd, sizeof(DiskHeader)+bitmap_size,disk_header); 
        if(disk_header == NULL){
			close(fd);
            exit(-1);
		}

        //R. Vado ad arricchire disk e disk_header con tutte le informazioni iniziali per iniziare a scrivere sul disco
        disk->header = disk_header;

        disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);
        disk_header->num_blocks = num_blocks;
        disk_header->bitmap_blocks = num_blocks;
        disk_header->bitmap_entries = bitmap_size; 
        disk_header->free_blocks = num_blocks;
        disk_header->first_free_block = 0;
        memset(disk->bitmap_data,0, bitmap_size); //R. Utilizzo memset per settare a tutti 0 i dati all'interno della bitmap

	}

	disk->fd = fd; //R. Mi vado a salvare il file descriptor
	return;
}

// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num, int bytes_to_read){
    if(block_num >= disk->header->bitmap_blocks || block_num < 0 || dest == NULL || disk == NULL ){
		//fprintf(stderr,"Error: could not start with read block. Bad parameters \n");
        return -1;
	}
	
    //R. Inizializzo la mia bitmap
    BitMap bitmap;
    bitmap.num_bits = disk->header->bitmap_blocks;
    bitmap.entries = disk->bitmap_data;

    //R. Qui devo andare a verificare che il blocco a cui voglio andare ad accedere non sia già scritto
    if(!BitMap_is_free_block(&bitmap, block_num)){ 
		//fprintf(stderr,"Error: Could't read a free block"); //R. Only Debug
        return -1;
    }
    
    /*Versione con memcpy non funzionante
    //R. Vado a calcolare la posizione in cui devo iniziare la lettura del blocco nel disco
    int position = sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE);
    //R. Posiziono il puntatore
    void* readPosition = disk + position;
    //R. Sfrutto memcpy per copiare la memoria del blocco che voglio leggere all'interno di dest
    memcpy(dest, readPosition, BLOCK_SIZE);
	*/
	
	//R. Estraggo il FileDescriptor
    int fd = disk->fd;
    
    //R. Vado a spostare il FileDescriptor nella posizione iniziale in cui devo iniziare a scrivere
	off_t offset = lseek(fd, sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);	
	if(offset == -1){
		fprintf(stderr,"Error: lseek");
		return -1;
	}
		
	//R. Classica funzione di scrittura con il file descriptor
	int ret, read_bytes = 0;
	while(read_bytes < bytes_to_read){
		//printf("read bytes: %d\n",read_bytes);
		//printf("bytes_to_read: %d\n",bytes_to_read);
		//if(read_bytes == 200 && bytes_to_read == 512) return -1;																	
		if((ret = read(fd, dest + read_bytes, bytes_to_read - read_bytes)) == -1){
			if(errno == EINTR) 
				continue;
			else{
				fprintf(stderr,"Error: read\n");
				return -1;
			}
		}
			
		read_bytes += ret;						
	}

    return 0;
}

// writes a block in position block_num, returns -1 if operation not possible
int DiskDriver_updateBlock(DiskDriver* disk, void* src, int block_num, int bytes_to_write){
	 if(block_num >= disk->header->bitmap_blocks || block_num < 0 || src == NULL || disk == NULL ){
		fprintf(stderr,"Error: could not start with write block. Bad parameters \n");
        return -1;
	}
	
	//R. Vado ad aggiornare al disco i blocchi libero
    /*if(block_num == disk->header->first_free_block)																
	    disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num+1);
	   
	disk->header->free_blocks -=1;*/
	
    /* Versione con memcpy non funziona
    //R. Vado a calcolare la posizione in cui devo iniziare la scrittura del blocco nel disco
    int position = sizeof(DiskHeader)+(disk->header->bitmap_entries)+(block_num*BLOCK_SIZE);
    //R. Posiziono il puntatore alla posizione in cui devo iniziare la scrittura del blocco nel disco
    void* writePosition = disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE);
    //R. Sfrutto memcpy per copiare la memoria del blocco che voglio scrivere all'interno del blocco del disco
    memcpy(writePosition, src, BLOCK_SIZE);
    */
    
    //R. Estraggo il FileDescriptor
    int fd = disk->fd;
    
    //R. Vado a spostare il FileDescriptor nella posizione iniziale in cui devo iniziare a scrivere
	off_t offset = lseek(fd, sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);	
	if(offset == -1){
		fprintf(stderr,"Error: lseek");
		return -1;
	}
		
	//R. Classica funzione di scrittura con il file descriptor
	int ret, written_bytes = 0;
	while(written_bytes < bytes_to_write){																		
		if((ret = write(fd, src + written_bytes, bytes_to_write - written_bytes)) == -1){
			if(errno == EINTR) 
				continue;
			else{
				fprintf(stderr,"Error: write\n");
				return -1;
			}
		}
			
		written_bytes += ret;						
	}

    return 0;
}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num, int bytes_to_write){
	 if(block_num >= disk->header->bitmap_blocks || block_num < 0 || src == NULL || disk == NULL ){
		fprintf(stderr,"Error: could not start with write block. Bad parameters \n");
        return -1;
	}
	
    //R. Inizializzo la mia bitmap
    BitMap bitmap;
    bitmap.num_bits = disk->header->bitmap_blocks;
    bitmap.entries = disk->bitmap_data;

    //R. Qui devo andare a verificare che il blocco a cui voglio andare a scrivere sia libero
    //printf("block_num:%i\n",block_num);
    if(BitMap_is_free_block(&bitmap, block_num)){ 
		fprintf(stderr,"Error: Could't write a full block\n");
        return -1;
    }
	    
	//R. comunico alla bitmap che il blocco è stato occupato e diminuisco in header i free_blocks di 1    
	if(BitMap_set(&bitmap, block_num, 1) < 0){																	
		fprintf(stderr, "Error: could not set bit on bitmap\n");
		return -1;
	}
	
	//R. Vado ad aggiornare al disco i blocchi libero
    if(block_num == disk->header->first_free_block)																
	    disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num+1);
	   
	disk->header->free_blocks -=1;
	
    /* Versione con memcpy non funziona
    //R. Vado a calcolare la posizione in cui devo iniziare la scrittura del blocco nel disco
    int position = sizeof(DiskHeader)+(disk->header->bitmap_entries)+(block_num*BLOCK_SIZE);
    //R. Posiziono il puntatore alla posizione in cui devo iniziare la scrittura del blocco nel disco
    void* writePosition = disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE);
    //R. Sfrutto memcpy per copiare la memoria del blocco che voglio scrivere all'interno del blocco del disco
    memcpy(writePosition, src, BLOCK_SIZE);
    */
    
    //R. Estraggo il FileDescriptor
    int fd = disk->fd;
    
    //R. Vado a spostare il FileDescriptor nella posizione iniziale in cui devo iniziare a scrivere
	off_t offset = lseek(fd, sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);	
	if(offset == -1){
		fprintf(stderr,"Error: lseek");
		return -1;
	}
		
	//R. Classica funzione di scrittura con il file descriptor
	int ret, written_bytes = 0;
	while(written_bytes < bytes_to_write){																		
		if((ret = write(fd, src + written_bytes, bytes_to_write - written_bytes)) == -1){
			if(errno == EINTR) 
				continue;
			else{
				fprintf(stderr,"Error: write\n");
				return -1;
			}
		}
			
		written_bytes += ret;						
	}

    return 0;
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
	if(block_num >= disk->header->bitmap_blocks || block_num < 0 || disk == NULL ){
		fprintf(stderr,"Error: could not start with free block. Bad parameters \n");
        return -1;
	}
	
	//R. Inizializzo la mia bitmap
    BitMap bitmap;
    bitmap.num_bits = disk->header->bitmap_blocks;
    bitmap.entries = disk->bitmap_data;

    //R. Qui devo andare a verificare che il blocco a cui voglio andare a liberare sia libero
    if(!BitMap_is_free_block(&bitmap, block_num)){ 
		fprintf(stderr,"Error: Could't free a free block\n");
        return -1;
    }
    
    //R. Vado a settare la bitmap come blocco libero
    if(BitMap_set(&bitmap, block_num, 0) < 0){																	
		fprintf(stderr,"Error: could not set bit on bitmap\n \n");
		return -1;
	}
	
	//R. Vado ad aggiornare di nuovo il first free block e il contatore dei free_block
	if(block_num < disk->header->first_free_block || disk->header->first_free_block == -1){														
	    disk->header->first_free_block = block_num;		
	}														
	    
	disk->header->free_blocks += 1;	
	
	//R. Operazioni successive non strettamente necessarie. Se setto a 0 la bitmap posso anche lasciare
	//   Sporca la memoria, tanto successivamente viene sovrascritta
	
	/* Operazione con memcpy non funzionante
	//R. Vado a calcolare la posizione in cui devo iniziare la scrittura del blocco nel disco
    int position = sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE);
	//R. Posiziono il puntatore
    void* writePosition = disk + position;
    //R. Sfrutto memcpy per copiare la memoria del blocco che voglio scrivere all'interno del blocco del disco
    memset(writePosition, 0, BLOCK_SIZE);
    */
    
    //R. Estraggo il FileDescriptor
    int fd = disk->fd;
    
    //R. Vado a spostare il FileDescriptor nella posizione iniziale in cui devo iniziare a scrivere
	off_t offset = lseek(fd, sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);	
	if(offset == -1){
		fprintf(stderr,"Error: lseek");
		return -1;
	}
	
	//R. Setto tutti i bit a 0
	char buffer[BLOCK_SIZE] = {0};
		
	//R. Classica funzione di scrittura con il file descriptor
	int ret, written_bytes = 0;
	while(written_bytes < BLOCK_SIZE){																		
		if((ret = write(fd, buffer + written_bytes, BLOCK_SIZE - written_bytes)) == -1){
			if(errno == EINTR) 
				continue;
			else{
				fprintf(stderr,"Error: write\n");
				return -1;
			}
		}
			
		written_bytes += ret;						
	}
	
	return 0;
}

// returns the first free blockin the disk from position (checking the bitmap)
//R. La funzione restituisce -1 in caso di errore, altrimenti la posizione del disco
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
	//R. Verifico che venga passato un disco corretto e che la posizione desiderata non sia superiore al numero di blocchi del disco.
	if(disk == NULL || start > disk->header->num_blocks)
        return -1;
    
    BitMap bitmap;
    
    bitmap.num_bits = disk->header->bitmap_blocks;
    bitmap.entries = disk->bitmap_data;
    
    int position_free_block = BitMap_get(&bitmap, start, 0);

    return position_free_block;
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){
	
	int bitmap_size = (disk->header->num_blocks)/8+1;
	
	//R. nel momento in cui abbiamo effettuato delle operazioni sul disco dobbiamo andare ad effettuare
	// un flush sulla mmap. Senza l'utilizzo di msync non abbiamo nessuna garanzia che le operazioni 
	// di scrittura vengano correttamente registrate sul nostro file system
	
	if(msync(disk->header, (size_t)sizeof(DiskHeader)+bitmap_size, MS_SYNC) == -1){
			fprintf(stderr,"Error: Could not sync the file to disk\n");
			return -1;
	}								 

	return 0;
}

//R. print many informations about disk status
void DiskDriver_print_information(DiskDriver* disk,const char* filename){
    
    printf("======== MY DISK INFORMATION ========\n");
    printf("Filename: %s\n",filename);
    printf("Number blocks: %d\n", disk->header->num_blocks);
    printf("Free blocks: %d\n", disk->header->free_blocks);
    printf("First free block: %d\n\n", disk->header->first_free_block);
    printf("Used space: %d\n", ((disk->header->num_blocks)-(disk->header->free_blocks))*BLOCK_SIZE);
    printf("Free space: %d\n", (disk->header->free_blocks * BLOCK_SIZE));
    printf("=====================================\n");
    return;
}
