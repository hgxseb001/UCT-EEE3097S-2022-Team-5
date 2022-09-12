#include <stdint.h>
#include <ctype.h>

#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
#include "greatest.h"

#include <math.h>
#include <time.h>

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

static int compress_to_array(uint32_t *outputArray, uint8_t *input, uint32_t input_size){

    uint16_t Size32 = (int)ceil(input_size/4);

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
        //fwrite(&comp, polled, 1, outputFile);
        
    }
    

    printf("in: %u compressed: %u \n", input_size, polled);

    uint16_t uint32Count = 0;
    for (int i=0; i<Size32; i++){
        outputArray[i] = (comp[i*4] <<24) + (comp[(i*4)+1] <<16) + (comp[(i*4)+2] <<8) + comp[(i*4)+3];
        uint32Count++;
        //printf("%d\n", i);
    }
    //printf("flag1\n");
    uint8_t rem = polled % 4;
    uint32_t tempOutput = 0;
    if (rem > 0){
        for (int i=0; i< 4; i++){
            if (!(rem - i < 1)){
                tempOutput += (comp[(uint32Count*4)+i] <<((4-i)*8));
            }  
        }
        outputArray[uint32Count+1] = tempOutput;
        uint32Count++;
        

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

int main(void){

    

    uint16_t numBytes = 49152;//2^11 * 6 floats of *4 bites. Needs to be smaller than 2^16 or change to uint32
    //uint16_t numBytes = 20;
    uint8_t data[numBytes];

    //uint32_t compressedData[ceil(numBytes / 4)];

    //fill_with_pseudorandom_letters(&data, numBytes, 12345);
    //fill_with_ascending_numbers(&data, numBytes);
    read_data_from_file("/home/sebastian/EEE3097S/EEE3097_Compression/processData/Data2.bin", data, numBytes);
    //read_data_from_file("/home/sebastian/EEE3097S/EEE3097_Compression/processData/Data2.bin", data, numBytes);
    //read_data_from_file("/home/sebastian/EEE3097S/EEE3097_Compression/processData/Data3.bin", data, numBytes);

    clock_t begin = clock();
    int32_t compressedSize = compress("outputFile.bin", &data, numBytes);
    clock_t end = clock();
    printf("compression successful!\n");

    if (compressedSize == -1){

        printf("compressed file larger than input, return input\n");
        FILE *destinationFile;

        if ((destinationFile = fopen("destinationFile.bin", "wb")) == NULL){ //changed to write binary
            printf("invalid destination file\n");
            exit(1);
        }
        fwrite(data, numBytes, 1, destinationFile);
        fclose(destinationFile);
    }
    else{
        decompress("outputFile.bin", numBytes, compressedSize, "destinationFile.bin");
        printf("decompression successful!\n");
    }

    
    printf("Compression took %f seconds to process %d bytes\n", (double)(end - begin) / CLOCKS_PER_SEC, numBytes );
    
    return 1;

    /* 
    * This section of code is used for testing the compression ratio as a function of large input sizes.
    * Data was passed to a text file via IO piping in terminal, and then processed in Excel
    uint8_t num_tests = 46;
    uint16_t tempRes[3];
    float avg;

    for (int i = 0; i< num_tests; i++){
        fill_with_pseudorandom_letters(&data, numBytes, 12345);
        //compression_ratio[i] = compress("outputFile.bin", &data, numBytes);
        tempRes[0] = compress("outputFile.bin", &data, i*1000);
        fill_with_pseudorandom_letters(&data, numBytes, 31415);
        tempRes[1] = compress("outputFile.bin", &data, i*1000);
        fill_with_pseudorandom_letters(&data, numBytes, 27182);
        tempRes[2] = compress("outputFile.bin", &data, i*1000);
        avg = (tempRes[0] + tempRes[0] + tempRes[0]) / 3;
        

        //printf("Input size:%d, average compression size:%f\n", i*100, avg);
        printf("%d;%f\n", i*1000, avg);

    }*/

    /*uint8_t num_tests = 1001;
    uint16_t tempRes[3];
    float avg;

    for (int i = 0; i< num_tests; i++){
        fill_with_pseudorandom_letters(&data, numBytes, 12345);
        //compression_ratio[i] = compress("outputFile.bin", &data, numBytes);
        tempRes[0] = compress("outputFile.bin", &data, i);
        fill_with_pseudorandom_letters(&data, numBytes, 31415);
        tempRes[1] = compress("outputFile.bin", &data, i);
        fill_with_pseudorandom_letters(&data, numBytes, 27182);
        tempRes[2] = compress("outputFile.bin", &data, i);
        avg = (tempRes[0] + tempRes[0] + tempRes[0]) / 3;
        

        //printf("Input size:%d, average compression size:%f\n", i*100, avg);
        printf("%d;%f\n", i, avg);

    }*/


}

int main1(void){

    uint16_t numBytes = 49152; //2^11 * 6 floats of *4 bites. Needs to be smaller than 2^16 or change to uint32
    //uint16_t numBytes = 100;
    //uint16_t numBytes = 20;
    uint8_t data[numBytes];

    uint32_t compressedData[(int)ceil(numBytes / 4)];

    //fill_with_pseudorandom_letters(&data, numBytes, 12345);
    fill_with_ascending_numbers(&data, numBytes);
    //read_data_from_file("/home/sebastian/EEE3097S/EEE3097_Compression/processData/pData.bin", data, numBytes);

    //int32_t compressedSize = compress("outputFile.bin", &data, numBytes);
    int32_t compressedSize = compress_to_array(&compressedData, &data, numBytes);
    printf("flag1\n");
    dump_buf("output", &compressedData, compressedSize);
    compressedSize = compress("outputFile.bin", &data, numBytes);
    printf("compression successful!\n");

    /*if (compressedSize == -1){

        printf("compressed file larger than input, return input\n");
        FILE *destinationFile;

        if ((destinationFile = fopen("destinationFile.bin", "wb")) == NULL){ //changed to write binary
            printf("invalid destination file\n");
            exit(1);
        }
        fwrite(data, numBytes, 1, destinationFile);
        fclose(destinationFile);
    }
    else{
        decompress("outputFile.bin", numBytes, compressedSize, "destinationFile.bin");
        printf("decompression successful!\n");
    }*/
    
    return 1;
}

