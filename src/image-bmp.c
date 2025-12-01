#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1) 
// Prevent compiler from adding padding bytes into the structs.
// This ensures our headers exactly match the BMP file format.
typedef struct {
    unsigned short bfType;      // File type identifier, must be "BM"
    unsigned int   bfSize;      // Total file size in bytes
    unsigned short bfReserved1; // Reserved, must be 0
    unsigned short bfReserved2; // Reserved, must be 0
    unsigned int   bfOffBits;   // Offset from file start to pixel data
} BMPFileHeader;

typedef struct {
    unsigned int   biSize;          // Header size (40 bytes for BITMAPINFOHEADER)
    int            biWidth;         // Image width in pixels
    int            biHeight;        // Image height in pixels (can be negative)
    unsigned short biPlanes;        // Number of planes, must be 1
    unsigned short biBitCount;      // Bits per pixel (should be 24 for our tool)
    unsigned int   biCompression;   // Compression type (0 = uncompressed)
    unsigned int   biSizeImage;     // Size of pixel data
    int            biXPelsPerMeter; // Horizontal resolution
    int            biYPelsPerMeter; // Vertical resolution
    unsigned int   biClrUsed;       // Number of colors used
    unsigned int   biClrImportant;  // Number of important colors
} BMPInfoHeader;
#pragma pack(pop)


// ------------------------------------------------------------
// Function: embedMessage
// Purpose : Embed a secret message into a 24-bit BMP image
// Method  : Least Significant Bit (LSB) modification
// ------------------------------------------------------------
void embedMessage(const char* inputImage, const char* outputImage, const char* message) {
    FILE* in = fopen(inputImage, "rb");
    if (!in) { printf("Error opening input file.\n"); return; }

    // Load the entire file into memory
    fseek(in, 0, SEEK_END);
    long fileSize = ftell(in);
    rewind(in);

    unsigned char* buffer = malloc(fileSize);
    fread(buffer, 1, fileSize, in);
    fclose(in);

    BMPFileHeader* fileHeader = (BMPFileHeader*)buffer;
    BMPInfoHeader* infoHeader = (BMPInfoHeader*)(buffer + sizeof(BMPFileHeader));

    // Validate BMP file
    if (fileHeader->bfType != 0x4D42) { // 'BM' in little endian
        printf("Not a BMP file!\n");
        free(buffer);
        return;
    }

    // Pointer to the start of pixel data
    unsigned char* pixelData = buffer + fileHeader->bfOffBits;

    int width = infoHeader->biWidth;
    int height = abs(infoHeader->biHeight);
    int rowSize = ((infoHeader->biBitCount * width + 31) / 32) * 4; // includes padding

    int msgLen = strlen(message);
    int totalBits = 32 + msgLen * 8; // 32 bits for length + message bits

    // Capacity check: image must have enough pixels to hold the message
    if (totalBits > width * height * 3) {
        printf("Message too long for this image.\n");
        free(buffer);
        return;
    }

    // Embed message into pixel data
    int bitIndex = 0;
    for (int y = 0; y < height; y++) {
        unsigned char* row = pixelData + y * rowSize;
        for (int x = 0; x < width * 3; x++) { // process B, G, R channels
            if (bitIndex < totalBits) {
                unsigned char bit;
                if (bitIndex < 32) {
                    // First 32 bits store the message length
                    bit = (msgLen >> bitIndex) & 1;
                }
                else {
                    // Remaining bits store the actual message
                    int charIndex = (bitIndex - 32) / 8;
                    int charBit = (bitIndex - 32) % 8;
                    bit = (message[charIndex] >> charBit) & 1;
                }
                row[x] = (row[x] & 0xFE) | bit; // Replace LSB with our bit
                bitIndex++;
            }
        }
    }

    // Update header size values
    fileHeader->bfSize = fileSize;
    infoHeader->biSizeImage = fileSize - fileHeader->bfOffBits;

    // Write modified BMP to output file
    FILE* out = fopen(outputImage, "wb");
    if (!out) { printf("Error opening output file.\n"); free(buffer); return; }
    fwrite(buffer, 1, fileSize, out);
    fclose(out);

    free(buffer);
    printf("Message embedded successfully!\n");
}


// ------------------------------------------------------------
// Function: extractMessage
// Purpose : Extract a hidden message from a 24-bit BMP image
// Method  : Reads the LSBs of pixel data
// ------------------------------------------------------------
void extractMessage(const char* inputImage) {
    FILE* in = fopen(inputImage, "rb");
    if (!in) { printf("Error opening file.\n"); return; }

    // Load entire file into memory
    fseek(in, 0, SEEK_END);
    long fileSize = ftell(in);
    rewind(in);

    unsigned char* buffer = malloc(fileSize);
    fread(buffer, 1, fileSize, in);
    fclose(in);

    BMPFileHeader* fileHeader = (BMPFileHeader*)buffer;
    BMPInfoHeader* infoHeader = (BMPInfoHeader*)(buffer + sizeof(BMPFileHeader));

    // Validate BMP file
    if (fileHeader->bfType != 0x4D42) {
        printf("Not a BMP file!\n");
        free(buffer);
        return;
    }

    unsigned char* pixelData = buffer + fileHeader->bfOffBits;

    int width = infoHeader->biWidth;
    int height = abs(infoHeader->biHeight);
    int rowSize = ((infoHeader->biBitCount * width + 31) / 32) * 4;

    // -------------------------------
    // Step 1: Read message length (32 bits)
    // -------------------------------
    int msgLen = 0;
    int bitCount = 0;

    for (int y = 0; y < height && bitCount < 32; y++) {
        unsigned char* row = pixelData + y * rowSize;
        for (int x = 0; x < width * 3 && bitCount < 32; x++) {
            unsigned char bit = row[x] & 1;
            msgLen |= (bit << bitCount);
            bitCount++;
        }
    }

    if (msgLen <= 0 || msgLen > 1000000) {
        printf("Invalid or corrupted message length: %d\n", msgLen);
        free(buffer);
        return;
    }

    // -------------------------------
    // Step 2: Read message content
    // -------------------------------
    char* message = malloc(msgLen + 1);
    memset(message, 0, msgLen + 1);

    int charIndex = 0, charBit = 0;
    int bitsRead = 0;
    int startBit = 32; // skip first 32 bits (length)

    for (int y = 0; y < height && charIndex < msgLen; y++) {
        unsigned char* row = pixelData + y * rowSize;
        for (int x = 0; x < width * 3 && charIndex < msgLen; x++) {
            int globalBit = y * width * 3 + x;
            if (globalBit >= startBit) {
                unsigned char bit = row[x] & 1;
                message[charIndex] |= (bit << charBit);
                charBit++;
                if (charBit == 8) {
                    charBit = 0;
                    charIndex++;
                }
                bitsRead++;
                if (bitsRead >= msgLen * 8) break; // stop after full message
            }
        }
    }

    printf("Extracted message: %s\n", message);
    free(message);
    free(buffer);
}
