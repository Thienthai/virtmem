/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int *fr;
int framenum = 0;
struct disk *mydisk;

void page_fault_handler( struct page_table *pt, int page )
{
        time_t t;
        srand((unsigned) time(&t));
        int FreeFrame = page_table_get_nframes(pt);
        int frame = 0;
        int bits = 0;
        page_table_get_entry(pt,page,&frame,&bits);
        if(bits == 0){
            if(framenum == FreeFrame){
                int pageRand = rand() % page_table_get_npages(pt);
                frame = 0;
                bits = 0;
                page_table_get_entry(pt,pageRand,&frame,&bits);
                while(bits == 0 || pageRand == page){
                    pageRand = rand() % page_table_get_npages(pt);
                    page_table_get_entry(pt,pageRand,&frame,&bits);
                }
                if(bits == 3){
                    disk_write(mydisk,pageRand,&page_table_get_physmem(pt)[frame*PAGE_SIZE]);
                    disk_read(mydisk,page,&page_table_get_physmem(pt)[frame*PAGE_SIZE]);
                    page_table_set_entry(pt,page,frame,PROT_READ);
                    page_table_set_entry(pt,pageRand,0,0);
                }else{
                    disk_read(mydisk,page,&page_table_get_physmem(pt)[frame*PAGE_SIZE]);
                    page_table_set_entry(pt,page,frame,PROT_READ);
                    page_table_set_entry(pt,pageRand,0,0);
                }
            }else{
                int i = 0;
                while(fr[i] != 0){
                    i = rand() % page_table_get_nframes(pt);
                }
                page_table_set_entry(pt,page,i, PROT_READ );
                disk_read(mydisk,page,&page_table_get_physmem(pt)[i*PAGE_SIZE]);
                framenum += 1;
                fr[i] = 1;
            }
        }else{
            page_table_get_entry(pt,page,&frame,&bits);
            page_table_set_entry(pt,page,frame,PROT_READ | PROT_WRITE);
        }
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|lru> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	const char *program = argv[4];

	struct disk *disk = disk_open("myvirtualdisk",npages);
        mydisk = disk;
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
        
        fr = (int *) calloc(page_table_get_nframes(pt),sizeof(int));

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);

	}
        //page_table_print(pt);
	page_table_delete(pt);
	disk_close(disk);
        free(fr);

	return 0;
}
