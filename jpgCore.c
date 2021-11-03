#ifndef __JPEGCORE_C
#define __JPEGCORE_C

#include "stddef.h"

// Integer Transformation of floating point numbers using 8-bit precision

#define     C1  251         // 0.98079 * 2E8
#define     C2  237         // 0.92388 * 2E8
#define     C3  213         // 0.83147 * 2E8
#define     C4  181         // 0.70711 * 2E8
#define     C5  142         // 0.55557 * 2E8
#define     C6  98          // 0.38268 * 2E8
#define     C7  50          // 0.19509 * 2E8


uint8_t  deZigZagVector[64] = 
{
    0,  1,  8,  16, 9,  2,  3,  10,
    17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};


void setBlock(int *Block, int c)
{
uint8_t  i;


	for( i = 64 ; i ; )
	{
	Block[--i] = c;
	}

}


uint8_t bound(int value)
{

    if( value > 255 )
    {
        return 255;
    }
    
    else if( value < 0 )
    {
        return 0;
    }
    
    else
    {
        return (uint8_t)value;
    }
}


void RowIDCT(int *Row)
{
int   Sum[6];
int   Exp[8];


    Sum[0] = (int)(C4 * Row[0]) ;
    Sum[1] = (int)(C4 * Row[4]) ;
    Sum[2] = (int)(C2 * Row[2]) ;
    Sum[3] = (int)(C6 * Row[2]) ;
    Sum[4] = (int)(C2 * Row[6]) ;
    Sum[5] = (int)(C6 * Row[6]) ;
    
    Exp[0] = (int)(Sum[0] + Sum[2] + Sum[1] + Sum[5]) ;
    Exp[1] = (int)(Sum[0] + Sum[3] - Sum[1] - Sum[4]) ;
    Exp[2] = (int)(Sum[0] - Sum[3] - Sum[1] + Sum[4]) ;
    Exp[3] = (int)(Sum[0] - Sum[2] + Sum[1] - Sum[5]) ;
    
    Exp[4] = (int)(C1 * Row[1] + C3 * Row[3] + C5 * Row[5] + C7 * Row[7]) ;
    Exp[5] = (int)(C3 * Row[1] - C7 * Row[3] - C1 * Row[5] - C5 * Row[7]) ;
    Exp[6] = (int)(C5 * Row[1] - C1 * Row[3] + C7 * Row[5] + C3 * Row[7]) ;
    Exp[7] = (int)(C7 * Row[1] - C5 * Row[3] + C3 * Row[5] - C1 * Row[7]) ;
    
    Row[0] = (int)( (Exp[0] + Exp[4]) >> 1 );
    Row[1] = (int)( (Exp[1] + Exp[5]) >> 1 );
    Row[2] = (int)( (Exp[2] + Exp[6]) >> 1 );
    Row[3] = (int)( (Exp[3] + Exp[7]) >> 1 );
    Row[4] = (int)( (Exp[3] - Exp[7]) >> 1 );
    Row[5] = (int)( (Exp[2] - Exp[6]) >> 1 );
    Row[6] = (int)( (Exp[1] - Exp[5]) >> 1 );
    Row[7] = (int)( (Exp[0] - Exp[4]) >> 1 );    
       
}


void ColumnIDCT(int *Column)
{
int   Sum[6];
int   Exp[8];


    Sum[0] = (int)(C4 * Column[0])  ;
    Sum[1] = (int)(C4 * Column[32]) ;
    Sum[2] = (int)(C2 * Column[16]) ;
    Sum[3] = (int)(C6 * Column[16]) ;
    Sum[4] = (int)(C2 * Column[48]) ;
    Sum[5] = (int)(C6 * Column[48]) ;
    
    Exp[0] = (int)(Sum[0] + Sum[2] + Sum[1] + Sum[5]) ;
    Exp[1] = (int)(Sum[0] + Sum[3] - Sum[1] - Sum[4]) ;
    Exp[2] = (int)(Sum[0] - Sum[3] - Sum[1] + Sum[4]) ;
    Exp[3] = (int)(Sum[0] - Sum[2] + Sum[1] - Sum[5]) ;
    
    Exp[4] = (int)(C1 * Column[8] + C3 * Column[24] + C5 * Column[40] + C7 * Column[56]) ;
    Exp[5] = (int)(C3 * Column[8] - C7 * Column[24] - C1 * Column[40] - C5 * Column[56]) ;
    Exp[6] = (int)(C5 * Column[8] - C1 * Column[24] + C7 * Column[40] + C3 * Column[56]) ;
    Exp[7] = (int)(C7 * Column[8] - C5 * Column[24] + C3 * Column[40] - C1 * Column[56]) ;
    
    Column[0]  = (int)( (Exp[0] + Exp[4]) >> 1 );
    Column[8]  = (int)( (Exp[1] + Exp[5]) >> 1 );
    Column[16] = (int)( (Exp[2] + Exp[6]) >> 1 );
    Column[24] = (int)( (Exp[3] + Exp[7]) >> 1 );
    Column[32] = (int)( (Exp[3] - Exp[7]) >> 1 );
    Column[40] = (int)( (Exp[2] - Exp[6]) >> 1 );
    Column[48] = (int)( (Exp[1] - Exp[5]) >> 1 );
    Column[56] = (int)( (Exp[0] - Exp[4]) >> 1 );    
       
}


void performIDCT(int *Block8x8)
{
uint8_t     i;

 // First we perform 1D IDCT on each rows of a 8x8 block
    for( i = 0; i < 8; i++ )
    {
        RowIDCT( Block8x8 + (i << 3) );
    }
    
 // Then, we perform 1D IDCT on each column of the row transformed block
    for( i = 0; i < 8; i++ )
    {
        ColumnIDCT( Block8x8 + i );
    }
    
    for (i = 0; i < 64; i++ )
    {
        Block8x8[i] >>= 16;
        Block8x8[i]  += 128;
    }

}


/* To improve decoding performance, it is desirable to have different
 * YCbCr to XRGB transformation routines for different Sampling factors.
 * The key advantage of this is that each routine can be distinctly
 * optimized */
 
void Y4Cb1Cr1toXRGB(uint32_t * XRGB8x8Block, int * YBlock, int * CbBlock, int * CrBlock)
{
uint8_t     red, green, blue;
uint8_t     x, y;
uint8_t     i, j;
short       Y, Cb_, Cr_;
short       Exp1, Exp2, Exp3;


    for( y = 0, i = 0; y < 8; y++ )
    {
        for( x = 0, j = ( y & 0x06 ) << 2; x < 4; x++, j++ )
        {
         // Y is sampled every 1x1 pixel, Cb and Cr is sampled every 2x2 pixels
         
            Y       =   YBlock[i];
            Cb_     =   CbBlock[j] - 128 ;
            Cr_     =   CrBlock[j] - 128;
            
            Exp1    =   45 * Cr_ / 32;
            Exp2    =   (11  * Cb_ + 23 * Cr_) / 32; 
            Exp3    =   113 * Cb_ / 64;

            red     = bound(Y + Exp1);
            green   = bound(Y - Exp2);
            blue    = bound(Y + Exp3);
            
            XRGB8x8Block[i++] = (red << 16) + (green << 8) + blue;
            
            Y       = YBlock[i];
               
            red     = bound(Y + Exp1);
            green   = bound(Y - Exp2);
            blue    = bound(Y + Exp3);
            
            XRGB8x8Block[i++] = (red << 16) + (green << 8) + blue;
        }
    }

}


void Y2Cb1Cr1toXRGB(uint32_t * XRGB8x8Block, int * YBlock, int * CbBlock, int * CrBlock)
{
uint8_t     red, green, blue;
uint8_t     i, j;
short       Y, Cb_, Cr_;
short       Exp1, Exp2, Exp3;


    for( i = 0, j = 0; i < 64; j++ )
    { 
     // Y is sampled every 2x1 pixels, Cb and Cr are sampled every 1x1 pixel
      
        Y       =   YBlock[i];
        Cb_     =   CbBlock[j] - 128;
        Cr_     =   CrBlock[j] - 128;
        
        Exp1    =   45 * Cr_ / 32;
        Exp2    =   (11  * Cb_ + 23 * Cr_) / 32; 
        Exp3    =   113 * Cb_ / 64;

        red     = bound(Y + Exp1);
        green   = bound(Y - Exp2);
        blue    = bound(Y + Exp3);
            
        XRGB8x8Block[i++] = (red << 16) + (green << 8) + blue;
        
        Y       =   YBlock[i];
           
        red     = bound(Y + Exp1);
        green   = bound(Y - Exp2);
        blue    = bound(Y + Exp3);
            
        XRGB8x8Block[i++] = (red << 16) + (green << 8) + blue;
    }

}


void Y1Cb1Cr1toXRGB(uint32_t * XRGB8x8Block, int * YBlock, int * CbBlock, int * CrBlock)
{
uint8_t     red, green, blue;
uint8_t     i;
short       Y, Cb_, Cr_;
short       Exp1, Exp2, Exp3;


    for( i = 0; i < 64; i++ )
    { 
     // Each of Y, Cb and Cr are sampled over 1x1 pixel
      
        Y       =   YBlock[i];
        Cb_     =   CbBlock[i] - 128 ;
        Cr_     =   CrBlock[i] - 128;
           
        Exp1    =   45 * Cr_ / 32;
        Exp2    =   (11  * Cb_ + 23 * Cr_) / 32; 
        Exp3    =   113 * Cb_ / 64;

        red     = bound(Y + Exp1);
        green   = bound(Y - Exp2);
        blue    = bound(Y + Exp3);
            
        XRGB8x8Block[i] = (red << 16) + (green << 8) + blue;
    }

}


#endif
