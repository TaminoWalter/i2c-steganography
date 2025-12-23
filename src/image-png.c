#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Diese defines dürfen NUR in dieser Datei stehen!
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void embedMessagePNG(const char* inputImage, const char* outputImage, const char* message) {
    int width, height, channels;
    unsigned char* img = stbi_load(inputImage, &width, &height, &channels, 0);
    if (img == NULL) {
        printf("Error loading PNG: %s\n", inputImage);
        return;
    }

    if (channels < 3) {
        printf("Image needs RGB or RGBA channels.\n");
        stbi_image_free(img);
        return;
    }

    int msgLen = strlen(message);
    int totalBits = 32 + msgLen * 8;
    int bitIndex = 0;

    for (int i = 0; i < width * height * channels; i++) {
        if (channels == 4 && (i % 4 == 3)) continue; // Skip Alpha

        if (bitIndex < totalBits) {
            unsigned char bit;
            if (bitIndex < 32) bit = (msgLen >> bitIndex) & 1;
            else bit = (message[(bitIndex - 32) / 8] >> ((bitIndex - 32) % 8)) & 1;

            img[i] = (img[i] & 0xFE) | bit;
            bitIndex++;
        } else break;
    }

    if (!stbi_write_png(outputImage, width, height, channels, img, width * channels)) {
        printf("Failed to write output PNG.\n");
    } else {
        printf("Embedded successfully. Created file %s\n", outputImage);
    }
    stbi_image_free(img);
}

void extractMessagePNG(const char* inputImage, const char* outputFile) {
    int width, height, channels;
    unsigned char* img = stbi_load(inputImage, &width, &height, &channels, 0);
    if (!img) { printf("Error loading PNG.\n"); return; }

    int msgLen = 0, bitIndex = 0, i = 0;

    // Länge lesen
    while (bitIndex < 32 && i < width * height * channels) {
        if (channels == 4 && (i % 4 == 3)) { i++; continue; }
        msgLen |= ((img[i] & 1) << bitIndex);
        bitIndex++; i++;
    }

    if (msgLen <= 0 || msgLen > 10000) {
        printf("No message found or invalid length.\n");
        stbi_image_free(img);
        return;
    }

    char* message = calloc(msgLen + 1, 1);
    int charIdx = 0, charBit = 0;

    // Nachricht lesen
    while (charIdx < msgLen && i < width * height * channels) {
        if (channels == 4 && (i % 4 == 3)) { i++; continue; }

        if (img[i] & 1) message[charIdx] |= (1 << charBit);

        charBit++;
        if (charBit == 8) { charBit = 0; charIdx++; }
        i++;
    }

    if (outputFile != NULL) {
        // In Datei speichern
        FILE* f = fopen(outputFile, "wb");
        if (f) {
            fwrite(message, 1, msgLen, f);
            fclose(f);
            printf("Successfully extracted content to '%s' (%d bytes).\n", outputFile, msgLen);
        } else {
            printf("Error: Could not write to file '%s'.\n", outputFile);
        }
    } else {
        // Auf Konsole ausgeben (nur Text)
        printf("Extracted content:\n%s\n", message);
    }
    // ---------------------------------------------

    free(message);
    stbi_image_free(img);
}


long getPngCapacity(const char* inputImage) {
    int width, height, channels;

    // stbi_info holt nur Dimensionen, lädt nicht die Pixel (sehr schnell)
    int ok = stbi_info(inputImage, &width, &height, &channels);

    if (!ok) {
        return -1;
    }

    // Wir nutzen immer 3 Kanäle (RGB) zum Verstecken, auch wenn Alpha (4) da ist.
    long maxBits = ((long)width * (long)height * 3) - 32;

    if (maxBits < 0) return 0;
    return maxBits / 8;
}
