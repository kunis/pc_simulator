
#ifndef  __BITMAP_INCLUDE__
#define __BITMAP_INCLUDE__

#include <stdint.h>

typedef struct tagBITMAPFILEHEADER {
        uint16_t    bfType;
        uint32_t   bfSize;
        uint16_t    bfReserved1;
        uint16_t    bfReserved2;
        uint32_t   bfOffBits;
}__attribute__((packed)) BITMAPFILEHEADER;


typedef struct tagBITMAPINFOHEADER{
        uint32_t      biSize;
        uint32_t       biWidth;
        uint32_t       biHeight;
        uint16_t       biPlanes;
        uint16_t       biBitCount;
        uint32_t      biCompression;
        uint32_t      biSizeImage;
        uint32_t       biXPelsPerMeter;
        uint32_t       biYPelsPerMeter;
        uint32_t      biClrUsed;
        uint32_t      biClrImportant;
}__attribute__((packed)) BITMAPINFOHEADER;



#endif
