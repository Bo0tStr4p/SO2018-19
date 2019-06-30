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
	FirstDirectoryBlock first_directory_block = {};
	
	int res = DiskDriver_readBlock(disk,&first_directory_block,0, sizeof(FirstDirectoryBlock));
	if(res == -1){ 										//A. controllo che il blocco sia disponibile. Se non è disponibile, non possiamo andare avanti
		//printf("Blocco non disponibile\n");
		//free(first_directory_block);
		return NULL;
	};				
	
	
	
	DirectoryHandle* directory_handle = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));		//A. Il blocco è disponibile, quindi posso allocare la struttura
	directory_handle->sfs = fs;
	directory_handle->dcb = &first_directory_block;
	directory_handle->parent_dir = NULL;
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
	
	int ret = DiskDriver_writeBlock(fs->disk, &root_directory, 0, sizeof(FirstDirectoryBlock));		//A. vado a scrivere sul disco la root directory
	if (ret == -1){
		fprintf(stderr, "Errore nella format: impossibile formattare\n");
		//free(root_directory);
	}
	return;
}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename){
	if(d == NULL || filename == NULL){ 
		fprintf(stderr,"Errore nella create file: inseriti prametri non corretti\n");
		return NULL;
	}
	
	//A. Innanzitutto devo controllare che il file non sia già presente sul disco. Prima ci sono dei controlli
	SimpleFS* fs = d->sfs;
	DiskDriver* disk = fs->disk;                   
	FirstDirectoryBlock* fdb = d->dcb;
	if(fs == NULL || disk == NULL || fdb == NULL){ 
		fprintf(stderr,"Errore nella create file: la DirectoryHandle non è allocata bene\n");
		return NULL;
	}       
	
	//A. Prendiamo dal disco il primo blocco libero
	int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
	if(new_block == -1){
		fprintf(stderr, "Errore nella createFile: non ci sono blocchi liberi sul disco\n");
		return NULL;	//A. se non ci sono blocchi liberi sul disco interrompiamo
	}
	
	//A. creiamo il primo blocco del file
	FirstFileBlock* newfile = malloc(sizeof(FirstFileBlock));   
	newfile->index.previous = -1;
	newfile->index.next = -1;
	
	newfile->fcb.directory_block = fdb->fcb.block_in_disk;
	newfile->fcb.block_in_disk = new_block; 					//A. gli assegno il blocco libero sul disco ottenuto dalla getFreeBlock
	strcpy(newfile->fcb.name,filename);
	newfile->fcb.written_bytes = 0;
	newfile->fcb.size_in_bytes = 0;
	newfile->fcb.size_in_blocks = 0;
	newfile->fcb.is_dir = 0;
	
	//A. aggiorniamo lo spazio nella directory
	//fdb->file_blocks[fdb->num_entries] = new_block;			//A. eliminato a seguito del cambiamento fatti nelle struct
	fdb->num_entries++;
	
	//A. Scriviamo su disco il file
	int ret;
	ret = DiskDriver_writeBlock(disk,newfile,new_block, sizeof(FirstFileBlock));
	if(ret == -1){
		fprintf(stderr, "Errore nella createFile: impossibile scrivere sul disco");
		return NULL;
	}
	
	FileBlock* file_block = malloc(sizeof(FileBlock));
	file_block->index_block = 0;
	file_block->position = 0;
	//file_block->data = NULL;
	
	FileHandle* file_handle = malloc(sizeof(FileHandle));
	file_handle->sfs = d->sfs;
	file_handle->fcb = newfile;
	file_handle->directory = fdb;
	file_handle->current_block = file_block;
	file_handle->pos_in_file = 0;
	
	return file_handle;
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
	free(f->fcb);
	free(f->current_block);
	free(f);
	return 0;
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
	return 0;
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
		res = DiskDriver_readBlock(d->sfs->disk,parent_directory,parent_block,sizeof(FirstDirectoryBlock));
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
		//A. caso normale in cui mi sposto in una cartella contenuta nella cartella in cui mi trovo
		FirstDirectoryBlock* fdb = d->dcb;
		DirectoryBlock* db = d->current_block;
		DiskDriver* disk = d->sfs->disk;
		
		//A. Ci sono due sottocasi però: il primo, quello in cui la directory in cui mi sposto è nello stesso blocco indice della directory in cui sono
		FirstDirectoryBlock* dir_dest = malloc(sizeof(FirstDirectoryBlock));
		int i;
		int dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
		for(i = 0; i < dim; i++){
			if(db->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock))) != -1){
				if(strncmp(dir_dest->fcb.name,dirname) == 0){
					DiskDriver_readBlock(disk,dir_dest,db->file_blocks[i],sizeof(FirstDirectoryBlock));
					d->pos_in_block = 0; 
					d->parent_dir = fdb;
					d->dcb = dir_dest;
					return 0;
				}
			}
		}
		
		//A. Il secondo, quello in cui la directory in cui mi sposto è in un diverso blocco indice rispetto alla directory in cui sono
		int next_block = fdb->index.next;
		DirectoryBlock db;
		while(next_block != -1){
			res = DiskDriver_readBlock(disk,dir_dest,next_block,sizeof(FirstDirectoryBlock));
			if(res == -1){
				fprintf(stderr, "Errore in SimpleFS_changeDir: errore della readBlock\n");
				return -1;
			}
			for(i = 0; i < dim; i++){
				if(db.file_blocks[i] > 0 && (DiskDriver_readBlock(disk,&dir_dest,db.file_blocks[i],sizeof(FirstDirectoryBlock))) != -1){
					if(strcmp(dir_dest->fcb.name,dirname) == 0){
						DiskDriver_readBlock(disk,dir_dest,db.file_blocks[i],sizeof(FirstDirectoryBlock));
						d->pos_in_block = 0;
						d->directory = fdb;
						d->dcb = dir_dest;
						return 0;
					}
				}
			}
			next_block = db.index.next;
	}
	fprintf(stderr, "Errore: non si può cambiare directory\n");
	return -1;

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
  
