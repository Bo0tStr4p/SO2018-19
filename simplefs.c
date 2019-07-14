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
	
	fs->disk = disk;
	FirstDirectoryBlock* first_directory_block = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
	if(first_directory_block == NULL){
		fprintf(stderr, "Error in SimpleFS_init: could not create first directory block.\n");
		return NULL;
	}	
	
	int res = DiskDriver_readBlock(disk,first_directory_block,0, sizeof(FirstDirectoryBlock));
	if(res == -1){ 										//A. controllo che il blocco sia disponibile. Se non è disponibile, non possiamo andare avanti
		free(first_directory_block);
		return NULL;
	};				
	
	DirectoryHandle* directory_handle = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));		//A. Il blocco è disponibile, quindi posso allocare la struttura
	if(directory_handle == NULL){
		fprintf(stderr, "Error in SImpleFS_init: could not create directory handle.\n");
		return NULL;
	}
	
	directory_handle->sfs = fs;
	directory_handle->dcb = first_directory_block;
	directory_handle->parent_dir = NULL;
	//directory_handle->current_block = NULL;
	//directory_handle->pos_in_dir = 0;
	directory_handle->pos_in_block = 0;
	
	return directory_handle;
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
	if(fs == NULL){
		fprintf(stderr, "Error in SimpleFS_format: could not format, bad parameters.\n");
		return;
	}
	
	//A. Creiamo le strutture iniziali
	FirstDirectoryBlock root_directory = {0}; 				//A. Top Level Directory
	root_directory.index.previous = -1;						// Si inserisce il predecessore 
	root_directory.index.next = -1;							// Essendo appena inizializzato, ancora non c'è un successivo blocco index, quello corrente non è pieno
	
	int i;
	for(i=0; i<MAX_BLOCKS; i++){
		root_directory.index.blocks[i] = -1;				// Tutti i blocchi vuoti settati a -1
	}
	
	root_directory.fcb.directory_block = -1; 				//A. root non ha genitori
	root_directory.fcb.block_in_disk = 0;					//A. root ha il primo blocco nel disco
	root_directory.fcb.is_dir = 1; 							//A. è una directory
	strcpy(root_directory.fcb.name, "/"); 					//A. il suo nome è "/"
	
	root_directory.num_entries = 0;
	
	//A. puliamo la bitmap dei blocchi occupati nel disco
	fs->disk->header->free_blocks = fs->disk->header->num_blocks;	//A. il numero dei blocchi liberi è uguale a quello dei blocchi totali sul disco. Cioè il disco è vuoto
	fs->disk->header->first_free_block = 0;							//A. il primo blocco libero ovviamento è quello 0
	int bitmap_size = fs->disk->header->bitmap_entries; 			//A. numero di blocchi della bitmap
	memset(fs->disk->bitmap_data,'\0', bitmap_size); 				//A. mappo con 0 (blocco libero) l'array bitmap_data
	
	DirectoryBlock dir_block = {
		.index_block = 0, 
		.position = 0
	};
	
	for(i=0;i<((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));i++)
			dir_block.file_blocks[i] = 0;
	
	int free_block = DiskDriver_getFreeBlock(fs->disk, 1);
	if(free_block == -1){
		fprintf(stderr, "Error in SimpleFS_format: getFreeBlock\n");
		return;
	}
	
	root_directory.index.blocks[0] = free_block;
	
	if(DiskDriver_writeBlock(fs->disk, &dir_block, free_block, sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_format: get free block.\n");
		return;
	}
	
	int ret = DiskDriver_writeBlock(fs->disk, &root_directory, 0, sizeof(FirstDirectoryBlock));		//A. vado a scrivere sul disco la root directory
	if (ret == -1){
		fprintf(stderr, "Errore in SimpleFS_format: impossibile formattare\n");
		//free(root_directory);
		return;
	}
	return;
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename){
	if(d == NULL || filename == NULL){ 
		fprintf(stderr,"Errore in SimpleFS_createFile: inseriti prametri non corretti\n");
		return NULL;
	}
	
	//A. Innanzitutto devo controllare che il file non sia già presente sul disco. Prima ci sono dei controlli.
	SimpleFS* fs = d->sfs;
	DiskDriver* disk = fs->disk;                   
	FirstDirectoryBlock* fdb = d->dcb;
	//DirectoryBlock* db = d->current_block; //R. Non serve
	if(fs == NULL || disk == NULL || fdb == NULL){ 
		fprintf(stderr,"Errore in SimpleFS_createFile: la DirectoryHandle non è allocata bene\n");
		return NULL;
	}
	
	//A. Controllo.
	int ret,i;
	ret = SimpleFS_already_exists_file(disk,fdb,(char*)filename);
	if(ret == -1){
		fprintf(stderr, "Errore in SimpleFS_createFile: SimpleFS_already_exists_file restituisce errore\n");
		return NULL;
	}
	if(ret != -2){
		//fprintf(stderr, "Errore in SimpleFS_createFile: il file che stai creando é già presente sul disco\n");
		return NULL;
	} 
	
	//A. Il file non esiste, possiamo crearlo da 0. Prendiamo dal disco il primo blocco libero
	int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
	if(new_block == -1){
		fprintf(stderr, "Errore nella createFile: non ci sono blocchi liberi sul disco\n");
		return NULL;	//A. se non ci sono blocchi liberi sul disco interrompiamo
	}
	
	//A. creiamo il primo blocco del file
	FirstFileBlock* file_to_create = malloc(sizeof(FirstFileBlock));  
	if(file_to_create == NULL){
		fprintf(stderr,"Error: malloc on file_to_create.\n");
		return NULL;
	} 
	file_to_create->index.previous = -1;
	file_to_create->index.next = -1;

	for(i=0; i<MAX_BLOCKS; i++){
		file_to_create->index.blocks[i] = -1;				//R. Tutti i blocchi vuoti settati a -1
	}
	
	file_to_create->fcb.directory_block = fdb->fcb.block_in_disk;
	file_to_create->fcb.block_in_disk = new_block; 					//A. gli assegno il blocco libero sul disco ottenuto dalla getFreeBlock
	strncpy(file_to_create->fcb.name,filename,128);
	file_to_create->fcb.written_bytes = 0;
	file_to_create->fcb.size_in_bytes = BLOCK_SIZE;					//A. necessario inizializzarlo altrimenti valgrind da errore nella write
	file_to_create->fcb.size_in_blocks = 1;							//A. necessario inizializzarlo altrimenti valgrind da errore nella write
	file_to_create->fcb.is_dir = 0;
	
	int free_block = DiskDriver_getFreeBlock(disk,new_block + 1);
	if(free_block == -1){
		fprintf(stderr,"Error: get freeblock.\n");
		free(file_to_create);
		return NULL;
	}
	
	file_to_create->index.blocks[0] = free_block;
	
	FileBlock* file = (FileBlock*)malloc(sizeof(FileBlock));
	if(file == NULL){
		fprintf(stderr,"Error: malloc on file.\n");
		free(file_to_create);
		return NULL;
	}
	
	file->index_block = new_block;
	file->position = 0;
	int len = BLOCK_SIZE - sizeof(int) - sizeof(int);
	for(i=0; i<len; i++){
		file->data[i] = -1;				//A. Necessario inizializzare altrimenti valgrind da errore nella write
	}
	
	//A. Scriviamo su disco il file
	ret = DiskDriver_writeBlock(disk, file_to_create ,new_block, sizeof(FirstFileBlock));
	if(ret == -1){
		fprintf(stderr, "Errore in SimpleFS_createFile: impossibile scrivere sul disco");
		free(file_to_create);
		free(file);
		return NULL;
	}
	
	ret = DiskDriver_writeBlock(disk, file , free_block, sizeof(FirstFileBlock));
	if(ret == -1){
		fprintf(stderr, "Errore in SimpleFS_createFile: impossibile scrivere sul disco");
		free(file_to_create);
		free(file);
		return NULL;
	}
	
	//A. Dobbiamo mettere il file in un blocco directory.
	ret = SimpleFS_assignDirectory(disk, fdb , new_block, free_block);
	if(ret == -1){
		fprintf(stderr, "Errore in SimpleFS_createFile: impossibile assegnare spazio per il file in una directory");
		free(file_to_create);
		free(file);
		return NULL;
	}
	
	FileHandle* file_handle = malloc(sizeof(FileHandle));
	file_handle->sfs = d->sfs;
	file_handle->fcb = file_to_create;
	file_handle->directory = fdb;
	//file_handle->current_block = NULL;							//A. prima ci assegnavo un FileBlock, da rivedere anche qua
	file_handle->pos_in_file = 0;
	
	free(file);
	return file_handle;
}

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names,int* flag, DirectoryHandle* d){
	if(names == NULL || flag == NULL || d == NULL){
		fprintf(stderr, "Errore in SimpleFS_readDir: parametri inseriti non corretti\n");
		return -1;
	}
	
	DiskDriver* disk = d->sfs->disk;
	FirstDirectoryBlock *fdb = d->dcb;
	//DirectoryBlock* db = d->current_block; //R. Non serve
	
	//A. Se la directory è vuota, inutile procedere
	if(fdb->num_entries <= 0){
		fprintf(stderr, "Errore in SimpleFS_readDir: directory vuota\n");
		return -1;
	}
	
	int i, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int), dim_names=0;
	FirstFileBlock ffb_to_check; 
	
	//R. Estraggo il primo DirectoryBlock
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error in SimpleFS_already_exists_file: malloc on db in SimpleFS_already_exists");
		return -1;
	}
	
	if(DiskDriver_readBlock(disk,db,d->dcb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_already_exists_file: could not read directory block one.\n");
		free(db);
		return -1;
	}
	
	//A. Iniziamo a leggere i file contenuti nel blocco directory in cui ci troviamo, cioè d->current_block
	for (i=0; i<dim; i++){	
		if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &ffb_to_check, db->file_blocks[i], sizeof(FirstFileBlock)) != -1){ 
			names[dim_names] = strndup(ffb_to_check.fcb.name, 128); 	//A. Salvo il nome del file che sto leggendo nell'array names
			flag[i] = ffb_to_check.fcb.is_dir;							//R. Salvo se è file o directory
            dim_names++;
		}
	}
	
	//A. Caso in cui ci sono file non contenuti nello stesso blocco directory e quindi bisogna cambiare blocco
	if (fdb->num_entries > i){	
		
		db = get_next_block_directory(db,disk);

		while (db != NULL){	 

			for (i=0; i<dim; i++){	 
				if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &ffb_to_check, db->file_blocks[i], sizeof(FirstFileBlock)) != -1){ 
					names[dim_names] = strndup(ffb_to_check.fcb.name, 128); 											//A. Salvo il nome del file che sto leggendo nell'array names
					flag[i] = ffb_to_check.fcb.is_dir;							//R. Salvo se è file o directory
                    dim_names++;
				}
			}
			db = get_next_block_directory(db,disk);
		}
	}
	free(db);
	return 0;
}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
	//R. Verifico le condizioni iniziali
	if (d == NULL || filename == NULL){
		fprintf(stderr, "Error:could not open file: Bad Parameters\n");
        return NULL;
    }

	int i;
	FirstDirectoryBlock *first_directory_block = d->dcb; //R. Estraggo il file directory block
	DiskDriver* disk = d->sfs->disk; //R. Estraggo il disco
    int space_directory_block = ((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int)); //R. Lo spazio massimo che possiamo avere nei file blocks 

	//R. Caso in cui la directory non è vuota
	if (first_directory_block->num_entries > 0){
		//R. Creo il file handle di riferimento
		FileHandle* file_handle = (FileHandle*)malloc(sizeof(FileHandle));
		if(file_handle == NULL){
			fprintf(stderr,"Error: could not create file handle.\n");
			return NULL;
		}
		file_handle->sfs = d->sfs;
		file_handle->directory = first_directory_block;
		file_handle->pos_in_file = 0;

		//R. Utilizzo la variabile d'appoggio is_found per verificare se ho trovato o meno il file
		int is_found = 0;
		FirstFileBlock* to_check = (FirstFileBlock*)malloc(sizeof(FirstFileBlock));
		if(to_check == NULL){
			fprintf(stderr,"Error: could not create first file block.\n");
			free(file_handle);
			return NULL;
		}

		//R. Fino a qui
		
		//R. Verifico l'esistenza del file nei Directory Block
		
		//R. Estraggo il primo Directory Block
		DirectoryBlock* dir_block = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
		if(dir_block == NULL){
			fprintf(stderr,"Error: malloc dir_block in open_file.\n");
			free(file_handle);
			free(to_check);
			return NULL;
		}
		
		if(DiskDriver_readBlock(disk, (void*)dir_block, first_directory_block->index.blocks[0], sizeof(DirectoryBlock)) == -1){
			fprintf(stderr,"Error: read file block 1 in open file.\n");
			free(file_handle);
			free(to_check);
			free(dir_block);
			return NULL;
		}
		
		//R. Cerco il file
		while (dir_block != NULL && !is_found){	

            for(i = 0; i < space_directory_block; i++){
                if(dir_block->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,to_check,dir_block->file_blocks[i],sizeof(FirstFileBlock)) != -1)){ //R. Controllo che il blocco letto abbia del contenuto
                    if(strncmp(to_check->fcb.name,filename,128) == 0){
                        is_found = 1;
        				file_handle->fcb = to_check;
                        break;
                    }
                }
            }

			//R. Estraggo il Directory Block Successivo
			dir_block = get_next_block_directory(dir_block, disk);
		}
		
		//R. Verifico se il file è stato trovato e restituisco il suo file handle
		if (is_found){
			free(dir_block);
			return file_handle;
		} else {
			fprintf(stderr,"Error, could not open file: file doesn't exist.\n");
			free(file_handle);
			free(to_check);
			free(dir_block);
			return NULL;
		}

	//R. Caso in cui la directory è vuota
	} else { 
		fprintf(stderr,"Error:could not open file: directory is empty.\n");
		return NULL;
	}
    return NULL;
}


// closes a file handle (destroyes it)
int SimpleFS_close_file(FileHandle* f){
	free(f->fcb);
	//free(f->current_block);
	free(f);
	return 0;
}

int SimpleFS_close_directory(DirectoryHandle* f){
	free(f->dcb);
	if(f->parent_dir != NULL)
		free(f->parent_dir);
	free(f);
	return 0;
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written

// R. ATTENZIONE -> IN FASE DI TEST BISOGNA CONTROLLARE SE TUTTE LE FREE SONO STATE INSERITE CORRETTAMENTE

int SimpleFS_write(FileHandle* f, void* data, int size){
	FirstFileBlock* ffb = f->fcb;
	
	int written_bytes = 0,i;
	int to_write = size; //R. Bytes da scrivere
	
	int off = f->pos_in_file;
	int space_file_block = BLOCK_SIZE - sizeof(int) - sizeof(int); //R. Spazio per ogni File Block
	
	FirstFileBlock* first_file = f->fcb; //R. Estraggo il FirstFileBlock
	DiskDriver* my_disk = f->sfs->disk; //R. Estraggo il disco
	
	FileBlock* file_block_tmp = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block_tmp == NULL){
		fprintf(stderr,"Error: malloc of file_block_tmp in SimpleFS_Write.\n");
		return -1;
	}
	
	//R. Calcolo il blocco al quale devo accedere
	int index_block_ref = off/(10*space_file_block);
	int file_index_pos = (off - index_block_ref*space_file_block*10)/space_file_block;
		
	BlockIndex index = first_file->index;
	
	//R. Calcolo il nuovo offset corretto
	off = off - index_block_ref*space_file_block*10;
		
	//R. mi posiziono al blocco index di riferimento
	for(i=0;i<index_block_ref;i++){
		if(DiskDriver_readBlock(my_disk, (void*)&index, index.next, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error, next block in SimpleFs_read.\n");
			return -1;
		}
	}
	
	//R. Caso in cui posso scrivere direttamente nel primo blocco
	if(off < space_file_block){
		//R. Estraggo il File Block
		if(DiskDriver_readBlock(my_disk,(void*) file_block_tmp, index.blocks[file_index_pos],sizeof(FileBlock)) == -1){
			fprintf(stderr,"Error: could not read file block 1.\n");
			free(file_block_tmp);
			return -1;
		}
		
		//R. Caso in cui basta solo questo blocco per scrivere il contenuto
		if(to_write <= space_file_block-off){
			memcpy(file_block_tmp->data+off, (char*)data, to_write);											
			written_bytes += to_write;
			if(f->pos_in_file+written_bytes > ffb->fcb.written_bytes) //R. Aggiorno written_bytes
				ffb->fcb.written_bytes = f->pos_in_file+written_bytes;
			if(DiskDriver_updateBlock(my_disk,(void*)file_block_tmp, index.blocks[file_index_pos],sizeof(FileBlock)) == -1){ //R. Aggiorno file block su disco
				fprintf(stderr,"Error:could not update file block 1.\n");
				free(file_block_tmp);
				return -1;
			}
			if(DiskDriver_updateBlock(my_disk, ffb, ffb->fcb.block_in_disk,sizeof(FirstFileBlock)) == -1){ //R. Aggiorno il first file block
				fprintf(stderr,"Error:could not update ffb.\n");
				free(file_block_tmp);
				return -1;
			}
			free(file_block_tmp);
			return written_bytes;
		}
		//R. Caso in cui devo scrivere nel primo blocco e continuare
		else{
			memcpy(file_block_tmp->data+off, (char*)data, space_file_block-off);										
			written_bytes += space_file_block-off;
			to_write = size - written_bytes;
			if(DiskDriver_updateBlock(my_disk,(void*)file_block_tmp, index.blocks[file_index_pos],sizeof(FileBlock)) == -1){ //R. Aggiorno file block su disco
				fprintf(stderr,"Error:could not update file block 1.\n");
				free(file_block_tmp);
				return -1;
			}
		}
	}
	else{
		free(file_block_tmp);
		return -1;
	}
	
	FileBlock* current = file_block_tmp;
	int block_position;	
		
	//R. Vado a scrivere nei successivi blocchi, se necessario
	while(written_bytes < size && file_block_tmp != NULL){		
		
		//R. Creo il blocco successivo per scrivere le informazioni
		block_position = create_next_file_block(current, file_block_tmp, my_disk);
		if(block_position == -1){
			fprintf(stderr,"Error: could not create next file block.\n");
			free(file_block_tmp);
			return -1;
		}
				
		//R. Caso in cui posso scrivere tutto sul prossimo blocco
		if(to_write <= space_file_block-off){											
			memcpy(file_block_tmp->data, (char*)data + written_bytes, to_write);						
			written_bytes += to_write;															
			if(f->pos_in_file+written_bytes > ffb->fcb.written_bytes) //R. Aggiorno written_bytes
				ffb->fcb.written_bytes = f->pos_in_file+written_bytes;
			if(DiskDriver_updateBlock(my_disk, ffb, ffb->fcb.block_in_disk, sizeof(FirstFileBlock)) == -1){ //R. Aggiorno il first file block
				fprintf(stderr,"Error:could not update ffb.\n");
				free(file_block_tmp);
				return -1;
			}
			if(DiskDriver_writeBlock(my_disk, file_block_tmp, block_position, sizeof(FileBlock)) == -1){
				fprintf(stderr,"Error:could not write next file block on disk.\n");
				free(file_block_tmp);
				return -1;
			}
			free(file_block_tmp);
			return written_bytes;
		}
		//R. Caso in cui devo continuare a scrivere anche nel blocchi successivi
		else{										
			memcpy(file_block_tmp->data, (char*)data + written_bytes, space_file_block);					
			written_bytes += space_file_block;														
			to_write = size - written_bytes;
			if(DiskDriver_writeBlock(my_disk, file_block_tmp, block_position, sizeof(FileBlock)) == -1){
				fprintf(stderr,"Error:could not write next file block on disk.\n");
				free(file_block_tmp);
				return -1;
			}
		}
		
		current = file_block_tmp;
																												
	}

	free(file_block_tmp);
	return written_bytes;
}

// reads in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read

// R. ATTENZIONE -> IN FASE DI TEST BISOGNA CONTROLLARE SE TUTTE LE FREE SONO STATE INSERITE CORRETTAMENTE

int SimpleFS_read(FileHandle* f, void* data, int size){
	FirstFileBlock* first_file = f->fcb; //R. Estraggo il FirstFileBlock
	DiskDriver* my_disk = f->sfs->disk; //R. Estraggo il disco
	int i;
	
	int off = f->pos_in_file;															
	int written_bytes = first_file->fcb.written_bytes;													
	
	//R. Controllo che la parte da leggere non vada oltre ciò che si trova nel file
	if(size+off > written_bytes){																
		fprintf(stderr,"Could not read, size+off > written_bytes\n");
		return -1;
	}
	
	int bytes_read = 0;
	int to_read = size;
	int space_file_block = BLOCK_SIZE - sizeof(int) - sizeof(int);
	
	FileBlock* file_block_tmp = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block_tmp == NULL){
		fprintf(stderr,"Error: malloc file_block_tmp in SimpleFS_read\n");
		return -1;
	}
	
	//R. Calcolo il blocco al quale devo accedere
	int index_block_ref = off/(10*space_file_block);
	int file_index_pos = (off - index_block_ref*space_file_block*10)/space_file_block;
		
	BlockIndex index = first_file->index;
		
	//R. mi posiziono al blocco index di riferimento
	for(i=0;i<index_block_ref;i++){
		if(DiskDriver_readBlock(my_disk, (void*)&index, index.next, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error, next block in SimpleFs_read.\n");
			return -1;
		}
	}
		
	//R. Estraggo il FileBlock da cui partire
	if(DiskDriver_readBlock(my_disk, (void*)file_block_tmp, index.blocks[file_index_pos], sizeof(FileBlock)) == -1){
		fprintf(stderr,"Error: could not read file block in read.\n");
		return -1;
	}
	
	off = off - index_block_ref*space_file_block*10;
	
	//R. Caso in cui devo partire dal primo file block e non devo andare oltre
	if(off < space_file_block && to_read <= (space_file_block-off)){	
		memcpy(data, file_block_tmp->data + off, to_read);							
		bytes_read += to_read;
		to_read = size - bytes_read;
		f->pos_in_file += bytes_read;
		free(file_block_tmp);
		return bytes_read;
	}
	//R. Caso in cui devo partire dal primo file block ma devo andare anche oltre
	else if(off < space_file_block && to_read > (space_file_block-off)){
		memcpy(data, file_block_tmp->data + off, space_file_block-off);																	
		bytes_read += space_file_block-off;
		to_read = size - bytes_read;
	}
	//R. Caso default
	else{
		return -1;
	}
	//R. Continuo con la lettura in tutti i blocchi successivi
	
	while(bytes_read < size && file_block_tmp != NULL){
		//R. Estraggo il blocco successivo
		file_block_tmp =  get_next_block_file(file_block_tmp,my_disk);
		if(file_block_tmp == NULL){
			f->pos_in_file += bytes_read;
			free(file_block_tmp);
			return bytes_read;
		}
		//R. Caso in cui devo leggere il singolo blocco successivo
		else if(to_read <= space_file_block){											
			memcpy(data+bytes_read, file_block_tmp->data, to_read);													
			bytes_read += to_read;
			to_read = size - bytes_read;
			f->pos_in_file += bytes_read;
			free(file_block_tmp);
			return bytes_read;
		}
		//R. Caso in cui devo leggere tutti i blocchi successivi
		else{										
			memcpy(data+bytes_read, file_block_tmp->data, space_file_block);																	
			bytes_read += space_file_block;
			to_read = size - bytes_read;
		}
	}

	free(file_block_tmp);
	f->pos_in_file += bytes_read;
	return bytes_read;
}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){
	FirstFileBlock* first_file = f->fcb;
	//R. Stiamo provando a leggere uno spazio maggiore di quello disponibile
	if(pos > first_file->fcb.written_bytes){ 
		fprintf(stderr,"Error: simpleFS_seek\n");
		return -1;
	}
	f->pos_in_file = pos;
	return pos;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){
 	if(d == NULL || dirname == NULL){
		fprintf(stderr,"Errore in SimpleFS_changeDir: inseriti prametri non corretti\n");
		return -1;
	}
	
	//A. Confrontando il nome con ".." vedo se il comando inserito mi chiede di andare alla cartella genitore
	if(strcmp(dirname,"..") == 0){	
		if(d->dcb->fcb.block_in_disk == 0){ 												//A. Controllo se la directory in cui sto è la root
			fprintf(stderr, "Errore in SimpleFS_changeDir: mi trovo nella directory root\n");
			return -1;
		}
		
		int parent_block = d->parent_dir->fcb.directory_block;
		d->pos_in_block = 0;
		d->dcb = d->parent_dir;

		//A. Se -1, mi sto spostando nella root
		if(parent_block == -1){
			d->parent_dir = NULL;
			return 0;
		}
		
		//A. leggo dal disco il blocco della directory genitore. Se la lettura va a buon fine assegno al dir_handle la cartella genitore aggiornata
		int res;
		FirstDirectoryBlock* parent_directory = malloc(sizeof(FirstDirectoryBlock));
		res = DiskDriver_readBlock(d->sfs->disk, parent_directory, parent_block, sizeof(FirstDirectoryBlock));
		if(res == -1){
			fprintf(stderr, "Errore in SimpleFS_changeDir: fallita la lettura del blocco della directory genitore \n");
			d->parent_dir = NULL;
			return -1; 
		}
		else{
			d->parent_dir = parent_directory;
		}
		return 0;
	}
	
	//A. caso in cui la directory in cui sto dentro è vuota
	else if(d->dcb->num_entries < 0){ 
		fprintf(stderr, "Errore in SimpleFS_changeDir: la directory in cui sto è vuota\n");
		return -1;
	}
	else{
		//A. caso normale in cui mi sposto semplicemente in un' altra cartella
		FirstDirectoryBlock* fdb = d->dcb;
		DirectoryBlock* db = d->current_block;
		DiskDriver* disk = d->sfs->disk;
		
		//A. Controllo se la cartella in cui mi devo spostare è in questo blocco directory
		FirstDirectoryBlock* dir_dest = malloc(sizeof(FirstDirectoryBlock));
		int i,res;
		int dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
		
		for(i=0; i<dim; i++){
			if(db->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock))) != -1){
				if(strcmp(dir_dest->fcb.name,dirname) == 0){
					DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock));
					d->pos_in_block = 0; 
					d->parent_dir = fdb;
					d->dcb = dir_dest;
					return 0;
				}
			}
		}
		
		//A. Altrimenti continuo a cercare negli altri blocchi directory
		int pos_in_disk;
		DirectoryBlock* next_block = get_next_block_directory(db,disk);
		
		while(next_block != NULL){
			pos_in_disk = get_position_disk_directory_block(next_block,disk);
			
			res = DiskDriver_readBlock(disk,dir_dest,pos_in_disk,sizeof(FirstDirectoryBlock));
			if(res == -1){
				fprintf(stderr, "Errore in SimpleFS_changeDir: errore della readBlock\n");
				return -1;
			}
			for(i=0; i<dim; i++){
				if(next_block->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,dir_dest,next_block->file_blocks[i],sizeof(FirstDirectoryBlock))) != -1){
					if(strcmp(dir_dest->fcb.name,dirname) == 0){
						DiskDriver_readBlock(disk,dir_dest,next_block->file_blocks[i],sizeof(FirstDirectoryBlock));
						d->pos_in_block = 0;
						d->parent_dir = fdb;
						d->dcb = dir_dest;
						return 0;
					}
				}
			}
			next_block = get_next_block_directory(next_block,disk);
		}
		
		fprintf(stderr, "Errore: non si può cambiare directory\n");
		return -1;
	}
}

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
//R. DA RIVEDERE, CAMBIATO QUALCOSA
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
	if(d == NULL || dirname == NULL){
		fprintf(stderr,"Errore in SimpleFS_mkDir: parametri inseriti non corretti\n");
		return -1;
	}
		
	int res;
	
	DiskDriver* disk = d->sfs->disk;
	FirstDirectoryBlock* fdb = d->dcb;
	//DirectoryBlock* db = d->current_block;
	
	//A. Controlliamo prima che la directory che sto creando non esista già
	res = SimpleFS_already_exists_directory(disk,fdb,dirname);
	if(res == -1){
		fprintf(stderr, "Errore in SimpleFS_mkDir: l'elemento già esiste opppure la SImpleFS_already_exists restituisce errore");
		return -1;
	}
	
	//A. Ora che sappiamo che non esiste un' altra cartella con lo stesso nome, possiamo continuare con la creazione
	int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
	if(new_block == -1){
		fprintf(stderr,"Errore in SimpleFS_mkDir: nessun blocco libero restituito da DiskDriver_getFreeBlock\n");
		return -1;
	}
	
	BlockIndex index_to_assign = fdb->index;
	
	FirstDirectoryBlock* dir_to_create = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
	
	dir_to_create->index = index_to_assign;
	
	dir_to_create->fcb.directory_block = fdb->fcb.block_in_disk;
	dir_to_create->fcb.block_in_disk = new_block;
	strcpy(dir_to_create->fcb.name, dirname);
	dir_to_create->fcb.written_bytes = 0;
	dir_to_create->fcb.size_in_bytes = 0;
	dir_to_create->fcb.size_in_blocks = 0;
	dir_to_create->fcb.is_dir = 1;
	
	
	
	res = DiskDriver_writeBlock(disk,dir_to_create,new_block,sizeof(FirstDirectoryBlock)); 
	if(res == -1){
		fprintf(stderr,"Errore in SimpleFS_mkDir: scrittura del blocco fallita da DiskDriver_writeBlock\n");
		return -1;
	}
	
	
	res = SimpleFS_assignDirectory(disk,fdb,new_block,0);
	if(res == -1){
		fprintf(stderr, "Errore in SimpleFS_mkDir: impossibile assegnare spazio della nuova directory in una directory");
		return -1;
	}
	
	return 0;
}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
//R. DA RIVEDERE PERCHE' CAMBIATO QUALCOSA
int SimpleFS_remove(DirectoryHandle* d, char* filename){	
	if(d == NULL || filename == NULL){
		fprintf(stderr,"Errore in SimpleFS_remove: parametri inseriti non corretti\n");
		return -1;
	}
		
	int i,res, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
	int pos = -1;
	
	DiskDriver* disk = d->sfs->disk;
	FirstDirectoryBlock* fdb = d->dcb;
	//DirectoryBlock* db = d->current_block;
	
	//A. Controllo se la directory è vuota. Se lo è inutile continuare, non c'è nulla da rimuovere
	if(fdb->num_entries < 1){
		fprintf(stderr,"Errore in SimpleFS_remove: non ci sono files da rimuovere, la directory è vuota\n");
		return -1;
	}
	
	//A. La directory non è vuota. Cerco il file nei blocchi directory
	pos = SimpleFS_already_exists_file(disk,fdb,filename);
	if(pos == -1){
		fprintf(stderr, "Errore in SimpleFS_remove: l'elemento che si sta cercando non è in questa directory");
		return -1;
	}
	
	FirstFileBlock ffb_toRemove;
	
	res = DiskDriver_readBlock(disk,&ffb_toRemove,pos,sizeof(FirstFileBlock));
	if(res == -1){
		fprintf(stderr,"Errore in SimpleFS_remove: lettura del FirstFileBlock fallita\n");
		return -1;
	};
	
	int isDir = ffb_toRemove.fcb.is_dir;
	
	//A. Verifico se sto rimuovendo un file o una cartella e mi regolo di conseguenza
	//A. Non sono per niente sicuro sia corretto. Probabilmente dovrà essere fixato.
	//A. Il problema è che vado a leggere un FileBlock nella stessa posizione in cui leggo prima il FirstFileBlock.
	if(isDir == 0){
		FileBlock current_fb;
		int fb_pos_in_disk = pos; 			//A. non vado ad alterare pos perchè mi servirà dopo per liberare il FirstFileBlock che è in posizione pos sul disco.
		
		//A. leggo uno dei FileBlock
		res = DiskDriver_readBlock(disk, &current_fb, fb_pos_in_disk, sizeof(FileBlock));
		if(res == -1){
			fprintf(stderr,"Errore in SimpleFS_remove: lettura del FileBlock fallita\n");
			return -1;
		}
		
		FileBlock* next_block = get_next_block_file(&current_fb,disk);
		
		//A. elimino tutti i FileBlock del file
		while(next_block != NULL){
			fb_pos_in_disk = get_position_disk_file_block(next_block,disk);
				
			res = DiskDriver_readBlock(disk, &current_fb, fb_pos_in_disk, sizeof(FileBlock));
			if(res == -1){
				fprintf(stderr, "Errore in SimpleFS_remove: DiskDriver_readBlock non legge\n");
				return -1;
			}
			next_block = get_next_block_file(next_block,disk);
				
			res = DiskDriver_freeBlock(disk,fb_pos_in_disk);
			if(res == -1){
				fprintf(stderr, "Errore in SimpleFS_remove: blocco in posizione fb_pos_in_disk non liberato\n");
				return -1;
			}
		}
		
		res = DiskDriver_freeBlock(disk,pos);
		if(res == -1){
			fprintf(stderr, "Errore in SimpleFS_remove: FirstFileBlock in posizione pos non liberato\n");
			return -1;
		}
		
		d->dcb = fdb; //A. ripristino la directory che precedentemente ho scorso
		
		return 0;
	}
	//A. sto eliminando invece una directory
	else{
		FirstDirectoryBlock fdb_toRemove;
		res = DiskDriver_readBlock(disk, &fdb_toRemove,pos, sizeof(FirstDirectoryBlock));
		if(res == -1){
			fprintf(stderr,"Errore in SimpleFS_remove: lettura del FirstDirectoryBlock fallita\n");
			return -1;
		}
		
		//A. La directory contiene elementi al suo interno
		if(fdb_toRemove.num_entries > 0){
			
			//A. Stesso problema di prima: vado a leggere un DirectoryBlock nella stessa posizione in cui leggo prima il FirstDirectoryBlock.
			DirectoryBlock current_db;
			int db_pos_in_disk = pos;								//A. non vado ad alterare pos perchè mi servirà dopo per liberare il FirstDirectoryBlock che è in posizione pos sul disco.
			
			
			res = DiskDriver_readBlock(disk,&current_db,db_pos_in_disk,sizeof(DirectoryBlock));
			if(res == -1){
				return -1;
			}
			
			//A. ricorsivamente elimino tutti i file
			for(i=0; i<dim; i++){
				FirstFileBlock ffb;
				if(current_db.file_blocks[i] > 0 && DiskDriver_readBlock(disk, &ffb, current_db.file_blocks[i], sizeof(FirstFileBlock)) != -1)
					SimpleFS_remove(d,ffb.fcb.name); 
			}
			
			//A. Non ho eliminato tutti gli elementi. Continuo.
			if(fdb_toRemove.num_entries > i){
				FirstFileBlock ffb;
				DirectoryBlock* next_block = get_next_block_directory(&current_db,disk);
			
				while(next_block != NULL){
					db_pos_in_disk = get_position_disk_directory_block(next_block,disk);
					
					res = DiskDriver_readBlock(disk, &ffb, db_pos_in_disk, sizeof(FirstFileBlock));
					if(res == -1){
						fprintf(stderr, "Errore in SimpleFS_remove: DiskDriver_readBlock non legge\n");
						return -1;
					}
				
					for(i=0; i<dim; i++){
						if(next_block->file_blocks[i] > 0 && DiskDriver_readBlock(disk,&ffb, next_block->file_blocks[i], sizeof(FirstFileBlock)) != -1){
							fprintf(stderr, "Errore in SimpleFS_remove: DiskDriver_readBlock non legge\n");
							return -1;
						}
						SimpleFS_remove(d,ffb.fcb.name);
					}
					next_block = get_next_block_directory(next_block,disk);
					
					res = DiskDriver_freeBlock(disk,db_pos_in_disk);
					if(res == -1){
						fprintf(stderr, "Errore in SimpleFS_remove: blocco in posizione db_pos_in_disk non liberato\n");
						return -1;
					}
				}
				
				res = DiskDriver_freeBlock(disk,pos);
				if(res == -1){
					fprintf(stderr, "Errore in SimpleFS_remove: FirstDirectoryBlock in posizione pos non liberato\n");
					return -1;
				}
				
				d->dcb = fdb; //A. ripristino la directory che precedentemente ho scorso
				return 0;
			} 
		}
		//A. La directory che sto eliminando non contiene nulla al suo interno
		else{
			res = DiskDriver_freeBlock(disk, pos);
			if(res == -1){
				fprintf(stderr, "Errore in SimpleFS_remove: FirstDirectoryBlock in posizione pos non liberato\n");
			}
			d->dcb = fdb;
			return 0;
		}
	}
	return 0;
}

//==================================================================//
//R. Ulteriori Funzioni
//==================================================================//

// Funzione per creare un nuovo blocco index, passandogli il predecessore
BlockIndex create_block_index(int previous){
	BlockIndex index;
	index.previous = previous;		// Si inserisce il predecessore 
	index.next = -1;				// Essendo appena inizializzato, ancora non c'è un successivo blocco index, quello corrente non è pieno
	
	int i;
	for(i=0; i<MAX_BLOCKS; i++){
		index.blocks[i] = -1;		// Tutti i blocchi vuoti settati a -1
	}
	return index;
}

//R. Funzione per ottenere il blocco index da un file
BlockIndex* get_block_index_file(FileBlock* file, DiskDriver* disk){
	BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
	if(DiskDriver_readBlock(disk, index, file->index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Errore nella get_block_index_file\n");
			free(index);
			return NULL;
		}
	return index;
}

//A. Funzione per ottenere il blocco index da una directory
BlockIndex* get_block_index_directory(DirectoryBlock* directory, DiskDriver* disk){
	BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
	if(DiskDriver_readBlock(disk, index, directory->index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Errore nella get block index directory\n");
			free(index);
			return NULL;
		}
	return index;
}

//R. Funzione che restituisce il blocco successivo file
FileBlock* get_next_block_file(FileBlock* file,DiskDriver* disk){
	BlockIndex* index = get_block_index_file(file,disk); //R. Estraggo il blocco index
	if(index == NULL){
		fprintf(stderr,"Error in get next block file\n");
		return NULL;
	}
	
	int current_position = file->position; //R. posizione nell'array index
	 
	//R. Caso in cui devo andare nel blocco index successivo
	if((current_position + 1) == MAX_BLOCKS){
		if(index->next == -1){
			fprintf(stderr,"Error in get next block file\n");
			free(index);
			return NULL;
		}
		BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, next, index->next, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			return NULL;
		}
		FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
		if(DiskDriver_readBlock(disk, next_file, next->blocks[0], sizeof(FileBlock)) == -1){
			fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			free(next);
			return NULL;
		}
		free(index);
		free(next);
		
		return next_file;
	}
	else{
	//R. Caso in cui mi trovo ancora nello stesso blocco index
	FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
	if(DiskDriver_readBlock(disk, next_file, index->blocks[current_position + 1], sizeof(FileBlock)) == -1){
			fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			return NULL;
		}
	free(index);
	
	return next_file;
	}
}

//A. Funzione che restituisce il blocco successivo directory
DirectoryBlock* get_next_block_directory(DirectoryBlock* directory,DiskDriver* disk){
	BlockIndex* index = get_block_index_directory(directory,disk); //R. Estraggo il blocco index
	if(index == NULL){
		fprintf(stderr,"Errore nella get next block directory\n");
		return NULL;
	}
	
	int current_position = directory->position; //A. posizione nell'array index
	 
	//A. Caso in cui devo andare nel blocco index successivo
	if((current_position + 1) == MAX_BLOCKS){
		if(index->next == -1){
			fprintf(stderr,"Error in get next block directory\n");
			free(index);
			return NULL;
		}
		BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, next, index->next, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Errore nella get next block directory\n");
			free(index);
			return NULL;
		}
		DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
		if(DiskDriver_readBlock(disk, next_directory, next->blocks[0], sizeof(DirectoryBlock)) == -1){
			fprintf(stderr,"Errore nella get next block directory\n");
			free(index);
			free(next);
			return NULL;
		}
		free(index);
		free(next);
		
		return next_directory;
	}
	else{
	//A. Caso in cui mi trovo ancora nello stesso blocco index
	DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(DiskDriver_readBlock(disk, next_directory, index->blocks[current_position + 1], sizeof(DirectoryBlock)) == -1){
			fprintf(stderr,"Errore nella get next block directory\n");
			free(index);
			return NULL;
		}
	free(index);
		
	return next_directory;
	}
}

//R. Funzione per creare un nuovo file blocks collegandolo con il blocco index di riferimento.
//   Restituisce il numero del blocco del disk driver su cui fare update, in caso di errore -1.
//   Utile soprattutto in casi di scrittura.
//   In FileBlock* new andiamo a restituire il blocco, il quale verrà riempito con le informazioni
//   e successivamente scritto nel disco (tramite writeBlock) nel int restituito dalla funzione.

int create_next_file_block(FileBlock* current_block, FileBlock* new, DiskDriver* disk){
	int current_position_in_index = current_block -> position;
	
	if(current_position_in_index + 1 == MAX_BLOCKS){
		//R. Caso in cui devo creare un nuovo blocco index e collegarlo
		BlockIndex* index = get_block_index_file(current_block,disk); //R. Recupero il blocco index
		if(index == NULL){
			fprintf(stderr,"Error: create_next_block_file, get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		//R. Ottengo il nuovo blocco libero per index
		int new_index_block = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(new_index_block == -1){
			fprintf(stderr,"Error: create_next_block_file, get free block\n");
			free(index);
			return -1;
		}
		
		//R. Ottengo il nuovo blocco libero per blocco
		int block_return = DiskDriver_getFreeBlock(disk, new_index_block + 1);
		if(block_return == -1){
			fprintf(stderr,"Error: create_next_block_file, get free block\n");
			free(index);
			return -1;
		}
		
		int index_block = current_block -> index_block; //R. Recupero il blocco index nel disk driver
		
		//R. Aggiorno il vecchio blocco index e lo riscrivo sul disco
		index->next = new_index_block;
		
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error:create next file block, update index block\n");
			free(index);
			return -1;
		}
		
		 //R. Creo il nuovo blocco index, lo aggiorno e lo scrivo sul blocco
		BlockIndex new_index = create_block_index(index_block);
		new_index.blocks[0] = block_return;
		if(DiskDriver_writeBlock(disk, &new_index, new_index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error:create next file block, write new index block\n");
			free(index);
			return -1;
		}
		
		//R. Inizializzo il nuovo blocco
		//FileBlock new_block;
		new->index_block = new_index_block;
		new->position = 0;
		//new = &new_block; //R. Salvo il file block nel puntatore che passo alla funzione
		
		free(index);
		
		return block_return;
		
	}
	else{
		//R. Caso in cui posso utilizzare il blocco index corrente
		
		BlockIndex* index = get_block_index_file(current_block,disk); //R. Recupero il blocco index
		if(index == NULL){
			fprintf(stderr,"Error: create_next_block_file, get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		int index_block = current_block -> index_block; //R. Recupero il blocco index nel disk driver
		
		//R. Inizializzo il nuovo blocco
		new->index_block = index_block;
		new->position = current_position_in_index + 1;
		
		//R. Ottengo il nuovo blocco libero
		int block_return = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(block_return == -1){
			fprintf(stderr,"Error: create_next_block_file, get free block\n");
			free(index);
			return -1;
		}
		
		//R. Aggiorno il blocco index e lo riscrivo sul disco
		index->blocks[new->position] = block_return;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error:create next file block, update index block\n");
			free(index);
			return -1;
		}
		
		free(index);
		
		return block_return;
	}

}


//A. Funzione per creare un nuovo directory block collegandolo con il blocco index di riferimento.
int create_next_directory_block(DirectoryBlock* current_block, DirectoryBlock* new, DiskDriver* disk){
	int current_position_in_index = current_block -> position;
	int i;
	
	//A. Caso in cui devo creare un nuovo blocco index e collegarlo
	if(current_position_in_index + 1 == MAX_BLOCKS){	
		
		BlockIndex* index = get_block_index_directory(current_block,disk); 
		if(index == NULL){
			fprintf(stderr,"Error: create_next_directory_block, get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		//A. Ottengo il nuovo blocco libero per index
		int new_index_block = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(new_index_block == -1){
			fprintf(stderr,"Error: create_next_directory_block, get free block\n");
			free(index);
			return -1;
		}
		
		//A. Ottengo il nuovo blocco libero per blocco
		int block_return = DiskDriver_getFreeBlock(disk, new_index_block + 1);
		if(block_return == -1){
			fprintf(stderr,"Error: create_next_directory_block, get free block\n");
			free(index);
			return -1;
		}
		
		int index_block = current_block -> index_block; 
		
		//A. Aggiorno il vecchio blocco index e lo riscrivo sul disco
		index->next = new_index_block;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error: create_next_directory_block, update index block\n");
			free(index);
			return -1;
		}
		
		//A. Creo il nuovo blocco index, lo aggiorno e lo scrivo sul blocco
		BlockIndex new_index = create_block_index(index_block);
		new_index.blocks[0] = block_return;
		if(DiskDriver_writeBlock(disk, &new_index, new_index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error: create_next_directory_block, write new index block\n");
			free(index);
			return -1;
		}
		
		//A. Si Inizializza il nuovo blocco
		new->index_block = new_index_block;
		new->position = 0;
		for(i=0;i<((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));i++)
			new->file_blocks[i] = 0;
		
		free(index); 
		
		return block_return;
		
	}
	else{
		
		//A. Caso in cui posso utilizzare il blocco index corrente
		BlockIndex* index = get_block_index_directory(current_block,disk); 
		if(index == NULL){
			fprintf(stderr,"Error: create_next_directory_block, get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		int index_block = current_block -> index_block; 
		
		//A. Inizializzo il nuovo blocco
		new->index_block = index_block;
		new->position = current_position_in_index + 1;
		for(i=0;i<((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));i++)
			new->file_blocks[i] = 0; 
		
		//A. Ottengo il nuovo blocco libero
		int block_return = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(block_return == -1){
			fprintf(stderr,"Error: create_next_directory_block, get free block\n");
			free(index);
			return -1;
		}
		
		//A. Aggiorno il blocco index e lo riscrivo sul disco
		index->blocks[new->position] = block_return;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error:create_next_directory_block, update index block\n");
			free(index);
			return -1;
		}
		
		free(index);
		
		return block_return;
	}

}

// Funzione per ottenere la posizione nel disco di un file block
int get_position_disk_file_block(FileBlock* file_block, DiskDriver* disk){
	BlockIndex index;
	if(DiskDriver_readBlock(disk, &index, file_block->index_block, sizeof(BlockIndex)) == -1){
		fprintf(stderr,"Error: could not read block index.\n");
		return -1;
	}
	return index.blocks[file_block->position];
}

// Funzione per ottenere la posizione nel disco di un directory block
int get_position_disk_directory_block(DirectoryBlock* directory_block, DiskDriver* disk){
	BlockIndex index;
	if(DiskDriver_readBlock(disk, &index, directory_block->index_block, sizeof(BlockIndex)) == -1){
		fprintf(stderr,"Error: could not read block index.\n");
		return -1;
	}
	return index.blocks[directory_block->position];
}

// Funzione per cercare se un file con nome elem_name è gia presente sul disco.
// Restituisce la posizione dell'elemento nel blocco directory in caso trovi il file sul disco, -1 in caso di errore, -2 in caso non lo trovi. 
int SimpleFS_already_exists_file(DiskDriver* disk, FirstDirectoryBlock* fdb, char* elem_name){
	if(disk == NULL || fdb == NULL || elem_name == NULL){
		fprintf(stderr, "Errore in SImpleFS_already_exists_file: parametri inseriti non corretti\n");
		return -1;
	}
	
	//R. Estraggo il primo DirectoryBlock
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error in SimpleFS_already_exists_file: malloc on db in SimpleFS_already_exists");
		return -1;
	}
	
	if(DiskDriver_readBlock(disk,db,fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_already_exists_file: could not read directory block one.\n");
		free(db);
		return -1;
	}
	
	FirstFileBlock ffb_to_check;
	int i,res, pos_in_disk, elem_pos, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);

	
	//A. Controlliamo prima che la directory che sto creando non esista già
	if(fdb->num_entries > 0){	
		for(i=0; i<dim; i++){
			if(db->file_blocks[i] > 0 && DiskDriver_readBlock(disk,&ffb_to_check,db->file_blocks[i],sizeof(FirstFileBlock)) != -1){
				if(strcmp(ffb_to_check.fcb.name,elem_name) == 0){ 						
					//esiste già un elemento con lo stesso nome presente sul disco
					elem_pos = db->file_blocks[i];
					free(db);
					return elem_pos;
				}
			}
		}

		//A. Se ci sono altri blocchi directory vanno controllati anche quelli
		if(fdb->num_entries > i){
			db = get_next_block_directory(db,disk);
			
			while(db != NULL){
				pos_in_disk = get_position_disk_directory_block(db,disk);
				
				res = DiskDriver_readBlock(disk, &ffb_to_check, pos_in_disk, sizeof(FirstFileBlock));
				if(res == -1){
					fprintf(stderr, "Errore in SimpleFS_already_exists_file: DiskDriver_readBlock non legge\n");
					free(db);
					return -1;
				}
				
				for(i=0; i<dim; i++){
					if(db->file_blocks[i] > 0 && DiskDriver_readBlock(disk,&ffb_to_check,db->file_blocks[i], sizeof(FirstFileBlock)) != -1){
						if(strcmp(ffb_to_check.fcb.name,elem_name) == 0){
							//esiste già un elemento con lo stesso nome presente sul disco
							elem_pos = db->file_blocks[i];
							free(db);
							return elem_pos;
						}
					}
				}
				db = get_next_block_directory(db,disk);
			}
		}
	}
	free(db);
	return -2;
}

// Funzione per cercare se una directory con nome elem_name è gia presente sul disco.
// Restituisce la posizione dell'elemento nel blocco directory in caso trovi la directory sul disco, -1 in caso non la trovi o di errore. 
int SimpleFS_already_exists_directory(DiskDriver* disk, FirstDirectoryBlock* fdb, char* elem_name){
	if(disk == NULL || fdb == NULL || elem_name == NULL){
		fprintf(stderr, "Errore in SimpleFS_already_exists_directory: parametri inseriti non corretti\n");
		return -1;
	}
	
	//A. Estraggo il primo DirectoryBlock
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error in SimpleFS_already_exists_directory: malloc on db in SimpleFS_already_exists");
		return -1;
	}
	if(DiskDriver_readBlock(disk,db,fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_already_exists_directory: could not read directory block one.\n");
		free(db);
		return -1;
	}
	
	FirstDirectoryBlock fdb_to_check;
	int i,res, pos_in_disk, elem_pos, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);

	
	//A. Controlliamo prima che la directory che sto creando non esista già
	if(fdb->num_entries > 0){	
		for(i=0; i<dim; i++){
			if(db->file_blocks[i] > 0 && DiskDriver_readBlock(disk,&fdb_to_check,db->file_blocks[i],sizeof(FirstFileBlock)) != -1){
				if(strcmp(fdb_to_check.fcb.name,elem_name) == 0){ 						
					//esiste già un elemento con lo stesso nome presente sul disco
					elem_pos = db->file_blocks[i];
					free(db);
					return elem_pos;
				}
			}
		}

		//A. Se ci sono altri blocchi directory vanno controllati anche quelli
		if(fdb->num_entries > i){
			db = get_next_block_directory(db,disk);
			
			while(db != NULL){
				pos_in_disk = get_position_disk_directory_block(db,disk);
				
				res = DiskDriver_readBlock(disk, &fdb_to_check, pos_in_disk, sizeof(FirstFileBlock));
				if(res == -1){
					fprintf(stderr, "Errore in SimpleFS_already_exists_directory: DiskDriver_readBlock non legge\n");
					free(db);
					return -1;
				}
				
				for(i=0; i<dim; i++){
					if(db->file_blocks[i] > 0 && DiskDriver_readBlock(disk,&fdb_to_check,db->file_blocks[i], sizeof(FirstFileBlock)) != -1){
						if(strcmp(fdb_to_check.fcb.name,elem_name) == 0){
							//esiste già un elemento con lo stesso nome presente sul disco
							elem_pos = db->file_blocks[i];
							free(db);
							return elem_pos;
						}
					}
				}
				db = get_next_block_directory(db,disk);
			}
		}
	}
	free(db);
	return -1;
}

int SimpleFS_assignDirectory(DiskDriver* disk, FirstDirectoryBlock* fdb, int pos_ffb, int pos_fb){
	if(disk == NULL || fdb == NULL || pos_ffb == -1 || pos_fb == -1){
		fprintf(stderr, "Errore in SimpleFS_assignDirectory: parametri inseriti non corretti");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		return -1;
	}
	
	int i, block_pos, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
	int found = 0;
	
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error: malloc on db in SimpleFS_assignDirectory");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		return -1;
	}
	
	//R. Estraggo il primo DirectoryBlock
	if(DiskDriver_readBlock(disk, db, fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error: read directory block one on disk.\n");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		free(db);
		return -1;
	}
	
	
	//A. Controlliamo se ci sta spazio nei blocchi directory
	for(i=0; i<dim; i++){
		if(db->file_blocks[i] == 0){  
			found = 1;
			block_pos = i;
			break;
		}
	}
	
	//A. Ci sono altri blocchi da controllare e non abbiamo trovato ancora un blocco libero
	if(fdb->num_entries > i && found == 0){
		db = get_next_block_directory(db,disk);
			
			while(db != NULL && found == 0){
				
				for(i=0; i<dim; i++){
					if(db->file_blocks[i] == 0){
						found = 1;
						block_pos = i;
						break;
					}
				}
				db = get_next_block_directory(db,disk);
			}
	}
	
	//A. Non c'era spazio nei precedenti blocchi directory. Devo usarne uno nuovo. Mi procuro un blocco libero e ci scrivo sopra il DirectoryBlock
	if(found == 0){
		DirectoryBlock* db_new = (DirectoryBlock*)malloc(sizeof(DirectoryBlock*));
		if(db_new == NULL){
			fprintf(stderr,"Error: malloc on db_new.\n");
			DiskDriver_freeBlock(disk, pos_ffb);
			DiskDriver_freeBlock(disk, pos_fb);
			free(db);
			return -1;
		}
		int pos = create_next_directory_block(db, db_new, disk);
		if(pos == -1){
			fprintf(stderr,"Error: crate next directory block.\n");
			DiskDriver_freeBlock(disk, pos_ffb);
			DiskDriver_freeBlock(disk, pos_fb);
			free(db);
			free(db_new);
			return -1;
		}
		db_new->file_blocks[0] = pos_ffb;
		
		if(DiskDriver_writeBlock(disk, db_new, pos, sizeof(DirectoryBlock)) == -1){
			fprintf(stderr,"Error: write block of db_new.\n");
			DiskDriver_freeBlock(disk, pos_ffb);
			DiskDriver_freeBlock(disk, pos_fb);
			free(db);
			free(db_new);
			return -1;
		}
		free(db_new);
	}
	//R. Aggiorno il blocco precedente che ha ancora spazio
	else{
		db->file_blocks[block_pos] = pos_ffb;
	}
	
	fdb->num_entries++;
	
	//A. Aggiorno il FirsDirectoryBlock e il DirectoryBlock su cui ho scritto il file
	if(DiskDriver_updateBlock(disk, fdb, fdb->fcb.block_in_disk, sizeof(FirstDirectoryBlock)) == -1){
		fprintf(stderr,"Errore in SimpleFS_assignDirectory: updateBlock di fdb.\n");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		free(db);
		return -1;
	}
	
	//A. E aggiorno il DirectoryBlock su cui ho scritto il file
	int pos_db = get_position_disk_directory_block(db,disk);
	if(DiskDriver_updateBlock(disk, db, pos_db, sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Errore in SimpleFS_assignDirectory: updateBlock di db.\n");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		free(db);
		return -1;
	}
	
	
	free(db);
	return 0;	
}

void print_index_block(BlockIndex* index){
	printf("============ INDEX BLOCK ============\n");
	printf("Block_index_information:\n");
	printf("Previous: %d\n",index->previous);
	int i;
	for(i=0;i<MAX_BLOCKS;i++){
		printf("Blocco %d: %d\n",i+1,index->blocks[i]);
	}
	printf("Next: %d\n",index->next);
	printf("=====================================\n");
}


  
