#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "jpg.h"
#include "bmp.c"


int main(int argc, char *argv[])
{
jpg_t *     jpg;
uint32_t *  buf;

    if( argc != 2 ) {
      printf("Usage: jpg2bmp <jpgfile>\n");
      return -1;
    }
    
    jpg = jpg_open(argv[1]);
    if( jpg == NULL ) {
      printf("\nError: jpg_open(%s)\n", argv[1]);
      return 1;
    }
    
    buf = malloc(jpg->width * jpg->height * 4);
    jpg_read(buf, jpg->width, jpg->height, jpg);
    bmp_write("output.bmp", buf, jpg->width, jpg->height);
    jpg_close(jpg);
    free(buf);
    
    return 0;
}
