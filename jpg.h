#ifndef     __JPG_H
#define     __JPG_H

#include  "stddef.h"
#include  "stdio.h"
#include  "stdint.h"


/* The header part of a JPEG file is divided into segments
 * Each segment starts with a marker identifying the segment */

// The following are some of the markers defined in JPEG standard

#define     SOI     0xFFD8              // Start of Image
#define     APP0    0xFFE0              // JPEG Application Segment
#define     EOI     0xFFD9              // End of Image
#define     SOF0    0xFFC0              // Start of Frame (baseline DCT)
#define     SOF1    0xFFC1              // Start of Frame (Extended Sequential DCT)
#define     SOF2    0xFFC2              // Start of Frame (Progressive DCT)
#define     SOF3    0xFFC3              // Start of Frame (Lossless)
#define     SOS     0xFFDA              // Start of Scan
#define     DHT     0xFFC4              // Define Huffman Table
#define     DQT     0xFFDB              // Define Quantization Table
#define     DRI     0xFFDD              // Define Restart Interval
#define     NOM     0x0000              // No Marker present

#define     EOB     0x00                // END_OF_BLOCK marker for Huffman bitstream


struct NODE
{
struct NODE *   parent;
struct NODE *   leftChild;
struct NODE *   rightChild;
uint8_t         isLeaf;
uint8_t         leafSymbol;
};


typedef struct
{
    uint8_t         ID;                // Component ID
    uint8_t         SmplFctr;          // High nibble = horizontal sampling factor, low nibble = vertical sampling factor
    uint8_t         QntzTblN;          // Quantization table number of the Component
}
__attribute__((packed)) FCSF;          // Frame Component Specification


typedef struct
{
    uint8_t         ID;                // Component ID
    uint8_t         HuffTblN;          // High nibble = DC Huffman table number, low nibble = AC Huffman table number
}
__attribute__((packed)) SCSF;          // Scan Component Specification


typedef struct
{
    uint8_t         ID;                 // ID (For Luminance, typically 1; for Chroma-blue, typically 2; for Chroma-red, typically 3)
    uint8_t         HSmplFctr;          // The horizontal sampling factor
    uint8_t         VSmplFctr;          // The vertical sampling factor
    uint8_t   *     QntzTbl;            // Pointer to the 8x8 Quantization Table
    struct NODE     HuffTreeDC;         // Root of the Huffman Tree for DC coefficients
    struct NODE     HuffTreeAC;         // Root of the Huffman Tree for AC coefficients
    int             DCcoeff;            // Current value for the DC coefficient
}
Component;


typedef struct 
{
    uint16_t        SOI_marker;         // Start of Image Marker (Always 0xFFD8)
}
SOIseg;


typedef struct
{
    uint16_t        APP0_marker;        // JPEG Application Segment Marker (Always 0xFFEO). This marks the beginning of the JFIF header
    uint16_t        length;             // Length of segment (excluding the APP0 marker but including the byte count value)
    char            identifier[5];      // Always "JFIF" with a 0 to terminate string
    uint16_t        version;            // Most significant byte used for major revision, least significant byte for minor revision
    uint8_t         density_units;      // Unit of pixel density fields
    uint16_t        xdensity;           // Horizontal pixel density
    uint16_t        ydensity;           // Vertical pixel density

/*  Unit of pixel density fields
 *  0 - No units, only aspect ratio specified
 *  1 - pixels per inch
 *  2 - pixels per centimetre */
}
__attribute__((packed)) APP0seg;


typedef struct
{
    uint16_t        SOF_marker;         // JPEG Start of Frame Marker (Always 0xFFC0)
    uint16_t        length;             // Length of segment
    uint8_t         samplePrecision;    // Sample Precision
    uint16_t        frameHeight;        // Number of lines
    uint16_t        frameWidth;         // Samples per line
    uint8_t         nComponents;        // Number of Components, typically 3(Y,Cb,Cr)
    FCSF            FCSFstruct[3];      // Frame Component specification for individual Components (assumming 3 Components)
}
__attribute__((packed)) SOFseg;


typedef struct
{
    uint16_t        DQT_marker;         // JPEG Define Quantization Table Marker (Always 0xFFDB)
    uint16_t        length;             // Length of segment
    uint8_t         QTID;               // 4-bit Quantization Table Precision(always 0) + 4-bit Quantization Table Identifier
    uint8_t         QntzTbl[8][8];      // Quantization Table/Matrix
}
__attribute__((packed)) DQTseg;


typedef struct 
{
    uint16_t        DHT_marker;         // JPEG Define Huffman Table Marker (Always 0xFFC4)
    uint16_t        length;             // Length of segment
    uint8_t         CLASS_ID;           // 4-bit Huffman Table Class + 4-bit Huffman Table Identifier
    uint8_t         huffCodefreq[16];   // Frequency of Huffman Codeword of Codelength i (for i = 1 ... 16)
    
/*  The rest of the segment lists the value associated with each Huffman
 *  codeword. However, number of codewords is only known after summing up
 *  all values in the huffCodefeq field. Hence, it is impossible to know 
 *  beforehand the size of the list. So, we need to read those fields
 *  separately from this structure after reading all huffCodefeq values */
}
DHTseg;


typedef struct 
{
    uint16_t        SOS_marker;         // JPEG Start of Scan Marker (Always 0xFFDA)
    uint16_t        length;             // Length of segment
    uint8_t         nComponents;        // Number of Components, typically 3(Y,Cb,Cr)
    SCSF            SCSFstruct[3];      // Scan Component specification for individual Components (assumming 3 Components)
    uint8_t         unused1;
    uint8_t         unused2;
    uint8_t         unused3;
    
/*  The rest of the segment is basically a stream of Huffman encoded
 *  Category byte(for DC Component) or Zero-run-length nibble + Category
 *  nibble(for AC Component). This byte is followed by the bit string
 *  which determines the value of the corresponding DCT Coefficient */ 
    
}
__attribute__((packed)) SOSseg;


typedef struct
{
    uint16_t        DRI_marker;         // JPEG Define Restart Interval Marker (Always 0xFFDD)
    uint16_t        length;             // Length of segment
    uint16_t        nMCUs;              // Number of MCUs in the Restart Interval
}
__attribute__((packed)) DRIseg;


typedef struct
{
    FILE  *   fp;
    uint16_t  width;            /* Width of the JPEG image */
    uint16_t  height;           /* Height of the JPEG image */
    uint16_t  extended_width;   /* Width of the image extended to the nearest 16 byte boundary */
    uint16_t  extended_height;  /* Height of the image extended to the nearest 16 byte boundary */
    uint16_t  nc;               /* Number of Components */
    uint16_t  hsf;              /* Horizontal sampling factor (luminance) */
    uint16_t  vsf;              /* Vertical sampling factor (luminance) */
    struct
    {
        APP0seg     app0;
        SOFseg      sof;
        DQTseg      dqt[2];
        DHTseg      dht;
        SOSseg      sos;
        DRIseg      dri;
        Component   Y;
        Component   Cb;
        Component   Cr;
    } seg;
    
    struct
    {
        uint8_t index;          /* Index of the next bit to read (Note: index of MSBit = 0 and LSBit = 7 (big-endian) */
        uint8_t _byte;          /* Current byte in the stream */
    } stream;
}
jpg_t;


jpg_t  *  jpg_open(const char * JPGfile);
int8_t    jpg_read( uint32_t * surface, uint16_t surface_width, uint16_t surface_height, jpg_t * jpg);
void      jpg_close(jpg_t * jpg);


#endif
