#include <stdint.h>
#include <ctype.h>

#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
#include "greatest.h"

#include <math.h>
#include <time.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include<stdint.h>


#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif



static heatshrink_encoder hse;
static heatshrink_decoder hsd;

static void fill_with_pseudorandom_letters(uint8_t *buf, uint16_t size, uint32_t seed) {
    uint64_t rn = 9223372036854775783; /* prime under 2^64 */
    for (int i=0; i<size; i++) {
        rn = rn*seed + seed;
        buf[i] = (rn % 26) + 'a';
    }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static void fill_with_ascending_numbers(uint8_t *buf, uint16_t size) {
    for (int i=0; i<size; i++) {
        buf[i] = i;
    }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static void read_data_from_file(char *fileLoc, uint8_t *data, uint16_t numBytes){

    FILE *inputFile;

    if ((inputFile = fopen(fileLoc, "rb")) == NULL){
        printf("invalid input file\n");
        exit(1);
    }
    
    fread(data, sizeof(uint8_t), numBytes, inputFile);
    fclose(inputFile);
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void dump_buf(char *name, uint8_t *buf, uint16_t count) {
    for (int i=0; i<count; i++) {
        uint8_t c = (uint8_t)buf[i];
        printf("%s %d: 0x%02x ('%c')\n", name, i, c, isprint(c) ? c : '.');
    }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static int compress(char* output_file_name, uint8_t *input, uint32_t input_size){

    FILE *outputFile;

    if ((outputFile = fopen(output_file_name, "wb")) == NULL){
        printf("invalid output file\n");
        exit(1);
    }

    heatshrink_encoder_reset(&hse);

    size_t comp_sz = input_size + (input_size/2) + 4;
    uint8_t *comp = malloc(comp_sz);
    memset(comp, 0, comp_sz);
    size_t count = 0;

    uint32_t sunk = 0;
    uint32_t polled = 0;
    while (sunk < input_size) {
        heatshrink_encoder_sink(&hse, &input[sunk], input_size - sunk, &count);
        sunk += count;
        if (sunk == input_size) {
            heatshrink_encoder_finish(&hse);
        }

        HSE_poll_res pres;
        do {                    /* "turn the crank" */
            pres = heatshrink_encoder_poll(&hse, &comp[polled], comp_sz - polled, &count);
            polled += count;
        } while (pres == HSER_POLL_MORE);
        if (sunk == input_size) {
            heatshrink_encoder_finish(&hse);
        }

        if (polled > input_size){
            return -1;
        }
    }

    if (!(polled > input_size)){
        fwrite(&comp, polled, 1, outputFile);
        
    }
    fclose(outputFile);
    //fwrite(&comp, polled, 1, outputFile);

    //fclose(outputFile);

    //dump_buf("compressed", &comp, polled);

    printf("in: %u compressed: %u \n", input_size, polled);

    //!free(comp);
    return polled;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static int compress_to_array(uint8_t outputArray[], uint8_t *input, uint16_t input_size){

    //uint16_t Size32 = (int)ceil(input_size/4);

    heatshrink_encoder_reset(&hse);
    printf("%d\n", input_size);

    size_t comp_sz = input_size + (input_size/2) + 4;
    uint8_t *comp = malloc(comp_sz);
    memset(comp, 0, comp_sz);
    size_t count = 0;

    uint32_t sunk = 0;
    uint32_t polled = 0;
    while (sunk < input_size) {
        heatshrink_encoder_sink(&hse, &input[sunk], input_size - sunk, &count);
        sunk += count;
        if (sunk == input_size) {
            heatshrink_encoder_finish(&hse);
        }

        HSE_poll_res pres;
        do {                    /* "turn the crank" */
            pres = heatshrink_encoder_poll(&hse, &comp[polled], comp_sz - polled, &count);
            polled += count;
        } while (pres == HSER_POLL_MORE);
        if (sunk == input_size) {
            heatshrink_encoder_finish(&hse);
        }

        if (polled > input_size){
            return -1;
        }
    }

    if (!(polled > input_size)){
        //fwrite(&comp, polled, 1, outputFile);
        
    }
    

    printf("in: %u compressed: %u \n", input_size, polled);

    for (int i=0; i<polled; i++){
        outputArray[i] = comp[i];
    }

    //!free(comp);
    return polled;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static void decompress(char* source_file_name, uint16_t input_size, int32_t compressed_size, char* destination_file_name){

    FILE *sourceFile;

    if ((sourceFile = fopen(source_file_name, "rb")) == NULL){
        printf("invalid input file\n");
        exit(1);
    }

    fseek(sourceFile, 0, SEEK_END);
    uint32_t fLen = ftell(sourceFile);
    fseek(sourceFile, 0, SEEK_SET);

    heatshrink_decoder_reset(&hsd);

    size_t comp_sz = compressed_size;
    size_t decomp_sz = input_size + (input_size/2) + 4;

    uint8_t *comp = malloc(comp_sz);
    uint8_t *decomp = malloc(decomp_sz);

    memset(comp, 0, comp_sz);
    memset(decomp, 0, decomp_sz);

    fread(&comp, sizeof(comp), 1, sourceFile);
    fclose(sourceFile);

    size_t count = 0;
    uint32_t sunk = 0;
    uint32_t polled = 0;

    while (sunk < compressed_size) {
        heatshrink_decoder_sink(&hsd, &comp[sunk], compressed_size - sunk, &count);
        sunk += count;
        if (sunk == compressed_size) {
            heatshrink_decoder_finish(&hsd);
        }

        HSD_poll_res pres;
        do {
            pres = heatshrink_decoder_poll(&hsd, &decomp[polled],
                decomp_sz - polled, &count);
            polled += count;
        } while (pres == HSDR_POLL_MORE);
        if (sunk == compressed_size) {
            HSD_finish_res fres = heatshrink_decoder_finish(&hsd);
        }

    }

    FILE *destinationFile;

    if ((destinationFile = fopen(destination_file_name, "wb")) == NULL){ //changed to write binary
        printf("invalid output file\n");
        exit(1);
    }

    fwrite(decomp, input_size, 1, destinationFile);
    fclose(destinationFile);

    free(comp);
    free(decomp);
}

static void decompress_from_array(uint8_t compressedData[], uint16_t input_size, int32_t compressed_size, char* destination_file_name){

    heatshrink_decoder_reset(&hsd);

    //size_t comp_sz = compressed_size;
    size_t decomp_sz = input_size + (input_size/2) + 4;

    //uint8_t *comp = malloc(comp_sz);
    uint8_t *decomp = malloc(decomp_sz);

    //memset(comp, 0, comp_sz);
    memset(decomp, 0, decomp_sz);

    //fread(&comp, sizeof(comp), 1, sourceFile);
    //fclose(sourceFile);

    size_t count = 0;
    uint32_t sunk = 0;
    uint32_t polled = 0;

    //printf("flag01\n");
    while (sunk < compressed_size) {
        //printf("flag02\n");
        heatshrink_decoder_sink(&hsd, &compressedData[sunk], compressed_size - sunk, &count);
        //printf("flag03\n");
        sunk += count;
        if (sunk == compressed_size) {
            heatshrink_decoder_finish(&hsd);
        }

        HSD_poll_res pres;
        do {
            pres = heatshrink_decoder_poll(&hsd, &decomp[polled], decomp_sz - polled, &count);
            polled += count;
        } while (pres == HSDR_POLL_MORE);
        if (sunk == compressed_size) {
            HSD_finish_res fres = heatshrink_decoder_finish(&hsd);
        }

    }

    //printf("flag0\n");
    FILE *destinationFile;

    if ((destinationFile = fopen(destination_file_name, "wb")) == NULL){ //changed to write binary
        printf("invalid output file\n");
        exit(1);
    }

    fwrite(decomp, input_size, 1, destinationFile);
    fclose(destinationFile);

    //free(comp);
    free(decomp);
}




/*
Custom lightwiegt encryption algorithm by Gabriel Nichollas
Bassed off Blowfish and Twofish encryption algorithms
File Size: 7KB
*/

uint32_t parray[18] =
{
    0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344, 0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89,
    0x452821E6, 0x38D01377, 0xBE5466CF, 0x34E90C6C, 0xC0AC29B7, 0xC97C50DD, 0x3F84D5B5, 0xB5470917,
    0x9216D5D9, 0x8979FB1B
};

uint32_t keyedParray[18];

uint32_t sbox1[256] =
{
   0xA9, 0x67, 0xB3, 0xE8, 0x04, 0xFD, 0xA3, 0x76, 0x9A, 0x92, 0x80, 0x78, 0xE4, 0xDD, 0xD1, 0x38,
   0x0D, 0xC6, 0x35, 0x98, 0x18, 0xF7, 0xEC, 0x6C, 0x43, 0x75, 0x37, 0x26, 0xFA, 0x13, 0x94, 0x48,
   0xF2, 0xD0, 0x8B, 0x30, 0x84, 0x54, 0xDF, 0x23, 0x19, 0x5B, 0x3D, 0x59, 0xF3, 0xAE, 0xA2, 0x82,
   0x63, 0x01, 0x83, 0x2E, 0xD9, 0x51, 0x9B, 0x7C, 0xA6, 0xEB, 0xA5, 0xBE, 0x16, 0x0C, 0xE3, 0x61,
   0xC0, 0x8C, 0x3A, 0xF5, 0x73, 0x2C, 0x25, 0x0B, 0xBB, 0x4E, 0x89, 0x6B, 0x53, 0x6A, 0xB4, 0xF1,
   0xE1, 0xE6, 0xBD, 0x45, 0xE2, 0xF4, 0xB6, 0x66, 0xCC, 0x95, 0x03, 0x56, 0xD4, 0x1C, 0x1E, 0xD7,
   0xFB, 0xC3, 0x8E, 0xB5, 0xE9, 0xCF, 0xBF, 0xBA, 0xEA, 0x77, 0x39, 0xAF, 0x33, 0xC9, 0x62, 0x71,
   0x81, 0x79, 0x09, 0xAD, 0x24, 0xCD, 0xF9, 0xD8, 0xE5, 0xC5, 0xB9, 0x4D, 0x44, 0x08, 0x86, 0xE7,
   0xA1, 0x1D, 0xAA, 0xED, 0x06, 0x70, 0xB2, 0xD2, 0x41, 0x7B, 0xA0, 0x11, 0x31, 0xC2, 0x27, 0x90,
   0x20, 0xF6, 0x60, 0xFF, 0x96, 0x5C, 0xB1, 0xAB, 0x9E, 0x9C, 0x52, 0x1B, 0x5F, 0x93, 0x0A, 0xEF,
   0x91, 0x85, 0x49, 0xEE, 0x2D, 0x4F, 0x8F, 0x3B, 0x47, 0x87, 0x6D, 0x46, 0xD6, 0x3E, 0x69, 0x64,
   0x2A, 0xCE, 0xCB, 0x2F, 0xFC, 0x97, 0x05, 0x7A, 0xAC, 0x7F, 0xD5, 0x1A, 0x4B, 0x0E, 0xA7, 0x5A,
   0x28, 0x14, 0x3F, 0x29, 0x88, 0x3C, 0x4C, 0x02, 0xB8, 0xDA, 0xB0, 0x17, 0x55, 0x1F, 0x8A, 0x7D,
   0x57, 0xC7, 0x8D, 0x74, 0xB7, 0xC4, 0x9F, 0x72, 0x7E, 0x15, 0x22, 0x12, 0x58, 0x07, 0x99, 0x34,
   0x6E, 0x50, 0xDE, 0x68, 0x65, 0xBC, 0xDB, 0xF8, 0xC8, 0xA8, 0x2B, 0x40, 0xDC, 0xFE, 0x32, 0xA4,
   0xCA, 0x10, 0x21, 0xF0, 0xD3, 0x5D, 0x0F, 0x00, 0x6F, 0x9D, 0x36, 0x42, 0x4A, 0x5E, 0xC1, 0xE0
};
uint32_t sbox2[256] =
{
   0x75, 0xF3, 0xC6, 0xF4, 0xDB, 0x7B, 0xFB, 0xC8, 0x4A, 0xD3, 0xE6, 0x6B, 0x45, 0x7D, 0xE8, 0x4B,
   0xD6, 0x32, 0xD8, 0xFD, 0x37, 0x71, 0xF1, 0xE1, 0x30, 0x0F, 0xF8, 0x1B, 0x87, 0xFA, 0x06, 0x3F,
   0x5E, 0xBA, 0xAE, 0x5B, 0x8A, 0x00, 0xBC, 0x9D, 0x6D, 0xC1, 0xB1, 0x0E, 0x80, 0x5D, 0xD2, 0xD5,
   0xA0, 0x84, 0x07, 0x14, 0xB5, 0x90, 0x2C, 0xA3, 0xB2, 0x73, 0x4C, 0x54, 0x92, 0x74, 0x36, 0x51,
   0x38, 0xB0, 0xBD, 0x5A, 0xFC, 0x60, 0x62, 0x96, 0x6C, 0x42, 0xF7, 0x10, 0x7C, 0x28, 0x27, 0x8C,
   0x13, 0x95, 0x9C, 0xC7, 0x24, 0x46, 0x3B, 0x70, 0xCA, 0xE3, 0x85, 0xCB, 0x11, 0xD0, 0x93, 0xB8,
   0xA6, 0x83, 0x20, 0xFF, 0x9F, 0x77, 0xC3, 0xCC, 0x03, 0x6F, 0x08, 0xBF, 0x40, 0xE7, 0x2B, 0xE2,
   0x79, 0x0C, 0xAA, 0x82, 0x41, 0x3A, 0xEA, 0xB9, 0xE4, 0x9A, 0xA4, 0x97, 0x7E, 0xDA, 0x7A, 0x17,
   0x66, 0x94, 0xA1, 0x1D, 0x3D, 0xF0, 0xDE, 0xB3, 0x0B, 0x72, 0xA7, 0x1C, 0xEF, 0xD1, 0x53, 0x3E,
   0x8F, 0x33, 0x26, 0x5F, 0xEC, 0x76, 0x2A, 0x49, 0x81, 0x88, 0xEE, 0x21, 0xC4, 0x1A, 0xEB, 0xD9,
   0xC5, 0x39, 0x99, 0xCD, 0xAD, 0x31, 0x8B, 0x01, 0x18, 0x23, 0xDD, 0x1F, 0x4E, 0x2D, 0xF9, 0x48,
   0x4F, 0xF2, 0x65, 0x8E, 0x78, 0x5C, 0x58, 0x19, 0x8D, 0xE5, 0x98, 0x57, 0x67, 0x7F, 0x05, 0x64,
   0xAF, 0x63, 0xB6, 0xFE, 0xF5, 0xB7, 0x3C, 0xA5, 0xCE, 0xE9, 0x68, 0x44, 0xE0, 0x4D, 0x43, 0x69,
   0x29, 0x2E, 0xAC, 0x15, 0x59, 0xA8, 0x0A, 0x9E, 0x6E, 0x47, 0xDF, 0x34, 0x35, 0x6A, 0xCF, 0xDC,
   0x22, 0xC9, 0xC0, 0x9B, 0x89, 0xD4, 0xED, 0xAB, 0x12, 0xA2, 0x0D, 0x52, 0xBB, 0x02, 0x2F, 0xA9,
   0xD7, 0x61, 0x1E, 0xB4, 0x50, 0x04, 0xF6, 0xC2, 0x16, 0x25, 0x86, 0x56, 0x55, 0x09, 0xBE, 0x91
};

uint32_t substitute(uint32_t input) 
{
    uint8_t in[4];
    in[0] = input >> 24;
    in[1] = (input >> 16) & 0x00FF;
    in[2] = (input >> 8) & 0x0000FF;
    in[3] = input & 0x000000FF;

    uint8_t sub1[4];
    sub1[0] = sbox1[in[0]];
    sub1[1] = sbox1[in[1]];
    sub1[2] = sbox1[in[2]];
    sub1[3] = sbox1[in[3]];

    uint8_t sub2[4];
    sub2[0] = sbox2[sub1[0]];
    sub2[1] = sbox2[sub1[1]];
    sub2[2] = sbox2[sub1[2]];
    sub2[3] = sbox2[sub1[3]];

    uint32_t output = ((sub2[0] << 24) | (sub2[1] << 16) | (sub2[2] << 8) | sub2[3]);
    return output;
}

uint32_t desubstitute(uint32_t input) 
{
    uint8_t in[4];
    in[0] = input >> 24;
    in[1] = (input >> 16) & 0x00FF;
    in[2] = (input >> 8) & 0x0000FF;
    in[3] = input & 0x000000FF;

    uint32_t desub1[4];  
    for (int i = 0; i < 4; i++) 
    {
        for (int pos = 0; pos < 256; pos++) 
        {
            if (sbox2[pos] == in[i]) 
            {
                desub1[i] = pos;
                //printf("0x%x is at pos: %d.  ", in[i], pos);
                break;
            }
        }
    }
    
    uint32_t desub2[4];
    for (int i = 0; i < 4; i++) 
    {
        for (int pos = 0; pos < 256; pos++) 
        {
            if (sbox1[pos] == desub1[i]) 
            {
                desub2[i] = pos;
                break;
            }
        }
    }
    
    uint32_t output = ((desub2[0] << 24) | (desub2[1] << 16) | (desub2[2] << 8) | desub2[3]);
    return output;
}

void createPkeys(uint32_t key)
{
    for (int i = 0; i < 18; i++)
    {
        keyedParray[i] = key ^ parray[i];
    }
}

uint32_t encryptBlock(uint32_t blockToEncrypt)
{
    uint32_t temp = blockToEncrypt;
    
    for (int i = 0; i < 18; i++)
    {
        temp = substitute(temp);
        temp = temp ^ keyedParray[i];
    }
    return temp;
}

uint32_t decryptBlock(uint32_t blockToDecrypt)
{
    uint32_t temp = blockToDecrypt;
    
    for (int i = 17; i >= 0; i--)
    {
        temp = temp ^ keyedParray[i];
        temp = desubstitute(temp);
    }
    return temp;
}

// CONCATINATION TOOLS

void concatTo32(uint32_t dest, uint8_t inputs[])
{
    dest = ((inputs[0] << 24) | (inputs[1] << 16) | (inputs[2] << 8) | inputs[3]);
}

void deconcatTo8(uint32_t input32, uint8_t dest[])
{
    dest[0] = input32 >> 24;
    dest[1] = (input32 >> 16) & 0x00FF;
    dest[2] = (input32 >> 8) & 0x0000FF;
    dest[3] = input32 & 0x000000FF;
}

int main(void){

    //uint16_t numBytes = 49152; //2^11 * 6 floats of *4 bites. Needs to be smaller than 2^16 or change to uint32
    uint16_t numBytes = 16000;

    uint8_t data[numBytes];
    uint8_t compressedData[numBytes];

    uint32_t mykey = 0x1A6B1018;
    createPkeys(mykey);

    

    //fill_with_pseudorandom_letters(&data, numBytes, 12345);
    //fill_with_ascending_numbers(&data, numBytes);
    read_data_from_file("/home/sebastian/EEE3097S/EEE3097_Compression/processData/pData.bin", data, numBytes);

    //!!! Start clock
    clock_t begin = clock();


    uint32_t compressedSize = compress_to_array(compressedData, &data, numBytes);
    printf("compression successful!\n");

    uint32_t encryptedData[(int)ceil(numBytes/4)];
    uint32_t decrypted4Data[(int)ceil(numBytes/4)];

    uint32_t *compress4;
    compress4 = (uint32_t*)(&compressedData);

    uint16_t num4Bytes = (int)ceil(compressedSize/4);

    for (int i=0; i<num4Bytes+1; i++){
        encryptedData[i] = encryptBlock(compress4[i]);
    }
    printf("encryption successful!\n");
    dump_buf("encrypted", encryptedData, 4);

    for (int i=0; i<num4Bytes+1; i++){
        decrypted4Data[i] = decryptBlock(encryptedData[i] );
    }
    printf("decryption successful!\n");

    uint8_t *decrypt;
    decrypt = (uint8_t*)(&decrypted4Data);

    //decompress_from_array(compressedData,numBytes,compressedSize, "destinationFile.bin");
    decompress_from_array(decrypt,numBytes,compressedSize, "destinationFile.bin");
    printf("decompression successful!\n");

    //!!!!!!!!!!!!!end clock
    clock_t end = clock();

    printf("Full test took %f seconds to process %d bytes\n", (double)(end - begin) / CLOCKS_PER_SEC, numBytes );
    
    return 0;
    

}
