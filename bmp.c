#ifndef  __BITMAP_C
#define  __BITMAP_C

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>


typedef struct __attribute__((packed))
{                 
    uint8_t     Header[2];          // the header field ('BM' for a valid bitmap)
    uint32_t    FileSize;           // the size of BMP file in bytes
    uint16_t    Reserved1;          // reserved
    uint16_t    Reserved2;          // reserved
    uint32_t    DataOffset;         // the offset to the bitmap data
}   
BITMAP_FILE_HEADER;


typedef struct __attribute__((packed))
{
    uint32_t    HeaderSize;         // the size of the bitmap info header (40 bytes)
    int         ImageWidth;         // the bitmap width in pixels
    int         ImageHeight;        // the bitmap height in pixels
    uint16_t    nColorPlanes;       // No. of color planes being used (Must be 1)
    uint16_t    BitsPerPixel;       // Color depth of the image (bits per pixel)
    uint32_t    Compression;        // the compression method being used
    uint32_t    RawDataSize;        // the size of raw bitmap data ( 0 for BI_RGB Bitmaps )
    int         HRes;               // the horizontal resolution of the image (pixel per meter)
    int         VRes;               // the vertical resolution of the image (pixel per meter)
    uint32_t    nColors;            // the no. of colors in the color pallette
    uint32_t    nImpColors;         // the no. of important colors being used (generally ignored)
}
DIB_HEADER;


void bmp_write(char * BMPFile, uint32_t * RawImage, uint16_t ImageWidth, uint16_t ImageHeight)
{
BITMAP_FILE_HEADER      BMFH;
DIB_HEADER              DIBH;
FILE *                  fp;


    BMFH.Header[0]      =   'B';
    BMFH.Header[1]      =   'M';
    BMFH.FileSize       =   sizeof(BITMAP_FILE_HEADER) + sizeof(DIB_HEADER) + 4 * ImageWidth * ImageHeight;
    BMFH.DataOffset     =   sizeof(BITMAP_FILE_HEADER) + sizeof(DIB_HEADER);
    
    DIBH.HeaderSize     =   sizeof(DIB_HEADER);
    DIBH.ImageWidth     =   ImageWidth;
    DIBH.ImageHeight    =  -ImageHeight;
    DIBH.nColorPlanes   =   1;
    DIBH.BitsPerPixel   =   32;
    DIBH.Compression    =   0;
    DIBH.RawDataSize    =   0;
    DIBH.HRes           =   0x355C;
    DIBH.VRes           =   0x355C;
    DIBH.nColors        =   0;
    DIBH.nImpColors     =   0;
    
    
    fp = fopen(BMPFile, "wb");
    
    fwrite( &BMFH, 1, sizeof(BITMAP_FILE_HEADER), fp) ;
    fwrite( &DIBH, 1, sizeof(DIB_HEADER), fp );
    
    fwrite( RawImage, 4, ImageWidth * ImageHeight, fp) ;
    
    fclose(fp);
    
}



#endif
