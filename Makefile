CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes
LIBS= 
CC=gcc
AR=ar

BINS= simplefs_test bitmap_test disk_driver_test

HEADERS=bitmap.h\
	disk_driver.h\
	simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 
	
bitmap_test: bitmap_test.o bitmap.o $(HEADERS)
				$(CC) $(CCOPTS) bitmap_test.c bitmap.c -o bitmap_test

disk_driver_test: disk_driver_test.o disk_driver.o $(HEADERS)
				$(CC) $(CCOPTS) disk_driver_test.c disk_driver.c -o disk_driver_test

simplefs_test: simplefs_test.o simplefs.o $(HEADERS)
				$(CC) $(CCOPTS) simplefs_test.c simplefs.c -o simplefs_test

clean:
	rm -rf *.o *~  $(BINS)
