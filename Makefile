CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes
LIBS= 
CC=gcc
AR=ar

BINS= simplefs_test bitmap_test disk_driver_test index_test

HEADERS=bitmap.h\
	disk_driver.h\
	simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 
	
bitmap_test: bitmap_test.o bitmap.o $(HEADERS)
				$(CC) $(CCOPTS) bitmap_test.c bitmap.c -o bitmap_test

disk_driver_test: disk_driver_test.o disk_driver.o bitmap.o $(HEADERS)
				$(CC) $(CCOPTS) bitmap.c disk_driver_test.c disk_driver.c -o disk_driver_test

simplefs_test: simplefs_test.o simplefs.o disk_driver.o bitmap.o $(HEADERS)
				$(CC) $(CCOPTS) disk_driver.c bitmap.c simplefs_test.c simplefs.c -o simplefs_test

index_test: index_test.o simplefs.o disk_driver.o bitmap.o $(HEADERS)
				$(CC) $(CCOPTS) disk_driver.c bitmap.c index_test.c simplefs.c -o index_test

clean:
	rm -rf *.txt *.o *~  $(BINS)
