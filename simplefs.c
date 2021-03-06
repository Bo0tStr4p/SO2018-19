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
	for(i=0; i<MAX_BLOCKS_FIRST; i++){
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
		fprintf(stderr, "Error in SimpleFS_format: getFreeBlock.\n");
		return;
	}
	
	root_directory.index.blocks[0] = free_block;
	
	if(DiskDriver_writeBlock(fs->disk, &dir_block, free_block, sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_format: writeBlock.\n");
		return;
	}
	
	int ret = DiskDriver_writeBlock(fs->disk, &root_directory, 0, sizeof(FirstDirectoryBlock));		//A. vado a scrivere sul disco la root directory
	if (ret == -1){
		fprintf(stderr, "Error in SimpleFS_format: could not write root directory.\n");
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
		fprintf(stderr,"Error in SimpleFS_createFile: bad parameters.\n");
		return NULL;
	}
	
	//A. Innanzitutto devo controllare che il file non sia già presente sul disco. Prima ci sono dei controlli.
	SimpleFS* fs = d->sfs;
	DiskDriver* disk = fs->disk;                   
	FirstDirectoryBlock* fdb = d->dcb;
	//DirectoryBlock* db = d->current_block; //R. Non serve
	if(fs == NULL || disk == NULL || fdb == NULL){ 
		fprintf(stderr,"Error in SimpleFS_createFile: DirectoryHandle bad parameter.\n");
		return NULL;
	}
	
	//A. Controllo.
	int ret,i;
	ret = SimpleFS_already_exists(disk,fdb,(char*)filename);
	if(ret == -1){
		fprintf(stderr, "Error in SimpleFS_createFile: SimpleFS_already_exists error.\n");
		return NULL;
	}
	if(ret != -2){
		//fprintf(stderr, "Errore in SimpleFS_createFile: il file che stai creando é già presente sul disco\n");
		return NULL;
	} 
	
	//A. Il file non esiste, possiamo crearlo da 0. Prendiamo dal disco il primo blocco libero
	int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
	if(new_block == -1){
		fprintf(stderr, "Error in createFile: no free block on disk.\n");
		return NULL;	//A. se non ci sono blocchi liberi sul disco interrompiamo
	}
	
	//A. creiamo il primo blocco del file
	FirstFileBlock* file_to_create = malloc(sizeof(FirstFileBlock));  
	if(file_to_create == NULL){
		fprintf(stderr,"Error in SimpleFS_createFile: malloc on file_to_create.\n");
		return NULL;
	} 
	file_to_create->index.previous = -1;
	file_to_create->index.next = -1;
	file_to_create->num_blocks = 0;

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
		fprintf(stderr,"Error in SimpleFS_createFile: getFreeBlock.\n");
		free(file_to_create);
		return NULL;
	}
	
	file_to_create->index.blocks[0] = free_block;
	
	FileBlock* file = (FileBlock*)malloc(sizeof(FileBlock));
	if(file == NULL){
		fprintf(stderr,"Error in SimpleFS_createFile: malloc on file.\n");
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
		fprintf(stderr, "Error in SimpleFS_createFile: could not write on disk.\n");
		free(file_to_create);
		free(file);
		return NULL;
	}
	
	ret = DiskDriver_writeBlock(disk, file , free_block, sizeof(FileBlock));
	if(ret == -1){
		fprintf(stderr, "Error in SimpleFS_createFile: could not writeBlock.\n");
		free(file_to_create);
		free(file);
		return NULL;
	}
	
	//A. Dobbiamo mettere il file in un blocco directory.
	ret = SimpleFS_assignDirectory(disk, fdb , new_block, free_block);
	if(ret == -1){
		fprintf(stderr, "Error in SimpleFS_createFile: could not assignDirectory.\n");
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
		fprintf(stderr, "Error in SimpleFS_readDir: bad parameters.\n");
		return -1;
	}
	
	DiskDriver* disk = d->sfs->disk;
	FirstDirectoryBlock *fdb = d->dcb;
	//DirectoryBlock* db = d->current_block; //R. Non serve
	
	//A. Se la directory è vuota, inutile procedere
	if(fdb->num_entries <= 0){
		//fprintf(stderr, "Error in SimpleFS_readDir: empty directory.\n");
		return -1;
	}
	
	int i, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int), num_tot = 0;
	FirstFileBlock ffb_to_check; 
	
	//R. Estraggo il primo DirectoryBlock
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		//fprintf(stderr,"Error in SimpleFS_readDir: malloc on db.\n");
		return -1;
	}
	
	if(DiskDriver_readBlock(disk,db,d->dcb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		//fprintf(stderr,"Error in SimpleFS_readDir: could not read directory block one.\n");
		free(db);
		return -1;
	}
	
	//A. Iniziamo a leggere i file contenuti nel blocco directory in cui ci troviamo, cioè d->current_block
	for (i=0; i<dim; i++){	
		if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &ffb_to_check, db->file_blocks[i], sizeof(FirstFileBlock)) != -1){ 
			strncpy(names[num_tot],ffb_to_check.fcb.name, 128); 				//A. Salvo il nome del file che sto leggendo nell'array names
			flag[num_tot] = ffb_to_check.fcb.is_dir;							//R. Salvo se è file o directory
			num_tot++;
		}
		//dim_names++;
	}
	
	//A. Caso in cui ci sono file non contenuti nello stesso blocco directory e quindi bisogna cambiare blocco
	if (fdb->num_entries > i){	
		
		if(fdb->fcb.block_in_disk == db->index_block)
			db = get_next_block_directory_first(db, disk);
		else
			db = get_next_block_directory(db, disk);

		while (db != NULL){	 

			for (i=0; i<dim; i++){	 
				if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &ffb_to_check, db->file_blocks[i], sizeof(FirstFileBlock)) != -1){ 
					strncpy(names[num_tot],ffb_to_check.fcb.name, 128); 				//A. Salvo il nome del file che sto leggendo nell'array names
					flag[num_tot] = ffb_to_check.fcb.is_dir;							//R. Salvo se è file o directory
					num_tot++;
				}
				//dim_names++;
			}
			if(fdb->fcb.block_in_disk == db->index_block)
				db = get_next_block_directory_first(db, disk);
			else
				db = get_next_block_directory(db, disk);
		}
	}
	free(db);
	return 0;
}


// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
	//R. Verifico le condizioni iniziali
	if (d == NULL || filename == NULL){
		fprintf(stderr, "Error in SimpleFS_openFile: could not open file, bad Parameters\n");
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
			fprintf(stderr,"Error in SimpleFS_openFile: could not malloc file handle.\n");
			return NULL;
		}
		file_handle->sfs = d->sfs;
		file_handle->directory = first_directory_block;
		file_handle->pos_in_file = 0;

		//R. Utilizzo la variabile d'appoggio is_found per verificare se ho trovato o meno il file
		int is_found = 0;
		FirstFileBlock* to_check = (FirstFileBlock*)malloc(sizeof(FirstFileBlock));
		if(to_check == NULL){
			fprintf(stderr,"Error in SimpleFS_openFile: could not malloc first file block.\n");
			free(file_handle);
			return NULL;
		}
		
		//R. Verifico l'esistenza del file nei Directory Block
		
		//R. Estraggo il primo Directory Block
		DirectoryBlock* dir_block = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
		if(dir_block == NULL){
			fprintf(stderr,"Error in SimpleFS_openFile: malloc dir_block.\n");
			free(file_handle);
			free(to_check);
			return NULL;
		}
		
		if(DiskDriver_readBlock(disk, (void*)dir_block, first_directory_block->index.blocks[0], sizeof(DirectoryBlock)) == -1){
			fprintf(stderr,"Error in SimpleFS_openFile: read file block 1.\n");
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
			if(first_directory_block->fcb.block_in_disk == dir_block->index_block)
				dir_block = get_next_block_directory_first(dir_block, disk);
			else
				dir_block = get_next_block_directory(dir_block, disk);
		}
		
		//R. Verifico se il file è stato trovato e restituisco il suo file handle
		if (is_found){
			free(dir_block);
			return file_handle;
		} else {
			//fprintf(stderr,"Error, could not open file: file doesn't exist.\n");
			free(file_handle);
			free(to_check);
			free(dir_block);
			return NULL;
		}

	//R. Caso in cui la directory è vuota
	} else { 
		fprintf(stderr,"Error in SimpleFS_openFile: could not open file, directory is empty.\n");
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
	if(f->dcb != NULL)
		free(f->dcb);
	if(f->parent_dir != NULL)
		free(f->parent_dir);
	free(f);
	return 0;
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written

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
		fprintf(stderr,"Error in SimpleFS_write: malloc of file_block_tmp.\n");
		return -1;
	}
	
	//R. Calcolo il blocco al quale devo accedere
	int file_index_pos;
	int index_block_ref = off - MAX_BLOCKS_FIRST*space_file_block;
	if(index_block_ref < 0){
		index_block_ref = 0;
		file_index_pos = off / (MAX_BLOCKS*space_file_block);
		off = off - file_index_pos * space_file_block;
	}
	else{
		index_block_ref = index_block_ref / (MAX_BLOCKS*space_file_block);
		file_index_pos = ((off - MAX_BLOCKS_FIRST*space_file_block)-index_block_ref*(MAX_BLOCKS*space_file_block))/space_file_block;
		off = off - (off - MAX_BLOCKS_FIRST*space_file_block)-index_block_ref*(MAX_BLOCKS*space_file_block);
	}
	
		
	FirstBlockIndex index = first_file->index;
		
	//R. mi posiziono al blocco index di riferimento
	for(i=0;i<index_block_ref;i++){
		if(DiskDriver_readBlock(my_disk, (void*)&index, index.next, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in SimpleFS_write: next block in readBlock.\n");
			return -1;
		}
	}
	
	//R. Caso in cui posso scrivere direttamente nel primo blocco
	if(off < space_file_block){
		//R. Estraggo il File Block
		if(DiskDriver_readBlock(my_disk,(void*) file_block_tmp, index.blocks[file_index_pos],sizeof(FileBlock)) == -1){
			fprintf(stderr,"Error in SimpleFS_write: could not read file block 1.\n");
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
				fprintf(stderr,"Error in SimpleFS_write: could not update file block 1.\n");
				free(file_block_tmp);
				return -1;
			}
			if(DiskDriver_updateBlock(my_disk, ffb, ffb->fcb.block_in_disk,sizeof(FirstFileBlock)) == -1){ //R. Aggiorno il first file block
				fprintf(stderr,"Error in SimpleFS_write: could not update ffb.\n");
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
				fprintf(stderr,"Error in SimpleFS_write: could not updateBlock.\n");
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
		if(first_file->fcb.block_in_disk == file_block_tmp->index_block)
			block_position = create_next_file_block_first(current, file_block_tmp, my_disk);
		else
			block_position = create_next_file_block(current, file_block_tmp, my_disk);
		
		if(block_position == -1){
			fprintf(stderr,"Error in SimpleFS_write: could not create next file block.\n");
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
				fprintf(stderr,"Error in SimpleFS_write: could not updateBlock ffb.\n");
				free(file_block_tmp);
				return -1;
			}
			if(DiskDriver_writeBlock(my_disk, file_block_tmp, block_position, sizeof(FileBlock)) == -1){
				fprintf(stderr,"Error in SimpleFS_write: could not write next file block on disk.\n");
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
				fprintf(stderr,"Error in SimpleFS_write: could not write next file block on disk.\n");
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

int SimpleFS_read(FileHandle* f, void* data, int size){
	FirstFileBlock* first_file = f->fcb; //R. Estraggo il FirstFileBlock
	DiskDriver* my_disk = f->sfs->disk; //R. Estraggo il disco
	int i;
	
	int off = f->pos_in_file;															
	int written_bytes = first_file->fcb.written_bytes;													
	
	//R. Controllo che la parte da leggere non vada oltre ciò che si trova nel file
	if(size+off > written_bytes){																
		fprintf(stderr,"Error in SimpleFS_read: could not read, size+off > written_bytes.\n");
		return -1;
	}
	
	int bytes_read = 0;
	int to_read = size;
	int space_file_block = BLOCK_SIZE - sizeof(int) - sizeof(int);
	
	FileBlock* file_block_tmp = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block_tmp == NULL){
		fprintf(stderr,"Error in SimpleFS_read: malloc file_block_tmp.\n");
		return -1;
	}
	
	//R. Calcolo il blocco al quale devo accedere
	int file_index_pos;
	int index_block_ref = off - MAX_BLOCKS_FIRST*space_file_block;
	if(index_block_ref < 0){
		index_block_ref = 0;
		file_index_pos = off / (MAX_BLOCKS*space_file_block);
		off = off - file_index_pos * space_file_block;
	}
	else{
		index_block_ref = index_block_ref / (MAX_BLOCKS*space_file_block);
		file_index_pos = ((off - MAX_BLOCKS_FIRST*space_file_block)-index_block_ref*(MAX_BLOCKS*space_file_block))/space_file_block;
		off = off - (off - MAX_BLOCKS_FIRST*space_file_block)-index_block_ref*(MAX_BLOCKS*space_file_block);
	}
		
	FirstBlockIndex index = first_file->index;
		
	//R. mi posiziono al blocco index di riferimento
	for(i=0;i<index_block_ref;i++){
		if(DiskDriver_readBlock(my_disk, (void*)&index, index.next, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in SimpleFS_read: readBlock.\n");
			return -1;
		}
	}
		
	//R. Estraggo il FileBlock da cui partire
	if(DiskDriver_readBlock(my_disk, (void*)file_block_tmp, index.blocks[file_index_pos], sizeof(FileBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_read: could not read file block.\n");
		return -1;
	}
	
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
		if(first_file->fcb.block_in_disk == file_block_tmp->index_block)
			file_block_tmp =  get_next_block_file_first(file_block_tmp,my_disk);
		else
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
		fprintf(stderr,"Error: simpleFS_seek.\n");
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
		fprintf(stderr,"Error in SimpleFS_changeDir: bad parameters.\n");
		return -1;
	}
	
	//A. Confrontando il nome con ".." vedo se il comando inserito mi chiede di andare alla cartella genitore
	if(strcmp(dirname,"..") == 0){	
		if(d->dcb->fcb.block_in_disk == 0){ 												//A. Controllo se la directory in cui sto è la root
			//fprintf(stderr, "Error in SimpleFS_changeDir: this is the root directory.\n");
			return -1;
		}
		free(d->dcb);
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
			//fprintf(stderr, "Error in SimpleFS_changeDir: could not read parent directory.\n");
			d->parent_dir = NULL;
			//free(parent_directory);
			return 0; 
		}
		else{
			d->parent_dir = parent_directory;
		}
		return 0;
	}
	
	//A. caso in cui la directory in cui sto dentro è vuota
	else if(d->dcb->num_entries < 0){ 
		//fprintf(stderr, "Error in SimpleFS_changeDir: empty directory.\n");
		return -1;
	}
	else{
		//A. caso normale in cui mi sposto semplicemente in un' altra cartella
		FirstDirectoryBlock* fdb = d->dcb;
		DiskDriver* disk = d->sfs->disk;
		
		FirstDirectoryBlock* dir_dest = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
		if(dir_dest == NULL){
			fprintf(stderr,"Error in SimpleFS_changeDir: could not create first directory block.\n");
			return -1;
		}
		
		//A. Estraggo il primo DirectoryBlock
		DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
		if(db == NULL){
			//fprintf(stderr,"Error in SimpleFS_changeDir: malloc on db.\n");
			return -1;
		}
	
		if(DiskDriver_readBlock(disk,db,fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
			//fprintf(stderr,"Error in SimpleFS_changeDir: could not read directory block one.\n");
			free(db);
			free(dir_dest);
			return -1;
		}
		
		//A. Controllo se la cartella in cui mi devo spostare è in questo blocco directory
		int i;
		int dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
		/*
		for(i=0; i<dim; i++){
			if(db->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock))) != -1){
				if(strcmp(dir_dest->fcb.name,dirname) == 0){
					DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock));
					d->pos_in_block = 0; 
					d->parent_dir = fdb;
					d->dcb = dir_dest;
					free(db);
					return 0;
				}
			}
		}
		*/
		//A. Altrimenti continuo a cercare negli altri blocchi directory
		//int pos_in_disk;
		//DirectoryBlock* next_block = get_next_block_directory(db,disk);
		
		while(db != NULL){
			/*pos_in_disk = get_position_disk_directory_block(next_block,disk);
			
			res = DiskDriver_readBlock(disk,dir_dest,pos_in_disk,sizeof(FirstDirectoryBlock));
			if(res == -1){
				fprintf(stderr, "Errore in SimpleFS_changeDir: errore della readBlock\n");
				free(db);
				return -1;
			}*/
			for(i=0; i<dim; i++){
				if(db->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock))) != -1){
					if(strncmp(dir_dest->fcb.name,dirname,128) == 0){
						//DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock));
						d->pos_in_block = 0;
						if(d->parent_dir != NULL)
							free(d->parent_dir);
						d->parent_dir = fdb;
						d->dcb = dir_dest;
						free(db);
						return 0;
					}
				}
			}
			if(fdb->fcb.block_in_disk == db->index_block)
				db = get_next_block_directory_first(db, disk);
			else
				db = get_next_block_directory(db,disk);
		}
		
		//fprintf(stderr, "Errore in SimpleFS_changeDir: could not change directory.\n");
		free(db);
		free(dir_dest);
		return -1;
	}
}

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error

int SimpleFS_mkDir(DirectoryHandle* d, char* dirname){
	if(d == NULL || dirname == NULL){
		fprintf(stderr,"Error in SimpleFS_mkDir: bad paremeters.\n");
		return -1;
	}
		
	int i;
	
	DiskDriver* disk = d->sfs->disk;
	FirstDirectoryBlock* fdb = d->dcb;
	//DirectoryBlock* db = d->current_block;
	
	//A. Controlliamo prima che la directory che sto creando non esista già
	int ret = SimpleFS_already_exists(disk,fdb,dirname);
	if(ret == -1){
		fprintf(stderr, "Error in SimpleFS_mkDir: l SimpleFS_already_exists error.\n");
		return -1;
	}
	if(ret != -2){
		//fprintf(stderr, "Errore in SimpleFS_mkDir: la directory che stai creando é già presente sul disco\n");
		return -1;
	} 
	
	//A. Ora che sappiamo che non esiste un' altra cartella con lo stesso nome, possiamo continuare con la creazione
	int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
	if(new_block == -1){
		fprintf(stderr,"Error in SimpleFS_mkDir: no free block.\n");
		return -1;
	}
	
	FirstDirectoryBlock* dir_to_create = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
	if(dir_to_create == NULL){
		fprintf(stderr,"Error in SimmpleFS_mkDir: malloc on dir_to_create.\n");
		return -1;
	}
	
	dir_to_create->fcb.directory_block = fdb->fcb.block_in_disk;
	dir_to_create->fcb.block_in_disk = new_block;
	strncpy(dir_to_create->fcb.name, dirname,128);
	dir_to_create->fcb.written_bytes = 0;
	dir_to_create->fcb.size_in_bytes = 0;
	dir_to_create->fcb.size_in_blocks = 0;
	dir_to_create->fcb.is_dir = 1;
	dir_to_create->num_entries = 0;
	
	FirstBlockIndex index = create_block_index_first(-1);
	
	//R. Creo il primo blocco index
	DirectoryBlock dir_block = {
		.index_block = new_block, 
		.position = 0
	};
	
	for(i=0;i<((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));i++)
			dir_block.file_blocks[i] = 0;
	
	
	int free_block = DiskDriver_getFreeBlock(disk, new_block + 1);
	if(new_block == -1){
		fprintf(stderr,"Error in SimpleFS_mkDir: no free block from DiskDriver_getFreeBlock.\n");
		return -1;
	}
	
	index.blocks[0] = free_block;
	
	dir_to_create->index = index;
	
	 
	if(DiskDriver_writeBlock(disk,dir_to_create,new_block,sizeof(FirstDirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_mkDir: could not write block.\n");
		return -1;
	}
	 
	if(DiskDriver_writeBlock(disk,&dir_block, free_block, sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_mkDir: could not DiskDriver_writeBlock.\n");
		return -1;
	}
	
	
	if(SimpleFS_assignDirectory(disk,fdb,new_block,free_block) == -1){
		fprintf(stderr, "Error in SimpleFS_mkDir: could not assignDirectory.\n");
		return -1;
	}
	
	free(dir_to_create);
	return 0;
}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files

int SimpleFS_remove(DirectoryHandle* d, char* filename){	
	if(d == NULL || filename == NULL){
		fprintf(stderr,"Error: could not use SimpleFS_remove. Bad parameters.\n");
		return -1;
	}
	
	int idx,i;
	
	FirstDirectoryBlock* fdb = d->dcb;
	DiskDriver* disk = d->sfs->disk;
	
	//A. Controllo se la directory è vuota. Se lo è inutile continuare, non c'è nulla da rimuovere
	if(fdb->num_entries < 1){
		fprintf(stderr,"Error in SimpleFS_remove: empty directory.\n");
		return -1;
	}
	
	DirectoryBlock* db_update = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db_update == NULL){
		fprintf(stderr, "Error in SimpleFS_remove: malloc on db_update.\n");
		return -1;
	}
	
	//A. La directory non è vuota. Cerco il file nei blocchi directory
	int exist_pos = SimpleFS_already_exists_remove(disk, fdb, filename, db_update, &idx);
	if(exist_pos == -1 || exist_pos == -2){
		fprintf(stderr,"Error in SimpleFS_remove: file or directory named %s doesn't exist.\n",filename);
		free(db_update);
		return -1;
	}
	
	FirstFileBlock* ffb = (FirstFileBlock*)malloc(sizeof(FirstFileBlock));
	if(ffb == NULL){
		fprintf(stderr,"Error in SimpleFS_remove: malloc on ffb");
		free(db_update);
		return -1;
	}
	
	if(DiskDriver_readBlock(disk, ffb, exist_pos, sizeof(FirstFileBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_remove: read on ffb");
		free(db_update);
		free(ffb);
		return -1;
	}
	
	//A. Verifico se sto rimuovendo un file o una cartella e mi regolo di conseguenza
	//R. Caso in cui dobbiamo rimuovere un file
	if(ffb->fcb.is_dir == 0){
		//R. Estraggo il primo file block
		FileBlock* fb = (FileBlock*)malloc(sizeof(FileBlock));
		if(fb == NULL){
			fprintf(stderr,"Error in SimpleFS_remove: malloc on fb.\n");
			free(ffb);
			free(db_update);
			return -1;
		}
		
		if(DiskDriver_readBlock(disk, fb, ffb->index.blocks[0], sizeof(FirstFileBlock)) == -1){
			fprintf(stderr,"Error in SimpleFS_remove: read on ffb");
			free(db_update);
			free(ffb);
			free(fb);
			return -1;
		}
		
		while(fb != NULL){
			int position;
			
			if(ffb->fcb.block_in_disk == fb->index_block)
				position = get_position_disk_file_block_first(fb, disk);
			else
				position = get_position_disk_file_block(fb, disk);
			
			if(DiskDriver_freeBlock(disk, position) == -1){
				fprintf(stderr,"Error in SimpleFS_remove: free_block.\n");
				free(db_update);
				free(ffb);
				free(fb);
			}
			if(ffb->fcb.block_in_disk == fb->index_block)
				fb = get_next_block_file_first(fb, disk);
			else
				fb = get_next_block_file(fb, disk);
		}
		
		if(DiskDriver_freeBlock(disk, exist_pos) == -1){
				fprintf(stderr,"Error in SimpleFS_remove: free_block.\n");
				free(ffb);
				free(db_update);
				if(fb!=NULL)
					free(fb);
			}
		
		free(ffb);
		
		db_update->file_blocks[idx] = 0;
		fdb->num_entries -= 1;											//A. Aggiorno il numero di elementi all'interno della directory
		//printf("fdb->num_entries:%d\n",fdb->num_entries);
		
		int position;
		
		if(fdb->fcb.block_in_disk == db_update->index_block)
			position = get_position_disk_directory_block_first(db_update, disk);
		else
			position = get_position_disk_directory_block(db_update, disk);
			
		//A. Aggiorno il DirectoryBlock da cui ho tolto il file
		if(DiskDriver_updateBlock(disk, db_update, position, sizeof(DirectoryBlock)) == -1){
			fprintf(stderr, "Error in SimpleFS_remove: on update db_update.\n");
			free(db_update);
			return -1;
		}
		
		//A. Aggiorno il FirstDirectoryBlock da cui ho tolto il file
		if(DiskDriver_updateBlock(disk, fdb, fdb->fcb.block_in_disk, sizeof(FirstDirectoryBlock)) == -1){
			fprintf(stderr, "Error in SimpleFS_remove: on update db_update.\n");
			free(db_update);
			return -1;
		}
		
		free(db_update);
		
		d->dcb = fdb;												//A. Ripristino la directory che precedentemente ho scorso
		//printf("d->dcb->num_entries:%d\n",d->dcb->num_entries);
		//printf("\nOk\n");
		return 0;
		
	}
	//R. Caso in cui dobbiamo rimuovere una directory ricorsivamente
	else{
		FirstDirectoryBlock* fdb_to_remove = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
		if(fdb_to_remove == NULL){
			fprintf(stderr, "Error in SimpleFS_remove: malloc on fdb_to_remove.\n");
			free(ffb);
			free(db_update);
		}
		
		if(DiskDriver_readBlock(disk, fdb_to_remove, exist_pos, sizeof(FirstDirectoryBlock)) == -1){
			fprintf(stderr, "Error in SimpleFS_remove: read of fdb.\n");
			free(db_update);
			free(ffb);
			free(fdb_to_remove);
			return -1;
		}
		//A. La directory contiene elementi al suo interno
		if(fdb_to_remove->num_entries > 0){

			if(SimpleFS_changeDir(d, fdb_to_remove->fcb.name) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: change dir of fdb_to_remove.\n");
				free(db_update);
				free(fdb_to_remove);
				free(ffb);
			}
			
			//R. Estraggo il primo directory block
			DirectoryBlock* dir_up = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
			if(dir_up == NULL){
				fprintf(stderr,"Error in SimpleFS_remove: malloc on dir_up");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				return -1;
			}
			
			if(DiskDriver_readBlock(disk, dir_up, fdb_to_remove->index.blocks[0], sizeof(DirectoryBlock)) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: read of directory block one.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				free(dir_up);
				return -1;
			}
			//A. ricorsivamente elimino tutti i file
			while(dir_up != NULL){
				for(i=0; i<((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int)); i++){
					if(dir_up->file_blocks[i] != 0){
						if(DiskDriver_readBlock(disk, ffb, dir_up->file_blocks[i], sizeof(FirstFileBlock)) == -1){
							fprintf(stderr, "Error in SimpleFS_remove: read block FirstFileBlock.\n");
							free(db_update);
							free(ffb);
							free(fdb_to_remove);
							free(dir_up);
							return -1;
						}
						//printf("\nElimino: %s\n",ffb->fcb.name);
						SimpleFS_remove(d, ffb->fcb.name);
					}
				}
		
				if(fdb_to_remove->fcb.block_in_disk == dir_up->index_block)
					dir_up = get_next_block_directory_first(dir_up, disk);
				else
					dir_up = get_next_block_directory(dir_up, disk);
			}
			
			if(SimpleFS_changeDir(d, "..") == -1){
				fprintf(stderr, "Error in SimpleFS_remove: change dir of fdb_to_remove.\n");
				free(db_update);
				free(fdb_to_remove);
				free(ffb);
			}
			
			if(DiskDriver_freeBlock(disk, exist_pos) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: free block of fdb.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				if(dir_up != NULL)
					free(dir_up);
				return -1;
			}
			
			d->dcb->num_entries -=1;
			
			//fdb->num_entries -= 1;
			
			//d->dcb = fdb;
			
			db_update->file_blocks[idx] = 0;
			
			int position;
			
			if(fdb_to_remove->fcb.block_in_disk == db_update->index_block)
				position = get_position_disk_directory_block_first(db_update, disk);
			else
				position = get_position_disk_directory_block(db_update, disk);
			
			if(DiskDriver_updateBlock(disk, db_update, position, sizeof(DirectoryBlock)) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: on update db_update.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				if(dir_up != NULL)
					free(dir_up);
				return -1;
			}
			
			if(DiskDriver_updateBlock(disk, d->dcb, d->dcb->fcb.block_in_disk, sizeof(FirstDirectoryBlock)) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: on update db_update.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				if(dir_up != NULL)
					free(dir_up);
				return -1;
			}
			
			free(db_update);
			free(ffb);
			free(fdb_to_remove);
			if(dir_up != NULL)
				free(dir_up);
				
			return 0;
			
		}
		//A. La directory che sto eliminando non contiene nulla al suo interno
		else{
			if(DiskDriver_freeBlock(disk, exist_pos) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: free block of fdb.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				return -1;
			}
			
			fdb->num_entries -= 1;				//A. Aggiorno il numero di elementi all'interno della directory
			d->dcb = fdb; 						//A. ripristino la directory che precedentemente ho scorso
			db_update->file_blocks[idx] = 0;
			
			int position;
			
			if(fdb_to_remove->fcb.block_in_disk == db_update->index_block)
				position = get_position_disk_directory_block_first(db_update, disk);
			else
				position = get_position_disk_directory_block(db_update, disk);
			
			if(DiskDriver_updateBlock(disk, db_update, position, sizeof(DirectoryBlock)) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: on update db_update.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				return -1;
			}
			
			if(DiskDriver_updateBlock(disk, fdb, fdb->fcb.block_in_disk, sizeof(FirstDirectoryBlock)) == -1){
				fprintf(stderr, "Error in SimpleFS_remove: on update db_update.\n");
				free(db_update);
				free(ffb);
				free(fdb_to_remove);
				return -1;
			}
			
			free(fdb_to_remove);
			free(db_update);
			free(ffb);
			
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

FirstBlockIndex create_block_index_first(int previous){
	FirstBlockIndex index;
	index.previous = previous;		// Si inserisce il predecessore 
	index.next = -1;				// Essendo appena inizializzato, ancora non c'è un successivo blocco index, quello corrente non è pieno
	
	int i;
	for(i=0; i<MAX_BLOCKS_FIRST; i++){
		index.blocks[i] = -1;		// Tutti i blocchi vuoti settati a -1
	}
	return index;
}

//R. Funzione per ottenere il blocco index da un file
BlockIndex* get_block_index_file(FileBlock* file, DiskDriver* disk){
	BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
	if(DiskDriver_readBlock(disk, index, file->index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in get_block_index_file.\n");
			free(index);
			return NULL;
		}
	return index;
}

FirstBlockIndex* get_block_index_file_first(FileBlock* file, DiskDriver* disk){
	FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
	if(DiskDriver_readBlock(disk, index, file->index_block, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in get_block_index_file.\n");
			free(index);
			return NULL;
		}
	return index;
}

//A. Funzione per ottenere il blocco index da una directory
BlockIndex* get_block_index_directory(DirectoryBlock* directory, DiskDriver* disk){
	BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
	if(DiskDriver_readBlock(disk, index, directory->index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in get_block_index_directory.\n");
			free(index);
			return NULL;
		}
	return index;
}

FirstBlockIndex* get_block_index_directory_first(DirectoryBlock* directory, DiskDriver* disk){
	FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
	if(DiskDriver_readBlock(disk, index, directory->index_block, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in get_block_index_directory.\n");
			free(index);
			return NULL;
		}
	return index;
}

//R. Funzione che restituisce il blocco successivo file
FileBlock* get_next_block_file(FileBlock* file,DiskDriver* disk){
	BlockIndex* index = get_block_index_file(file,disk); //R. Estraggo il blocco index
	if(index == NULL){
		fprintf(stderr,"Error in get_next_block_file\n");
		free(file);
		return NULL;
	}
	
	int current_position = file->position; //R. posizione nell'array index
	 
	//R. Caso in cui devo andare nel blocco index successivo
	if((current_position + 1) == MAX_BLOCKS){
		if(index->next == -1){
			//fprintf(stderr,"Error in get next block file\n");
			free(index);
			free(file);
			return NULL;
		}
		BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, next, index->next, sizeof(BlockIndex)) == -1){
			//fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			free(next);
			free(file);
			return NULL;
		}
		FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
		if(DiskDriver_readBlock(disk, next_file, next->blocks[0], sizeof(FileBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			free(next);
			free(next_file);
			free(file);
			return NULL;
		}
		free(index);
		free(next);
		free(file);
		
		return next_file;
	}
	else{
	//R. Caso in cui mi trovo ancora nello stesso blocco index
	FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
	if(DiskDriver_readBlock(disk, next_file, index->blocks[current_position + 1], sizeof(FileBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block file\n");
			free(next_file);
			free(index);
			free(file);
			return NULL;
		}
	free(index);
	free(file);
	
	return next_file;
	}
}

//R. Funzione che restituisce il blocco successivo file
FileBlock* get_next_block_file_first(FileBlock* file,DiskDriver* disk){
	FirstBlockIndex* index = get_block_index_file_first(file,disk); //R. Estraggo il blocco index
	if(index == NULL){
		fprintf(stderr,"Error in get_next_block_file\n");
		free(file);
		return NULL;
	}
	
	int current_position = file->position; //R. posizione nell'array index
	 
	//R. Caso in cui devo andare nel blocco index successivo
	if((current_position + 1) == MAX_BLOCKS_FIRST){
		if(index->next == -1){
			//fprintf(stderr,"Error in get next block file\n");
			free(index);
			free(file);
			return NULL;
		}
		BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, next, index->next, sizeof(BlockIndex)) == -1){
			//fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			free(next);
			free(file);
			return NULL;
		}
		FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
		if(DiskDriver_readBlock(disk, next_file, next->blocks[0], sizeof(FileBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block file\n");
			free(index);
			free(next);
			free(next_file);
			free(file);
			return NULL;
		}
		free(index);
		free(next);
		free(file);
		
		return next_file;
	}
	else{
	//R. Caso in cui mi trovo ancora nello stesso blocco index
	FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
	if(DiskDriver_readBlock(disk, next_file, index->blocks[current_position + 1], sizeof(FileBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block file\n");
			free(next_file);
			free(index);
			free(file);
			return NULL;
		}
	free(index);
	free(file);
	
	return next_file;
	}
}

//A. Funzione che restituisce il blocco successivo directory
DirectoryBlock* get_next_block_directory(DirectoryBlock* directory,DiskDriver* disk){
	BlockIndex* index = get_block_index_directory(directory,disk); //A. Estraggo il blocco index
	if(index == NULL){
		//fprintf(stderr,"Error in get next block directory\n");
		free(directory);
		return NULL;
	}
	
	int current_position = directory->position; //A. posizione nell'array index
	 
	//R. Caso in cui devo andare nel blocco index successivo
	if((current_position + 1) == MAX_BLOCKS){
		if(index->next == -1){
			//fprintf(stderr,"Error in get next block directory\n");
			free(index);
			free(directory);
			return NULL;
		}
		BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, next, index->next, sizeof(BlockIndex)) == -1){
			//fprintf(stderr,"Errore nella get next block directory\n");
			free(next);
			free(index);
			free(directory);
			return NULL;
		}
		DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
		if(DiskDriver_readBlock(disk, next_directory, next->blocks[0], sizeof(DirectoryBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block directory\n");
			free(next_directory);
			free(index);
			free(next);
			free(directory);
			return NULL;
		}
		free(index);
		free(next);
		free(directory);
		
		return next_directory;
	}
	else{
	//A. Caso in cui mi trovo ancora nello stesso blocco index
	DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(DiskDriver_readBlock(disk, next_directory, index->blocks[current_position + 1], sizeof(DirectoryBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block directory\n");
			free(next_directory);
			free(index);
			free(directory);
			return NULL;
		}
	free(index);
	free(directory);
	
	return next_directory;
	}
}

DirectoryBlock* get_next_block_directory_first(DirectoryBlock* directory,DiskDriver* disk){
	FirstBlockIndex* index = get_block_index_directory_first(directory,disk); //A. Estraggo il blocco index
	if(index == NULL){
		//fprintf(stderr,"Error in get next block directory\n");
		free(directory);
		return NULL;
	}
	
	int current_position = directory->position; //A. posizione nell'array index
	 
	//R. Caso in cui devo andare nel blocco index successivo
	if((current_position + 1) == MAX_BLOCKS_FIRST){
		if(index->next == -1){
			//fprintf(stderr,"Error in get next block directory\n");
			free(index);
			free(directory);
			return NULL;
		}
		BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, next, index->next, sizeof(BlockIndex)) == -1){
			//fprintf(stderr,"Errore nella get next block directory\n");
			free(next);
			free(index);
			free(directory);
			return NULL;
		}
		DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
		if(DiskDriver_readBlock(disk, next_directory, next->blocks[0], sizeof(DirectoryBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block directory\n");
			free(next_directory);
			free(index);
			free(next);
			free(directory);
			return NULL;
		}
		free(index);
		free(next);
		free(directory);
		
		return next_directory;
	}
	else{
	//A. Caso in cui mi trovo ancora nello stesso blocco index
	DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(DiskDriver_readBlock(disk, next_directory, index->blocks[current_position + 1], sizeof(DirectoryBlock)) == -1){
			//fprintf(stderr,"Errore nella get next block directory\n");
			free(next_directory);
			free(index);
			free(directory);
			return NULL;
		}
	free(index);
	free(directory);
	
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
			fprintf(stderr,"Error in create_next_file_block: get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		//R. Ottengo il nuovo blocco libero per index
		int new_index_block = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(new_index_block == -1){
			fprintf(stderr,"Error in create_next_file_block: get free block\n");
			free(index);
			return -1;
		}
		
		//R. Ottengo il nuovo blocco libero per blocco
		int block_return = DiskDriver_getFreeBlock(disk, new_index_block + 1);
		if(block_return == -1){
			fprintf(stderr,"Error in create_next_file_block: get free block\n");
			free(index);
			return -1;
		}
		
		int index_block = current_block -> index_block; //R. Recupero il blocco index nel disk driver
		
		//R. Aggiorno il vecchio blocco index e lo riscrivo sul disco
		index->next = new_index_block;
		
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_file_block: update index block\n");
			free(index);
			return -1;
		}
		
		 //R. Creo il nuovo blocco index, lo aggiorno e lo scrivo sul blocco
		BlockIndex new_index = create_block_index(index_block);
		new_index.blocks[0] = block_return;
		if(DiskDriver_writeBlock(disk, &new_index, new_index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_file_block: write new index block\n");
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
			fprintf(stderr,"Error in create_next_file_block: get index block\n");
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
			fprintf(stderr,"Error in create_next_file_block: get free block\n");
			free(index);
			return -1;
		}
		
		//R. Aggiorno il blocco index e lo riscrivo sul disco
		index->blocks[new->position] = block_return;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_file_block: update index block\n");
			free(index);
			return -1;
		}
		
		free(index);
		
		return block_return;
	}

}

int create_next_file_block_first(FileBlock* current_block, FileBlock* new, DiskDriver* disk){
	int current_position_in_index = current_block -> position;
	
	if(current_position_in_index + 1 == MAX_BLOCKS_FIRST){
		//R. Caso in cui devo creare un nuovo blocco index e collegarlo
		FirstBlockIndex* index = get_block_index_file_first(current_block,disk); //R. Recupero il blocco index
		if(index == NULL){
			fprintf(stderr,"Error in create_next_file_block: get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		//R. Ottengo il nuovo blocco libero per index
		int new_index_block = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(new_index_block == -1){
			fprintf(stderr,"Error in create_next_file_block: get free block\n");
			free(index);
			return -1;
		}
		
		//R. Ottengo il nuovo blocco libero per blocco
		int block_return = DiskDriver_getFreeBlock(disk, new_index_block + 1);
		if(block_return == -1){
			fprintf(stderr,"Error in create_next_file_block: get free block\n");
			free(index);
			return -1;
		}
		
		int index_block = current_block -> index_block; //R. Recupero il blocco index nel disk driver
		
		//R. Aggiorno il vecchio blocco index e lo riscrivo sul disco
		index->next = new_index_block;
		
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_file_block: update index block\n");
			free(index);
			return -1;
		}
		
		 //R. Creo il nuovo blocco index, lo aggiorno e lo scrivo sul blocco
		BlockIndex new_index = create_block_index(index_block);
		new_index.blocks[0] = block_return;
		if(DiskDriver_writeBlock(disk, &new_index, new_index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_file_block: write new index block\n");
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
		
		FirstBlockIndex* index = get_block_index_file_first(current_block,disk); //R. Recupero il blocco index
		if(index == NULL){
			fprintf(stderr,"Error in create_next_file_block: get index block\n");
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
			fprintf(stderr,"Error in create_next_file_block: get free block\n");
			free(index);
			return -1;
		}
		
		//R. Aggiorno il blocco index e lo riscrivo sul disco
		index->blocks[new->position] = block_return;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_file_block: update index block\n");
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
			fprintf(stderr,"Error in create_next_directory_block: get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		//A. Ottengo il nuovo blocco libero per index
		int new_index_block = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(new_index_block == -1){
			fprintf(stderr,"Error in create_next_directory_block: get free block\n");
			free(index);
			return -1;
		}
		
		//A. Ottengo il nuovo blocco libero per blocco
		int block_return = DiskDriver_getFreeBlock(disk, new_index_block + 1);
		if(block_return == -1){
			fprintf(stderr,"Error in create_next_directory_block: get free block\n");
			free(index);
			return -1;
		}
		
		int index_block = current_block -> index_block; 
		
		//A. Aggiorno il vecchio blocco index e lo riscrivo sul disco
		index->next = new_index_block;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_directory_block: update index block\n");
			free(index);
			return -1;
		}
		
		//A. Creo il nuovo blocco index, lo aggiorno e lo scrivo sul blocco
		BlockIndex new_index = create_block_index(index_block);
		new_index.blocks[0] = block_return;
		if(DiskDriver_writeBlock(disk, &new_index, new_index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_directory_block: write new index block\n");
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
			fprintf(stderr,"Error in create_next_directory_block: get index block\n");
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
			fprintf(stderr,"Error in create_next_directory_block: get free block\n");
			free(index);
			return -1;
		}
		
		//A. Aggiorno il blocco index e lo riscrivo sul disco
		index->blocks[new->position] = block_return;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_directory_block: update index block\n");
			free(index);
			return -1;
		}
		
		free(index);
		
		return block_return;
	}

}

int create_next_directory_block_first(DirectoryBlock* current_block, DirectoryBlock* new, DiskDriver* disk){
	int current_position_in_index = current_block -> position;
	int i;
	
	//A. Caso in cui devo creare un nuovo blocco index e collegarlo
	if(current_position_in_index + 1 == MAX_BLOCKS_FIRST){	
		
		FirstBlockIndex* index = get_block_index_directory_first(current_block,disk); 
		if(index == NULL){
			fprintf(stderr,"Error in create_next_directory_block: get index block\n");
			return -1;
		}
		
		//print_index_block(index);
		
		//A. Ottengo il nuovo blocco libero per index
		int new_index_block = DiskDriver_getFreeBlock(disk, index->blocks[current_position_in_index]);
		if(new_index_block == -1){
			fprintf(stderr,"Error in create_next_directory_block: get free block\n");
			free(index);
			return -1;
		}
		
		//A. Ottengo il nuovo blocco libero per blocco
		int block_return = DiskDriver_getFreeBlock(disk, new_index_block + 1);
		if(block_return == -1){
			fprintf(stderr,"Error in create_next_directory_block: get free block\n");
			free(index);
			return -1;
		}
		
		int index_block = current_block -> index_block; 
		
		//A. Aggiorno il vecchio blocco index e lo riscrivo sul disco
		index->next = new_index_block;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_directory_block: update index block\n");
			free(index);
			return -1;
		}
		
		//A. Creo il nuovo blocco index, lo aggiorno e lo scrivo sul blocco
		BlockIndex new_index = create_block_index(index_block);
		new_index.blocks[0] = block_return;
		if(DiskDriver_writeBlock(disk, &new_index, new_index_block, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_directory_block: write new index block\n");
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
		FirstBlockIndex* index = get_block_index_directory_first(current_block,disk); 
		if(index == NULL){
			fprintf(stderr,"Error in create_next_directory_block: get index block\n");
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
			fprintf(stderr,"Error in create_next_directory_block: get free block\n");
			free(index);
			return -1;
		}
		
		//A. Aggiorno il blocco index e lo riscrivo sul disco
		index->blocks[new->position] = block_return;
		if(DiskDriver_updateBlock(disk, index, index_block, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in create_next_directory_block: update index block\n");
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
		fprintf(stderr,"Error in get_position_disk_file_block: could not read block index.\n");
		return -1;
	}
	return index.blocks[file_block->position];
}

int get_position_disk_file_block_first(FileBlock* file_block, DiskDriver* disk){
	FirstBlockIndex index;
	if(DiskDriver_readBlock(disk, &index, file_block->index_block, sizeof(FirstBlockIndex)) == -1){
		fprintf(stderr,"Error in get_position_disk_file_block: could not read block index.\n");
		return -1;
	}
	return index.blocks[file_block->position];
}

// Funzione per ottenere la posizione nel disco di un directory block
int get_position_disk_directory_block(DirectoryBlock* directory_block, DiskDriver* disk){
	BlockIndex index;
	if(DiskDriver_readBlock(disk, &index, directory_block->index_block, sizeof(BlockIndex)) == -1){
		fprintf(stderr,"Error in get_position_disk_directory_block: could not read block index.\n");
		return -1;
	}
	return index.blocks[directory_block->position];
}

int get_position_disk_directory_block_first(DirectoryBlock* directory_block, DiskDriver* disk){
	FirstBlockIndex index;
	if(DiskDriver_readBlock(disk, &index, directory_block->index_block, sizeof(FirstBlockIndex)) == -1){
		fprintf(stderr,"Error in get_position_disk_directory_block: could not read block index.\n");
		return -1;
	}
	return index.blocks[directory_block->position];
}

// Funzione per cercare se un file con nome elem_name è gia presente sul disco.
// Restituisce la posizione dell'elemento nel blocco directory in caso trovi il file sul disco, -1 in caso di errore, -2 in caso non lo trovi. 
int SimpleFS_already_exists(DiskDriver* disk, FirstDirectoryBlock* fdb, char* elem_name){
	if(disk == NULL || fdb == NULL || elem_name == NULL){
		fprintf(stderr, "Error in SImpleFS_already_exists_file: bad paremeters.\n");
		return -1;
	}
	
	//R. Estraggo il primo DirectoryBlock
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error in SimpleFS_already_exists: malloc on db.\n");
		return -1;
	}
	
	if(DiskDriver_readBlock(disk,db,fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_already_exists: could not read directory block one.\n");
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
			if(fdb->fcb.block_in_disk == db->index_block)
				db = get_next_block_directory_first(db,disk);
			else
				db = get_next_block_directory(db,disk);
			
			while(db != NULL){
				if(fdb->fcb.block_in_disk == db->index_block)
					pos_in_disk = get_position_disk_directory_block_first(db,disk);
				else
					pos_in_disk = get_position_disk_directory_block(db,disk);
				
				res = DiskDriver_readBlock(disk, &ffb_to_check, pos_in_disk, sizeof(FirstFileBlock));
				if(res == -1){
					fprintf(stderr, "Errore in SimpleFS_already_exists: could not DiskDriver_readBlock.\n");
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
				if(fdb->fcb.block_in_disk == db->index_block)
					db = get_next_block_directory_first(db,disk);
				else
					db = get_next_block_directory(db,disk);
			}
		}
	}
	free(db);
	return -2;
}

int SimpleFS_already_exists_remove(DiskDriver* disk, FirstDirectoryBlock* fdb, char* elem_name, DirectoryBlock* db_save, int* idx){
	if(disk == NULL || fdb == NULL || elem_name == NULL){
		fprintf(stderr, "Error in SImpleFS_already_exists_file: bad paremeters.\n");
		return -1;
	}
	
	//R. Estraggo il primo DirectoryBlock
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error in SimpleFS_already_exists: malloc on db.\n");
		return -1;
	}
	
	if(DiskDriver_readBlock(disk,db,fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_already_exists: could not read directory block one.\n");
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
					memcpy(db_save, db, sizeof(DirectoryBlock));
					*idx = i;
					free(db);
					return elem_pos;
				}
			}
		}

		//A. Se ci sono altri blocchi directory vanno controllati anche quelli
		if(fdb->num_entries > i){
			if(fdb->fcb.block_in_disk == db->index_block)
				db = get_next_block_directory_first(db,disk);
			else
				db = get_next_block_directory(db,disk);
			
			while(db != NULL){
				if(fdb->fcb.block_in_disk == db->index_block)
					pos_in_disk = get_position_disk_directory_block_first(db,disk);
				else
					pos_in_disk = get_position_disk_directory_block(db,disk);
				
				res = DiskDriver_readBlock(disk, &ffb_to_check, pos_in_disk, sizeof(FirstFileBlock));
				if(res == -1){
					fprintf(stderr, "Errore in SimpleFS_already_exists: could not DiskDriver_readBlock.\n");
					free(db);
					return -1;
				}
				
				for(i=0; i<dim; i++){
					if(db->file_blocks[i] > 0 && DiskDriver_readBlock(disk,&ffb_to_check,db->file_blocks[i], sizeof(FirstFileBlock)) != -1){
						if(strcmp(ffb_to_check.fcb.name,elem_name) == 0){
							//esiste già un elemento con lo stesso nome presente sul disco
							elem_pos = db->file_blocks[i];
							memcpy(db_save, db, sizeof(DirectoryBlock));
							*idx = i;
							free(db);
							return elem_pos;
						}
					}
				}
				if(fdb->fcb.block_in_disk == db->index_block)
					db = get_next_block_directory_first(db,disk);
				else
					db = get_next_block_directory(db,disk);
			}
		}
	}
	free(db);
	return -2;
}

int SimpleFS_assignDirectory(DiskDriver* disk, FirstDirectoryBlock* fdb, int pos_ffb, int pos_fb){
	if(disk == NULL || fdb == NULL || pos_ffb == -1 || pos_fb == -1){
		fprintf(stderr, "Error in SimpleFS_assignDirectory: bad parameters.\n");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		return -1;
	}
	
	int i, block_pos, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
	int found = 0;
	
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL){
		fprintf(stderr,"Error in SimpleFS_assignDirectory: malloc on db.\n");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		return -1;
	}
	
	//R. Estraggo il primo DirectoryBlock
	if(DiskDriver_readBlock(disk, db, fdb->index.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_assignDirectory: read directory block one on disk.\n");
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
		
		if(fdb->fcb.block_in_disk == db->index_block){
			db = get_next_block_directory_first(db,disk);}
		else{
			db = get_next_block_directory(db,disk);}
		
			
			while(db != NULL && found == 0){
				
				for(i=0; i<dim; i++){
					if(db->file_blocks[i] == 0){
						found = 1;
						block_pos = i;
						break;
					}
				}
				if(fdb->fcb.block_in_disk == db->index_block)
					db = get_next_block_directory_first(db,disk);
				else
					db = get_next_block_directory(db,disk);
			}
	}
	
	//A. Non c'era spazio nei precedenti blocchi directory. Devo usarne uno nuovo. Mi procuro un blocco libero e ci scrivo sopra il DirectoryBlock
	if(found == 0){
		DirectoryBlock* db_new = (DirectoryBlock*)malloc(sizeof(DirectoryBlock*));
		if(db_new == NULL){
			fprintf(stderr,"Error in SimpleFS_assignDirectory: malloc on db_new.\n");
			DiskDriver_freeBlock(disk, pos_ffb);
			DiskDriver_freeBlock(disk, pos_fb);
			free(db);
			return -1;
		}
		int pos;
		if(fdb->fcb.block_in_disk == db->index_block)
			pos = create_next_directory_block_first(db, db_new, disk);
		else
			pos = create_next_directory_block(db, db_new, disk);
		
		if(pos == -1){
			fprintf(stderr,"Error in SimpleFS_assignDirectory: crate next directory block.\n");
			DiskDriver_freeBlock(disk, pos_ffb);
			DiskDriver_freeBlock(disk, pos_fb);
			free(db);
			free(db_new);
			return -1;
		}
		db_new->file_blocks[0] = pos_ffb;
		
		if(DiskDriver_writeBlock(disk, db_new, pos, sizeof(DirectoryBlock)) == -1){
			fprintf(stderr,"Error in SimpleFS_assignDirectory: write block of db_new.\n");
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
		fprintf(stderr,"Error in SimpleFS_assignDirectory: updateBlock on fdb.\n");
		DiskDriver_freeBlock(disk, pos_ffb);
		DiskDriver_freeBlock(disk, pos_fb);
		free(db);
		return -1;
	}
	
	//A. E aggiorno il DirectoryBlock su cui ho scritto il file
	int pos_db;
	if(fdb->fcb.block_in_disk == db->index_block)
		pos_db = get_position_disk_directory_block_first(db,disk);
	else
		pos_db = get_position_disk_directory_block(db,disk);

	if(DiskDriver_updateBlock(disk, db, pos_db, sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Error in SimpleFS_assignDirectory: updateBlock on db.\n");
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

void print_index_block_first(FirstBlockIndex* index){
	printf("============ INDEX BLOCK ============\n");
	printf("Block_index_information:\n");
	printf("Previous: %d\n",index->previous);
	int i;
	for(i=0;i<MAX_BLOCKS_FIRST;i++){
		printf("Blocco %d: %d\n",i+1,index->blocks[i]);
	}
	printf("Next: %d\n",index->next);
	printf("=====================================\n");
}




  
