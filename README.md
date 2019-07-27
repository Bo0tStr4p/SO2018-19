# Progetto Sistemi Operativi 2018/2019

## Progetto:
- File System con inode
## Componenti:
- Alessandro Appolloni
- Romeo Bertoldo

Il File-system è una parte del Sistema Operativo che si occupa di gestire e
strutturare le informazioni memorizzate su supporti permanenti. Attraverso il File
System il Sistema Operativo fornisce una visione astratta dei file su disco e permette
all’utente di accedervi effettuando operazioni ad alto livello.

Questo File System è stato realizzato con allocazione indicizzata dello spazio su
disco (Indexed Allocation). Il file system ha dei blocchi chiamati Index Block che
hanno il compito di far accedere in maniera diretta ai blocchi del file o della directory
in questione. Nel nostro caso è stato scelto il valore di 10 puntatori per ogni blocco
index. Inoltre, nel caso di esaurimento dei puntatori, il blocco index punta ad un
successivo blocco index con altri 10 puntatori, continuando a cascata.
Il disco è simulato con un file txt (chiamato file_system.txt) diviso in blocchi di
grandezza fissa (512 Byte) gestito con il supporto di una bitmap e driver di disco.

Per ulteriori informazioni leggere [Report.pdf](https://github.com/Bo0tStr4p/SO2018-19/blob/master/Report.pdf)
