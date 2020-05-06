#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef unsigned char BYTE;

#pragma pack(push, 1)

typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;  //specifies the file type - should be equal to: 0x4D42 for BM
    DWORD bfSize;  //specifies the size in bytes of the bitmap file
    WORD  bfReserved1;  //reserved; must be 0
    WORD  bfReserved2;  //reserved; must be 0
    DWORD bfOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
} BITMAPFILEHEADER;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;  //specifies the number of bytes required by the struct
    LONG  biWidth;  //specifies width in pixels
    LONG  biHeight;  //species height in pixels
    WORD  biPlanes; //specifies the number of color planes, must be 1
    WORD  biBitCount; //specifies the number of bit per pixel
    DWORD biCompression;//spcifies the type of compression
    DWORD biSizeImage;  //size of image in bytes
    LONG  biXPelsPerMeter;  //number of pixels per meter in x axis
    LONG  biYPelsPerMeter;  //number of pixels per meter in y axis
    DWORD biClrUsed;  //number of colors used by th ebitmap
    DWORD biClrImportant;  //number of colors that are important
} BITMAPINFOHEADER;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmInfoHeader;
    RGBQUAD bmInfoColors[1];
} BITMAPINFO;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct tagBITMAP {
    BITMAPINFO bmInfo;
    unsigned char* biData;
    unsigned char* miscellaneous;
    unsigned long misLen;
} BITMAP, *PBITMAP;

#pragma pack(pop)

void printFileHeader(BITMAPFILEHEADER bitmapFileHeader) {
    printf("\nBITMAP_FILE_HEADER:\n");
    printf("bfType: " "0x%x (BM)\n", bitmapFileHeader.bfType);
    printf("bfSize: " "%" PRIu32 "\n", bitmapFileHeader.bfSize);
    printf("bfReserved1: " "0x%x\n", bitmapFileHeader.bfReserved1);
    printf("bfReserved2: " "0x%x\n", bitmapFileHeader.bfReserved2);
    printf("bfOffBits: " "%" PRIu32 "\n", bitmapFileHeader.bfOffBits);
}

void printInfoHeader(BITMAPINFOHEADER bitmapInfoHeader) {
    printf("\nBITMAP_INFO_HEADER:\n");
    printf("biSize: " "%" PRIu32 "\n", bitmapInfoHeader.biSize);
    printf("biWidth: " "%" PRId32 "\n", bitmapInfoHeader.biWidth);
    printf("biHeight: " "%" PRId32 "\n", bitmapInfoHeader.biHeight);
    printf("biPlanes: " "%" PRIu16 "\n", bitmapInfoHeader.biPlanes);
    printf("biBitCount: " "%" PRIu16 "\n", bitmapInfoHeader.biBitCount);
    printf("biCompression: " "%" PRIu32 "\n", bitmapInfoHeader.biCompression);
    printf("biSizeImage: " "%" PRIu32 "\n", bitmapInfoHeader.biSizeImage);
    printf("biXPelsPerMeter: " "%" PRId32 "\n", bitmapInfoHeader.biXPelsPerMeter);
    printf("biYPelsPerMeter: " "%" PRId32 "\n", bitmapInfoHeader.biYPelsPerMeter);
    printf("biClrUsed: " "%" PRIu32 "\n", bitmapInfoHeader.biClrUsed);
    printf("biClrImportant: " "%" PRIu32 "\n\n", bitmapInfoHeader.biClrImportant);
}

void saveBitMap(const char* output, PBITMAP bitmap, BITMAPFILEHEADER biFileHeader, bool hasMiscellaneous) {
    FILE* file;

    file = fopen(output, "wb"); // wb for write-binary mode
    if (file == NULL) {
        printf("Cannot open output file!\n");
        return;
    }
    
    fwrite(&biFileHeader, sizeof(biFileHeader), 1, file);
    fwrite(&bitmap->bmInfo, sizeof(BITMAPINFO), 1, file);
    if (hasMiscellaneous)
        fwrite(bitmap->miscellaneous, 1, bitmap->misLen, file);
    fwrite(bitmap->biData, 1, bitmap->bmInfo.bmInfoHeader.biSizeImage, file);
    fclose(file);
}

//Ineficient method for chaning pic to greyscale;
void changeToGreyscale(PBITMAP bitmap) {
    // BGR - bitmap

    // Check if there are junk bytes in pixelArray;
    long startJunkByte = (
        ((long)bitmap->bmInfo.bmInfoHeader.biWidth * 3) % 4 == 0 ?
        -1 : (long)bitmap->bmInfo.bmInfoHeader.biWidth * 3
    );
    int noJunkBytes = startJunkByte == -1 ? -1 : 4 - (startJunkByte % 4);
    
    int count = 0;
    int sum = 0;
    for (int i = 0; i < bitmap->bmInfo.bmInfoHeader.biSizeImage; i++) {
        if (startJunkByte != -1 && i != 0 && i % startJunkByte == 0) {
            // Start of junk bytes;
            i += noJunkBytes - 1; // One is incremented after ending for loop;
            continue;
        }

        sum += bitmap->biData[i];
        count++;

        if (count == 3) {
            sum /= 3;
            bitmap->biData[i - 2] = bitmap->biData[i - 1] = bitmap->biData[i] = sum;
            sum = count = 0;
        }
    }
}

void displayHist(float color[]) {
    int startRange = 0;
    int endRange = 15;
    for (int i = 0; i < 16; i++) {
        printf("\t%d-%d: %.2f%%\n", startRange, endRange, color[i]);
        startRange += 16;
        endRange += 16;
    }
    printf("\n");
}

void calculateHist(PBITMAP bitmap) {
    float blue[16] = {0}; // 256 / 16 = 16
    float green[16] = {0};
    float red[16] = {0};
    int lenBlue = 0;
    int lenGreen = 0;
    int lenRed = 0;

    long startJunkByte = (
        ((long)bitmap->bmInfo.bmInfoHeader.biWidth * 3) % 4 == 0 ?
        -1 : (long)bitmap->bmInfo.bmInfoHeader.biWidth * 3
    );
    int noJunkBytes = startJunkByte == -1 ? -1 : 4 - (startJunkByte % 4);
    
    int tmpColor = 0;
    int count = 0;
    int curInd = 0;
    for (int i = 0; i < bitmap->bmInfo.bmInfoHeader.biSizeImage; i++) {
        if (startJunkByte != -1 && i != 0 && i % startJunkByte == 0) {
            // Start of junk bytes;
            i += noJunkBytes - 1; // One is incremented after ending for loop;
            continue;
        }

        count %= 3;
        tmpColor = bitmap->biData[i];
        curInd = tmpColor / 16;

        switch (count)
        {
        case 0:
            blue[curInd] += tmpColor;
            lenBlue += tmpColor;
            break;
        case 1:
            green[curInd] += tmpColor;
            lenGreen += tmpColor;
            break;
        case 2:
            red[curInd] += tmpColor;
            lenRed += tmpColor;
            break;
        }

        count++;
    }

    for (int i = 0; i < 16; i++) {
        blue[i] = blue[i] / lenBlue;
        green[i] = green[i] / lenGreen;
        red[i] = red[i] / lenRed;
    }
    printf("Histogram of colors:\n\n");
    printf("Red:\n");
    displayHist(red);
    
    printf("Green:\n");
    displayHist(green);

    printf("Blue:\n");
    displayHist(blue);
}

PBITMAP loadBitmapData(const char* filename, BITMAPFILEHEADER* bitmapFileHeader, bool* hasMiscellaneous) {
    FILE* file;
    BITMAPINFO bmInfo; // InfoHeader + colors;
    PBITMAP bitmap; // bmInfo + miscellaneous (if present) + pixels;

    file = fopen(filename, "rb"); // 'rb' for read-binary mode;
    if (file == NULL) {
        printf("Cannot open input file!\n");
        return NULL;
    }

    // Read file header;
    fread(bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, file);

    // Check if file is bitmap --> bfType == 0x4D42;
    if (bitmapFileHeader->bfType != 0x4D42) {
        fclose(file);

        printf("File is not type of BMP file!\n");
        return NULL;
    }

    bool RGBQExists = true;
    if (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) == bitmapFileHeader->bfOffBits) {
        // RGBQUAD does not exist in the file!
        RGBQExists = false;
    }

    if (RGBQExists) {
        // Read infoHeader + colors;
        fread(&bmInfo, sizeof(BITMAPINFO), 1, file);
    }
    else {
        fread(&bmInfo, sizeof(BITMAPINFO) - sizeof(RGBQUAD), 1, file);
    }

    // Check if bitmap is supported;
    if (bmInfo.bmInfoHeader.biBitCount != 24 ||
        bmInfo.bmInfoHeader.biCompression != 0 ||
        (
            bmInfo.bmInfoHeader.biClrUsed != 0 &&
            bmInfo.bmInfoHeader.biClrImportant != 0
        )
    ) {
        fclose(file);

        printf("Not supported bitmap file!\n");
        return NULL;
    }

    // Check if miscellaneous data is present between headers and pixel array;
    long bytesLeft = (long)bitmapFileHeader->bfOffBits - (long)(sizeof(bmInfo) + sizeof(*bitmapFileHeader));
    unsigned char* miscellaneous = calloc((size_t) bytesLeft, sizeof(BYTE));

    if (bytesLeft == 0 || !RGBQExists) {
        // All good, nothing to read;
        free(miscellaneous);
    }
    else if (bytesLeft > 0) {
        // Something to read between headers and pixel array;
        // First check if space allocated correctly;
        if (!miscellaneous) {
            printf("Error while allocating pixelArray memory!\n");
            free(miscellaneous); // Needed?;
            fclose(file);
            return NULL;
        }

        fread(miscellaneous, sizeof(BYTE), (size_t)bytesLeft, file);
    }
    else {
        // Shouldnt occur - unknown error (unsupported bitmap type???);
        free(miscellaneous);
        fclose(file);
        printf("Unknown error - cannot process the file! \n");
        return NULL;
    }

    // Go to pixel array, allocate space needed, read data and check for errors;
    fseek(file, bitmapFileHeader->bfOffBits, SEEK_SET);

    if (bmInfo.bmInfoHeader.biSizeImage == 0) {
        bmInfo.bmInfoHeader.biSizeImage = (
            bmInfo.bmInfoHeader.biHeight * bmInfo.bmInfoHeader.biWidth * 3
        );
    }
    
    unsigned char* bitmapData = calloc(bmInfo.bmInfoHeader.biSizeImage, sizeof(BYTE));

    if (!bitmapData) {
        printf("Error while allocating pixelArray memory!\n");
        free(bitmapData); // Needed?;
        fclose(file);
        return NULL;
    }

    fread(bitmapData, sizeof(char), bmInfo.bmInfoHeader.biSizeImage, file);

    if (bitmapData == NULL) {
        printf("Error while reading bitmap image data!\n");
        fclose(file);
        return NULL;
    }

    fclose(file);

    if (bytesLeft == 0) {
        // Allocate space for BITMAPINFO and bitmapData (pixelArray) only;
        bitmap = calloc(1, sizeof(BITMAPINFO) + sizeof(unsigned char*));
    }
    else if (!RGBQExists) {
        bitmap = calloc(1, sizeof(BITMAPINFO) - sizeof(RGBQUAD) + sizeof(unsigned char*));
    }
    else if (bytesLeft > 0) {
        // Allocate space for entire BITMAP struc - including miscellaneous data;
        bitmap = calloc(1, sizeof(BITMAP));
        bitmap->miscellaneous = miscellaneous;
        bitmap->misLen = bytesLeft;
        *hasMiscellaneous = true;
    }

    if (bitmap == NULL) {
        printf("Error while allocating bitmap memory!\n");
        free(bitmap);
        return NULL;
    }

    // Copy bmInfo memory and bitmapData (pixelArray) to bitmap fields;
    memcpy(&bitmap->bmInfo, &bmInfo, sizeof(bmInfo));
    bitmap->biData = bitmapData;

    return bitmap;
}

void parseBMP(const char* filename, const char* output) {
    BITMAPFILEHEADER bitmapFileHeader;
    bool hasMiscellaneous = false;
    PBITMAP bitmap = loadBitmapData(filename, &bitmapFileHeader, &hasMiscellaneous);
    if (bitmap == NULL)
        return;

    printf("----\n%s:\n----", filename);
    printFileHeader(bitmapFileHeader);
    printInfoHeader(bitmap->bmInfo.bmInfoHeader);
    calculateHist(bitmap);

    if (output != NULL) {
        changeToGreyscale(bitmap);
        saveBitMap(output, bitmap, bitmapFileHeader, hasMiscellaneous);
    }
}

int main(int argc, const char* argv[])
{
    if (argc > 3) {
        printf("Cannot process more than one input and one output file!\n");
        return -1;
    }

    for (int i = 1; i < argc; i += 2)
    {
        parseBMP(argv[i], (i + 1 < argc) ? argv[i + 1] : NULL);
    }
    return 0;
}