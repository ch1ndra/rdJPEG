/* JPEG decoder for baseline-DCT based JPEG images. Copyright(C), Ch1ndra
 * This software is licensed under Creative Commons Attribution License.
 * For usage, modification and distribution permission, refer to the
 * accompanying License */
 
#include  "stddef.h"
#include  "stdio.h"
#include  "jpg.h"
#include  "stdlib.h"
#include  "memory.h"
#include  "jpgCore.c"

static    uint16_t readMarker(jpg_t * jpg);
static    uint8_t  validateJPEG(jpg_t * jpg);
static    void     readAPP0(jpg_t * jpg);
static    void     readSOF(jpg_t * jpg);
static    void     readDQT(jpg_t * jpg);
static    void     HUFFTREE_create(jpg_t * jpg, struct NODE *root);
static    void     HUFFTREE_insertLeaf( uint8_t symbol, 
                                     uint16_t codeWord, 
                                     uint8_t codeLength, 
                                     struct NODE *root
                                    );
static    uint8_t  HUFFTREE_readSymbol(jpg_t * jpg, struct NODE *root);
static    void     HUFFTREE_destroy(struct NODE *root);
static    void     readDHT(jpg_t * jpg);
static    void     readSOS(jpg_t * jpg);
static    void     readDRI(jpg_t * jpg);
static    FILE *   skipSegment(jpg_t * jpg);
static    uint8_t  readBitStream(jpg_t * jpg);
static    int      readCoefficient(jpg_t * jpg, uint8_t category);
static    void     resetDecoder(jpg_t * jpg);
static    void     decodeYDU(jpg_t * jpg, int *coeffTbl);
static    void     decodeCbDU(jpg_t * jpg, int *coeffTbl);
static    void     decodeCrDU(jpg_t * jpg, int *coeffTbl);
static    uint32_t * decodeScanData(jpg_t * jpg);
static    void     writeBlock( uint32_t * dest, 
                               uint16_t x, 
                               uint16_t y,
                               uint32_t * RGB8x8Block, 
                               uint16_t imageWidth
                             );

#define    __Y__       1
#define    __Cb__      2
#define    __Cr__      3


// JPEG stores information in its header in big-endian format
// Hence, it is necessary to accomodate conversion to comply with little-endian architecture

uint16_t toSmallEndian(uint16_t big_endian)
{    
    return( ( (big_endian & 0xFF) << 8 ) + (big_endian >> 8) );
}


// All segments appearing in the JPEG file start with a marker identifying the segment

static uint16_t readMarker(jpg_t * jpg)
{
uint16_t    marker;


    fread( &marker, sizeof(marker), 1, jpg->fp);
    fseek(jpg->fp, -sizeof(marker), SEEK_CUR);
    
    if( (marker & 0xFF) != 0xFF )
        return NOM;
        
return toSmallEndian(marker);

}


static uint8_t validateJPEG(jpg_t * jpg)
{
uint16_t    firstMarker;

 // Make sure we're at the beginning of the file 
    rewind(jpg->fp);
    
    firstMarker = readMarker(jpg);
    if( firstMarker != SOI )
        return 0;
    
 // Skip SOI Marker and position the file pointer to the next segment
    fseek(jpg->fp, 2, SEEK_CUR);                                             
    
return 1;

}


static void readAPP0(jpg_t * jpg)
{
    
    fread(&jpg->seg.app0, 1, sizeof(jpg->seg.app0), jpg->fp);
    jpg->seg.app0.length = toSmallEndian(jpg->seg.app0.length);
    
 // Position the file pointer to the next segment
    fseek(jpg->fp, jpg->seg.app0.length + sizeof(jpg->seg.app0.APP0_marker) - sizeof(jpg->seg.app0), SEEK_CUR);
    
}


static void readSOF(jpg_t * jpg)
{
uint8_t  i;
    
    
    fread(&jpg->seg.sof, 1, sizeof(jpg->seg.sof), jpg->fp);
    
    jpg->seg.sof.length         =   toSmallEndian(jpg->seg.sof.length);
    jpg->seg.sof.frameWidth     =   toSmallEndian(jpg->seg.sof.frameWidth);
    jpg->seg.sof.frameHeight    =   toSmallEndian(jpg->seg.sof.frameHeight);
    
    fprintf(stdout, "\nImage Width: %d", jpg->seg.sof.frameWidth);
    fprintf(stdout, "\nImage Height: %d", jpg->seg.sof.frameHeight);
    fprintf(stdout, "\nNumber of Image Components: %d", jpg->seg.sof.nComponents);
                    
    for( i = 0; i < jpg->seg.sof.nComponents; i++ )
    {
        switch( jpg->seg.sof.FCSFstruct[i].ID )
        {
            case __Y__:
            {
                jpg->seg.Y.ID           =  jpg->seg.sof.FCSFstruct[i].ID;
                jpg->seg.Y.HSmplFctr    = (jpg->seg.sof.FCSFstruct[i].SmplFctr >> 4) & 0xF;
                jpg->seg.Y.VSmplFctr    = (jpg->seg.sof.FCSFstruct[i].SmplFctr) & 0xF;
                jpg->seg.Y.QntzTbl      = &jpg->seg.dqt[jpg->seg.sof.FCSFstruct[i].QntzTblN].QntzTbl[0][0];
                
                break;
            }
            
            case __Cb__:
            {
                jpg->seg.Cb.ID          =  jpg->seg.sof.FCSFstruct[i].ID;
                jpg->seg.Cb.HSmplFctr   = (jpg->seg.sof.FCSFstruct[i].SmplFctr >> 4) & 0xF;
                jpg->seg.Cb.VSmplFctr   = (jpg->seg.sof.FCSFstruct[i].SmplFctr) & 0xF;
                jpg->seg.Cb.QntzTbl     = &jpg->seg.dqt[jpg->seg.sof.FCSFstruct[i].QntzTblN].QntzTbl[0][0];
                
                break;
            }
            
            case __Cr__:
            {
                jpg->seg.Cr.ID          =  jpg->seg.sof.FCSFstruct[i].ID;
                jpg->seg.Cr.HSmplFctr   = (jpg->seg.sof.FCSFstruct[i].SmplFctr >> 4) & 0xF;
                jpg->seg.Cr.VSmplFctr   = (jpg->seg.sof.FCSFstruct[i].SmplFctr) & 0xF;
                jpg->seg.Cr.QntzTbl     = &jpg->seg.dqt[jpg->seg.sof.FCSFstruct[i].QntzTblN].QntzTbl[0][0];
                
                break;
            }
        }
    }
    
    fprintf(stdout, "\nChroma subsampling: %dx%d", jpg->seg.Y.HSmplFctr, jpg->seg.Y.VSmplFctr);    
    
 // Position the file pointer to the next segment 
    fseek(jpg->fp, jpg->seg.sof.length + sizeof(jpg->seg.sof.SOF_marker) - sizeof(jpg->seg.sof), SEEK_CUR);
    
}


static void readDQT(jpg_t * jpg)
{
DQTseg   dqt;
uint8_t  QTcount;
uint8_t  id;

 /* It is important to know that the Quantization Table Identifier
  * doesnot necessarily identify the component to which this
  * Quantization Table corresponds. The only way to tell which
  * is which is after reading Quantization Table Number for Y, Cb
  * and Cr components from the Frame Component Specification */
     
    fread(&dqt, 1, 4, jpg->fp);    
    dqt.length = toSmallEndian(dqt.length);
    
 // Determine number of Quantization Tables defined in this segment 
    QTcount = dqt.length >> 6;

    /* For now, we will assume that the QTID = 0 corresponds to the Luminance
     * and QTID = 1 corresponds to the Chrominance, which is usually the case.
     * We will make adjustments (if required) after we read the SOF segment */
    
    for( ; QTcount; QTcount-- )
    {
        fread(&id, 1, 1, jpg->fp );

        switch(id)
        {
            case 0:
                fread(jpg->seg.dqt[0].QntzTbl, 1, 64, jpg->fp);
            break;

            case 1:
                fread(jpg->seg.dqt[1].QntzTbl, 1, 64, jpg->fp);
            break;

            default:
                fprintf(stdout, "\nreadDQT(): Error! [Unknown identifier for the Quantization Table]");
            break;
        }
    }
    
 // At this point, the file pointer is right at the next segment
    
}


static void HUFFTREE_create(jpg_t * jpg, struct NODE *root)
{
uint8_t     i, j;
uint8_t     symbol;
uint16_t    codeWord = 0;


    memset( root, 0, sizeof(struct NODE) );
    root->parent = root;
    
    for( i = 1; i <= 16; i++ )
    {
        for( j = 1; j <= jpg->seg.dht.huffCodefreq[i-1]; j++ )
        {
            fread( &symbol, 1, 1, jpg->fp);
            HUFFTREE_insertLeaf( symbol, codeWord, i, root);
            codeWord++;
        }
        codeWord = codeWord << 1;
    }
}


static void HUFFTREE_insertLeaf(uint8_t symbol, uint16_t codeWord, uint8_t codeLength, struct NODE *root)
{
struct NODE * volatile node;
struct NODE * volatile newNode;
uint8_t       bit;

 
 // Start tree traversal from the root node
    node = root;
    
    while( codeLength )
    {
     // In each iteration, read a bit from the codeword and traverse the tree accordingly
        bit = (codeWord >> --codeLength) & 1;
        
        if( bit == 0 )
        {
         // If no left node exists, create a new node
            if( node->leftChild == 0 )
            {
                newNode = (struct NODE *)malloc( sizeof(struct NODE) );
                memset( newNode, 0, sizeof(struct NODE) );
                node->leftChild = newNode;
                newNode->parent = node;
            }
            node = node->leftChild;
        }
        
        else
        {
         // If no right node exists, create a new node
            if( node->rightChild == 0 )
            {
                newNode = (struct NODE *)malloc( sizeof(struct NODE) );
                memset( newNode, 0, sizeof(struct NODE) );
                node->rightChild = newNode;
                newNode->parent = node;
            }
            node = node->rightChild;
        }
    }
    
 // At the end of the tree traversal, we're at the leaf, so mark this leaf with input symbol
    node->isLeaf     = 1;
    node->leafSymbol = symbol;
    node->leftChild  = NULL;
    node->rightChild = NULL;
    
}


static uint8_t HUFFTREE_readSymbol(jpg_t * jpg, struct NODE *root)
{
struct NODE *  node;
uint8_t        bit;


 // Start tree traversal from the root node
    node = root;
    
    while( ! node->isLeaf )
    {
     // In each iteration, read a bit from the stream and traverse the tree accordingly
        bit = readBitStream(jpg);
        
        if( bit == 0 )
            node = node->leftChild;
        
        else
            node = node->rightChild;    
     }
    
 // At the end of the tree traversal, we're at the leaf which has the symbol w're looking for
    return(node->leafSymbol);
    
}


void HUFFTREE_destroy(struct NODE *root)
{
struct NODE * node;
struct NODE * parentNode;


    if( !(node = root) )
        return;
    
    while(root->rightChild||root->leftChild)
    {
        if(node->leftChild)
        {
            node = node->leftChild;
        }
            
        else if(node->rightChild)
        {
            node = node->rightChild;
        }               
                
        else
        {
            parentNode = node->parent;
            
            if( node == parentNode->leftChild )
                parentNode->leftChild = 0;
                
            else
                parentNode->rightChild = 0;
                
            free(node);
            node = parentNode;
        }
    }
}


static void readDHT(jpg_t * jpg)
{
uint8_t     ncodeWords = 0;
uint8_t     i;


    fread(&jpg->seg.dht, 1, 4, jpg->fp);
    jpg->seg.dht.length = toSmallEndian(jpg->seg.dht.length); 
    
    do
    {  
           
        fread( &jpg->seg.dht.CLASS_ID, 1, 17, jpg->fp );
        
     // To determine total number of codewords, all we need to do is to sum-up frequencies for each codeword of length i    
        for( i = 0; i < 16; i++ )
            ncodeWords += jpg->seg.dht.huffCodefreq[i];
        
     /* Just like the Quantization Table Identifier, the Huffman Table 
      * Identifier doesnot necessarily identify the component to which 
      * this  Huffman table corresponds. The only way to tell which is
      * whose is after reading SOS segment */
      
        switch( jpg->seg.dht.CLASS_ID )                
        {
        // The high nibble represents CLASS( 0 = DC and 1 = AC) and low nibble represents ID
        
            case 0x00:
            {
                fprintf(stdout, "\nReading Huffman Code Value for Luminance DC...");
                            
             // Start creating Huffman tree for this Component Class
                HUFFTREE_create( jpg, &jpg->seg.Y.HuffTreeDC );
                break;
            }
            
            case 0x01:
            {
                fprintf(stdout, "\nReading Huffman Code Value for Chrominance DC...");
                
             // Start creating Huffman tree for this Component Class
                HUFFTREE_create( jpg, &jpg->seg.Cb.HuffTreeDC );
                jpg->seg.Cr.HuffTreeDC = jpg->seg.Cb.HuffTreeDC;
                break;
            }
            
            case 0x10:
            {
                fprintf(stdout, "\nReading Huffman Code Value for Luminance AC...");
                
             // Start creating Huffman tree for this Component Class
                HUFFTREE_create( jpg, &jpg->seg.Y.HuffTreeAC );
                break;
            }
            
            case 0x11:
            {
                fprintf(stdout, "\nReading Huffman Code Value for Chrominance AC...");
                
             // Start creating Huffman tree for this Component Class
                HUFFTREE_create( jpg, &jpg->seg.Cb.HuffTreeAC );
                jpg->seg.Cr.HuffTreeAC = jpg->seg.Cb.HuffTreeAC;
                break;
            }
        }
        
    } while( !readMarker(jpg) );

 // By now, file pointer is right at the next segment
    
}


static void readSOS(jpg_t * jpg)
{
    
    fread(&jpg->seg.sos, 1, sizeof(jpg->seg.sos), jpg->fp);    
    jpg->seg.sos.length = toSmallEndian(jpg->seg.sos.length);
        
 // Skip the Scan Header and position the file pointer to the Scan data 
    fseek(jpg->fp, jpg->seg.sos.length + sizeof(jpg->seg.sos.SOS_marker) - sizeof(jpg->seg.sos), SEEK_CUR);
    
}


static void readDRI(jpg_t * jpg)
{
    
    fread(&jpg->seg.dri, 1, sizeof(jpg->seg.dri), jpg->fp);    
    jpg->seg.dri.length = toSmallEndian(jpg->seg.dri.length);    
    jpg->seg.dri.nMCUs  = toSmallEndian(jpg->seg.dri.nMCUs);
    
    fprintf(stdout, "\nNumber of MCUs in the Restart Interval: %d", jpg->seg.dri.nMCUs);
    
 // By now, file pointer is right at the next segment
 
}


static FILE * skipSegment(jpg_t * jpg)
{
uint16_t    segMarker;
uint16_t    segLength;


    fread(&segMarker, sizeof(segMarker), 1, jpg->fp);
    fread(&segLength, sizeof(segLength), 1, jpg->fp);    
    fseek(jpg->fp, toSmallEndian(segLength) - sizeof(segLength), SEEK_CUR);

  return jpg->fp;
}


static uint8_t readBitStream(jpg_t * jpg)
{
uint8_t shift;

    jpg->stream.index %= 8;     /* Wrap around */
    
    if(jpg->stream.index == 0)
    {
        /* Read the next byte from the stream */
        fread(&jpg->stream._byte, 1, 1, jpg->fp);

        /* JPEG standard specifies that the RST marker (0xFFD0 to 0xFFD7) 
        * may be encoded within the Scan Data for synchronization. As such,
        * if the encoder needs to write 0xff byte as a part of encoded MCU
        * data, it needs to write an additional stuff byte(0x00) to ensure
        * that this byte(0xff) is a part of MCU data and not a marker.
        * Hence, it is necessary to skip stuffed byte(if any). As a part of
        * generic implementation, it is also desirable to skip RST marker
        * (if any) */
      
        while( jpg->stream._byte == 0xFF )
        {
         // Read next byte from the stream 
            fread( &jpg->stream._byte, 1, 1, jpg->fp);
            
         // Check whether the byte that follows defines a RST marker 
            if( (jpg->stream._byte >> 4) == 0x0D )
            {
             // In this case, simply read another byte from the stream
                fread( &jpg->stream._byte, 1, 1, jpg->fp);
            }
          
            else
            {
             // Ignore the stuffed byte and restore the preceeding byte
                jpg->stream._byte = 0xFF;
                
             // The last fread() sets the file pointer right past the stuffed byte so, we're done
                break;
            }
        }    
    }

    shift = 7 - jpg->stream.index;
    jpg->stream.index++;
    
    return( (jpg->stream._byte >> shift) & 1 );
}


static int readCoefficient(jpg_t * jpg, uint8_t category)
{
int  coeff = 0;

    if( !category)
    {
        return 0;
    }
    
    coeff = readBitStream(jpg);
    
 /* If the bit-string for the coefficient starts with 1, it suggests
  * that the coefficient is positive and stored as an unsigned 
  * representation. Otherwise, the coefficient is negative and stored
  * with its bits complemented */
    
    if( !coeff )
    {
     // Complement the first bit in the bit-string
        coeff = 1;
        
        while( --category )
        {
            coeff = coeff << 1;
            coeff |= !readBitStream(jpg);
        }
        return -coeff;
    }
    
    else
    {        
        while( --category )
        {
            coeff = coeff << 1;
            coeff |= readBitStream(jpg);
        }
        return coeff;
    }        

}


static void resetDecoder(jpg_t * jpg)
{
    jpg->seg.Y.DCcoeff = 0;
    jpg->seg.Cb.DCcoeff = 0;
    jpg->seg.Cr.DCcoeff = 0;
    jpg->stream.index = 0;
}
    

static void decodeYDU(jpg_t * jpg, int * coeffTbl)
{
uint8_t         encodedByte;
int             CoeffAC;
uint8_t         ZeroRunLength;
uint8_t         category;
uint8_t         i;


/*  For DC coefficient, the scan data starts with a huffman encoded
 *  'category' byte. The 'category' is the minimum no. of bits required 
 *  to represent a coefficient */
 
    category     = HUFFTREE_readSymbol( jpg, &jpg->seg.Y.HuffTreeDC );
    
/*  DC Coefficient is stored as a difference from the previous block.
 *  Hence, to determine absolute DC coefficient, we add DC Coefficient
 *  of the previous block */
 
    jpg->seg.Y.DCcoeff    += readCoefficient(jpg, category);  
    
 // To improve performance, deZigZag and deQuantize Coefficients as they are retrieved
    coeffTbl[0]  = jpg->seg.Y.DCcoeff * jpg->seg.Y.QntzTbl[0];
    
/*  For AC coefficients, the scan data starts with a huffman encoded 
 *  'RLE+category' byte. The higher nibble represents the run length of
 *  preceeding zeroes whereas the lower nibble represents the 'category' */
    
        for(i = 1; i < 64; i++ )
        {
            encodedByte  = HUFFTREE_readSymbol( jpg, &jpg->seg.Y.HuffTreeAC );
            
         // Check for END_OF_BLOCK Marker(0x00)
            if( encodedByte == EOB )
            {                                                       
                for( ; i < 64; i++)
                {
                    coeffTbl[ deZigZagVector[i] ] = 0;
                }
            }
                    
            else
            {
                category        =   encodedByte & 0xf;                    
                ZeroRunLength   =   (encodedByte >> 4) & 0xf;                        
                CoeffAC         =   readCoefficient(jpg, category);        
                        
                for( ; ZeroRunLength; i++, ZeroRunLength-- )
                {
                    coeffTbl[ deZigZagVector[i] ] = 0;
                }
                coeffTbl[ deZigZagVector[i] ] = CoeffAC * jpg->seg.Y.QntzTbl[i];
            }
        }
            
}


static void decodeCbDU(jpg_t * jpg, int *coeffTbl)
{
uint8_t         encodedByte;
int             CoeffAC;
uint8_t         ZeroRunLength;
uint8_t         category;
uint8_t         i;


    category     = HUFFTREE_readSymbol( jpg, &jpg->seg.Cb.HuffTreeDC );
    jpg->seg.Cb.DCcoeff   += readCoefficient(jpg, category);  
    coeffTbl[0]  = jpg->seg.Cb.DCcoeff * jpg->seg.Cb.QntzTbl[0];
    
        for(i = 1; i < 64; i++ )
        {
            encodedByte  = HUFFTREE_readSymbol( jpg, &jpg->seg.Cb.HuffTreeAC );
                
            if( encodedByte == EOB )
            {                                                       
                for( ; i < 64; i++)
                {
                    coeffTbl[ deZigZagVector[i] ] = 0;
                }
            }
                    
            else
            {
                category        =   encodedByte & 0xf;                        
                ZeroRunLength   =   (encodedByte >> 4) & 0xf;                        
                CoeffAC         =   readCoefficient(jpg, category);        
                        
                for( ; ZeroRunLength; i++, ZeroRunLength-- )
                {
                    coeffTbl[ deZigZagVector[i] ] = 0;
                }                 
                coeffTbl[ deZigZagVector[i] ] = CoeffAC * jpg->seg.Cb.QntzTbl[i];
            }
        }
    
}


static void decodeCrDU(jpg_t * jpg, int *coeffTbl)
{
uint8_t         encodedByte;
int             CoeffAC;
uint8_t         ZeroRunLength;
uint8_t         category;
uint8_t         i;


    category     = HUFFTREE_readSymbol( jpg, &jpg->seg.Cr.HuffTreeDC );
    jpg->seg.Cr.DCcoeff   += readCoefficient(jpg, category);  
    coeffTbl[0]  = jpg->seg.Cr.DCcoeff * jpg->seg.Cr.QntzTbl[0];
              
        for(i = 1; i < 64; i++ )
        {
            encodedByte  = HUFFTREE_readSymbol( jpg, &jpg->seg.Cr.HuffTreeAC );
                
            if( encodedByte == EOB )
            {                                                       
                for( ; i < 64; i++)
                {
                    coeffTbl[ deZigZagVector[i] ] = 0;
                }
            }
                    
            else
            {
                category        =   encodedByte & 0xf;                        
                ZeroRunLength   =   (encodedByte >> 4) & 0xf;                        
                CoeffAC         =   readCoefficient(jpg, category);        
                        
                for( ; ZeroRunLength; i++, ZeroRunLength-- )
                {
                    coeffTbl[ deZigZagVector[i] ] = 0;
                }                    
                coeffTbl[ deZigZagVector[i] ] = CoeffAC * jpg->seg.Cr.QntzTbl[i];
            }
        }
    
}


static uint32_t * decodeScanData(jpg_t * jpg)
{
uint8_t     DUindx;
uint8_t     nYDU;
uint16_t    i, j;
uint16_t    nHorizBlocks, nVertBlocks;
uint16_t    RstCount;
int      *  YDU[4], * CbDU, * CrDU;
uint32_t *  raw_image;
uint32_t *  XRGB8x8Block;


/*  The order, in which the Data Units appear in the Scan Data
 *  corresponds to the order in which the components appear in the Scan-
 *  Segment. Besides, the number of blocks of each component that adds up
 *  to create a MCU is defined by the sampling factor of each component.
 *  Next, we've to take into account that the first coefficient of each
 *  block is the DC value and has a separate Huffman Table. Rest of the
 *  AC coefficients have a separate Huffman Table. Also, the Huffman
 *  encoded byte for DC coefficient consists of sole category value
 *  whereas that for AC coefficient consists of Zero-Run-Length nibble
 *  and a category nibble */ 
 
 // Allocate appropriate amount of memory for holding a decoded 8x8 block    
    XRGB8x8Block =  (uint32_t *)malloc( 8 * 8 * sizeof(uint32_t) );
    
 /* Extend the frame-width and frame-height of the image to the nearest 
  * 16-byte boundary to account for the appended blocks */
    jpg->seg.sof.frameWidth   = ( (jpg->seg.sof.frameWidth-1)  | ( (jpg->seg.Y.HSmplFctr << 3)-1 ) ) + 1 ;
    jpg->seg.sof.frameHeight  = ( (jpg->seg.sof.frameHeight-1) | ( (jpg->seg.Y.VSmplFctr << 3)-1 ) ) + 1 ;
    
 // Allocate enough memory to store the decoded image
    raw_image =  (uint32_t *)malloc( jpg->seg.sof.frameWidth * jpg->seg.sof.frameHeight * sizeof(uint32_t) );
 
 // Find out the number of Data Units of Y component present in a MCU 
    nYDU =  jpg->seg.Y.HSmplFctr  * jpg->seg.Y.VSmplFctr;
    
 // Allocate memory for each Data Units present in the MCU 
    for( DUindx = 0; DUindx < nYDU; DUindx++ )
    { 
        YDU[DUindx] = (int *)malloc( 64 * sizeof(int) );
    }
 
    CbDU = (int *)malloc( 64 * sizeof(int) );
    setBlock( CbDU, 128 );
    
    CrDU = (int *)malloc( 64 * sizeof(int) );
    setBlock( CrDU, 128 );
        
    nVertBlocks     =   (jpg->seg.sof.frameHeight)/(jpg->seg.Y.VSmplFctr << 3);
    nHorizBlocks    =   (jpg->seg.sof.frameWidth)/(jpg->seg.Y.HSmplFctr << 3);
    RstCount        =    jpg->seg.dri.nMCUs;
    
    fprintf(stdout, "\nWriting Blocks...");
        
    resetDecoder(jpg);    
    
    for( j = 0; j < nVertBlocks; j++)
    {
        for( i = 0; i < nHorizBlocks; i++)
        {
        /*  Decode n Data Units of Y Component present in the MCU and 
         *  perform Inverse Discrete Cosine Transform on each decoded block */
         
            for( DUindx = 0; DUindx < nYDU; DUindx++ )
            { 
                decodeYDU( jpg, YDU[DUindx] );            
                performIDCT( YDU[DUindx] );
            }
           
        /*  Then, decode the data units of Chroma components present in the MCU and
         *  perform Inverse Discrete Cosine Transform on the decoded block */
            
            if( jpg->seg.sof.nComponents > 1 )
            { 
                decodeCbDU( jpg, CbDU );            
                performIDCT( CbDU );                
                decodeCrDU( jpg, CrDU );            
                performIDCT( CrDU );
            }
            
            
         // Check whether Restart Interval is enabled
            if( jpg->seg.dri.nMCUs )
            {
             // Make sure to reset at the end of each Restart Interval
                if( --RstCount == 0)
                {
                    RstCount  = jpg->seg.dri.nMCUs;                    
                    resetDecoder(jpg);
                }
            }
                        
                        
            switch( (jpg->seg.Y.HSmplFctr) << 4 | jpg->seg.Y.VSmplFctr )
            {
                case 0x22:
                {
                 // 16x16 Y Block corresponds to 8x8 Cb and 8x8 Cr Block
                 // Hence, each 8x8 Y Block corresponds to a 4x4 Cb and a 4x4 Cr Block
                    Y4Cb1Cr1toXRGB(XRGB8x8Block, YDU[0], CbDU,    CrDU);
                    writeBlock(raw_image, i << 4, j << 4, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    
                    Y4Cb1Cr1toXRGB(XRGB8x8Block, YDU[1], CbDU+4,  CrDU+4 );
                    writeBlock(raw_image, (i << 4) + 8, j << 4, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    
                    Y4Cb1Cr1toXRGB(XRGB8x8Block, YDU[2], CbDU+32, CrDU+32 );
                    writeBlock(raw_image, i << 4, (j << 4) + 8, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    
                    Y4Cb1Cr1toXRGB(XRGB8x8Block, YDU[3], CbDU+36, CrDU+36 );
                    writeBlock(raw_image, (i << 4) + 8, (j << 4) + 8, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    break;
                }
                
                case 0x21:
                {
                 // Two Horizontal 8x8 Y Blocks correspond to a 8x8 Cb and a 8x8 Cr Block
                    Y2Cb1Cr1toXRGB(XRGB8x8Block, YDU[0], CbDU,    CrDU);
                    writeBlock(raw_image, 16*i, 8*j, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    
                    Y2Cb1Cr1toXRGB(XRGB8x8Block, YDU[1], CbDU+4, CrDU+4);
                    writeBlock(raw_image, 16*i+8, 8*j, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    break;
                }
                
                case 0x11:
                {
                 // Each 8x8 Y Block corresponds to a 8x8 Cb and a 8x8 Cr Block
                    Y1Cb1Cr1toXRGB(XRGB8x8Block, YDU[0], CbDU,    CrDU);
                    writeBlock(raw_image, i << 3, j << 3, XRGB8x8Block, jpg->seg.sof.frameWidth);
                    break;
                }
                
                default:
                {
                    fprintf(stdout, "\nUnsupported sampling factor!");                    
                    raw_image = (uint32_t *)NULL;
                    break;
                }
            }            
        }
    }
    
    fprintf(stdout, "\nComplete!");
    
    for( DUindx = 0; DUindx < nYDU; free(YDU[DUindx++]) );
    free(CbDU);
    free(CrDU);
    free(XRGB8x8Block);
    
    return raw_image;  
}


static void writeBlock( uint32_t * dest, uint16_t x, uint16_t y, uint32_t *XRGB8x8Block, uint16_t imageWidth)
{
uint32_t *  src;
uint32_t    offset;
uint8_t     i;


 // For the sake of efficiency over a short loop, I'm using unrolled loop
 
    for( src = XRGB8x8Block, i = 8; i; i-- )
    {
        offset = (y++)*imageWidth + x;
        
        dest[offset++] = *src++;
        dest[offset++] = *src++;
        dest[offset++] = *src++;
        dest[offset++] = *src++;
        dest[offset++] = *src++;
        dest[offset++] = *src++;
        dest[offset++] = *src++;
        dest[offset]   = *src++;
    }
    
}

    
int8_t jpg_read( uint32_t * surface, uint16_t surface_width, uint16_t surface_height, jpg_t * jpg)
{
uint16_t   x, y;
float      dx, dy;
uint32_t * raw_image;
double     sample_i;
uint32_t   offset;

    /* jpg_open() positions the file pointer at the SOS marker which contains the image data */
    
    if(readMarker(jpg) != SOS)
      return -1;
    
    fprintf(stdout, "\nReading SOS segment...");
    readSOS(jpg);
                
    raw_image = decodeScanData(jpg);
    if(!raw_image)
        return -2;
    
    if( (jpg->width == surface_width) && (jpg->height == surface_height) ) 
    {
        for(y = 0, offset = 0; y < surface_height; y++, offset += (jpg->extended_width - jpg->width))
        {
            for(x = 0; x < surface_width; x++, offset++, surface++ )
                *surface = raw_image[offset];
        }
    }
    
    else
    {
    /* Scale the image */
        dx = (float)jpg->extended_width / surface_width;
        dy = (float)jpg->extended_height / surface_height;
        
        for(y = 0, offset = 0; y < surface_height; y++ )
        {
            for(x = 0, sample_i = (int)(y * dy) * jpg->extended_width; x < surface_width; x++, sample_i += dx )
                surface[offset++] = raw_image[(uint32_t)sample_i];
        }
    }

    /* Ignore all other markers that follow the SOS marker */
    
    free(raw_image);
    return 0;

}


jpg_t * jpg_open(const char * JPGfile)
{
jpg_t   * jpg;
uint16_t  marker;
int8_t    error = 0;


    jpg = calloc(1, sizeof(jpg_t));
    if(!jpg)
      return NULL;
      
    jpg->fp = fopen(JPGfile,"rb");
    if(!jpg->fp)
    {
      //fprintf(stdout, "\nJPEG image file '%s' does not exist!", JPGfile);
      free(jpg);
      return NULL;
    }
    
    if(!validateJPEG(jpg))
    {
        fprintf(stdout, "\n'%s' is not a valid JPEG image file!", JPGfile);
        fclose(jpg->fp);
        free(jpg);
        return NULL;
    }
    
    while(!error)
    {
        marker = readMarker(jpg);
        
        switch(marker)
        {
            case APP0:
            {
                fprintf(stdout, "\nReading APP0 segment...");
                readAPP0(jpg);
                break;
            }
                
            case SOF0:
            { 
                fprintf(stdout, "\nReading SOF segment...");                
                readSOF(jpg);     
                jpg->width = jpg->seg.sof.frameWidth;
                jpg->height = jpg->seg.sof.frameHeight;
                jpg->nc = jpg->seg.sof.nComponents;
                jpg->hsf = jpg->seg.Y.HSmplFctr;
                jpg->vsf = jpg->seg.Y.VSmplFctr;
                /* The frame-width and frame-height of the image is extended to the nearest 16-byte boundary (see decodeScanData) */ 
                jpg->extended_width = ( (jpg->width-1)  | ( (jpg->hsf << 3)-1 ) ) + 1 ;
                jpg->extended_height = ( (jpg->height-1) | ( (jpg->vsf << 3)-1 ) ) + 1 ;     
                break;
            }
            
            case SOF1:
            {
                fprintf(stdout, "\nDecoding Error: Extended Sequential JPEG is not supported\n");                
                error = 1;
                break;
            }
            
            case SOF2:
            {
                fprintf(stdout, "\nDecoding Error: Progressive JPEG is not supported\n");                
                error = 1;
                break;
            }
            
            case SOF3:
            {
                fprintf(stdout, "\nDecoding Error: Lossless JPEG is not supported\n");                
                error = 1;
                break;
            }
            
            case DHT:
            {
                fprintf(stdout, "\nReading DHT segment...");                
                readDHT(jpg);                
                break;
            }
            
            case DQT:
            {
                fprintf(stdout, "\nReading DQT segment...");                
                readDQT(jpg);                
                break;
            }
            
            case DRI:
            {
                fprintf(stdout, "\nReading DRI segment...");                
                readDRI(jpg);                
                break;
            }
            
            case SOS:
            {
                return jpg;
            }
            
            case EOI:
            {
                /* EOI marker must come after SOS marker */
                error = 1;
                break;
            }
            
            case NOM:
            {
                fprintf(stdout, "\nInvalid JPEG: No marker found!");        
                error = 1;
                break;
            }
            
            default:
            {
                fprintf(stdout, "\nSkipping trivial segment (Marker: 0x%x)...", marker & 0xffff);                
                skipSegment(jpg);
            }
        }
    }

    
    fclose(jpg->fp);
    free(jpg);
    return NULL;
}


void jpg_close(jpg_t * jpg)
{
    HUFFTREE_destroy( &jpg->seg.Y.HuffTreeDC );
    HUFFTREE_destroy( &jpg->seg.Cb.HuffTreeDC );
    HUFFTREE_destroy( &jpg->seg.Y.HuffTreeAC );
    HUFFTREE_destroy( &jpg->seg.Cb.HuffTreeAC );
    
    fclose(jpg->fp);
    free(jpg);
}
