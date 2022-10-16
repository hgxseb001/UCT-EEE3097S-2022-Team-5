#include <stdint.h>
#include <ctype.h>

#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"
#include "greatest.h"

#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif

/* The majority of the tests are in test_heatshrink_dynamic, because that allows
 * instantiating encoders/decoders with different settings at run-time. */

static heatshrink_encoder hse;
static heatshrink_decoder hsd;

struct dataPoints{
float ax, ay, az, gx, gy, gz
};



static void fill_with_pseudorandom_letters(uint8_t *buf, uint16_t size, uint32_t seed) {
    uint64_t rn = 9223372036854775783; /* prime under 2^64 */
    for (int i=0; i<size; i++) {
        rn = rn*seed + seed;
        buf[i] = (rn % 26) + 'a';
    }
}

static void fill_with_ascending_numbers(uint8_t *buf, uint16_t size) {
    for (int i=0; i<size; i++) {
        buf[i] = i;
    }
}

static void read_data_from_file1(char *fileLoc, struct dataPoints *data, uint16_t size){

    FILE *inputFile;

    if ((inputFile = fopen(fileLoc, "rb")) == NULL){
        printf("invalid input file\n");
        exit(1);
    }

   for (int i = 0; i<size; i++){
      fread(&data[i], sizeof(struct dataPoints), 1, inputFile);   
      printf("%f %f %f %f %f %f\n", data[i].ax, data[i].ay, data[i].az, data[i].gx, data[i].gy, data[i].gz); 
    }

  fclose(inputFile);
  return data;

}
static struct dataPoints * read_data_from_file(char *fileLoc, uint8_t *data, uint16_t numBytes){

    FILE *inputFile;

    if ((inputFile = fopen(fileLoc, "rb")) == NULL){
        printf("invalid input file\n");
        exit(1);
    }

   for (int i = 0; i<numBytes; i++){
      fread(&data, sizeof(uint8_t), numBytes, inputFile);   
      //printf("%f %f %f %f %f %f\n", data[i].ax, data[i].ay, data[i].az, data[i].gx, data[i].gy, data[i].gz); 
    }

    fclose(inputFile);

}

static void dump_buf(char *name, uint8_t *buf, uint16_t count) {
    for (int i=0; i<count; i++) {
        uint8_t c = (uint8_t)buf[i];
        printf("%s %d: 0x%02x ('%c')\n", name, i, c, isprint(c) ? c : '.');
    }
}

static int compress(char* output_file_name, uint8_t *input, uint32_t input_size){

    FILE *outputFile;
    //dump_buf("input", input, input_size);

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
    }

    //fwrite(&comp, sizeof(comp), 1, outputFile);
    fwrite(&comp, comp_sz, 1, outputFile);
    fclose(outputFile);

    printf("in: %u compressed: %u \n", input_size, polled);

    //dump_buf("compressedData", &comp, sizeof(comp));
    dump_buf("Compressed", &comp, polled);
    return polled;

    free(comp);
    //PASS();
}

static void decompress(char* source_file_name, uint16_t input_size, uint32_t compressed_size, char* destination_file_name){

    FILE *sourceFile;

    if ((sourceFile = fopen(source_file_name, "rb")) == NULL){
        printf("invalid input file\n");
        exit(1);
    }

    fseek(sourceFile, 0, SEEK_END);
    uint32_t fLen = ftell(sourceFile);
    fseek(sourceFile, 0, SEEK_SET);

    heatshrink_decoder_reset(&hsd);

    //uint32_t compressed_size = (polled1);
    //uint32_t compressed_size = polled1;


    size_t comp_sz = compressed_size;
    //size_t comp_sz =
    //printf("%d", comp_sz);
    size_t decomp_sz = input_size + (input_size/2) + 4;

    //printf("flag0\n");

    uint8_t *comp = malloc(comp_sz);
    uint8_t *decomp = malloc(decomp_sz);

    memset(comp, 0, comp_sz);
    memset(decomp, 0, decomp_sz);


    printf("flag0\n");

    /*uint8_t comp1 [polled1];

    for (int i=0; i<polled1; i++){
        scanf(sourceFile, "%d", comp1[i]);
        printf("%d", comp1[i]);
    }*/

    //fread(&comp, sizeof(uint8_t), 314, sourceFile); ///////////////////////////////////////Potential error
    fread(&comp, sizeof(comp), 1, sourceFile);

    fclose(sourceFile);

    printf("flag1\n");

    dump_buf("Compressed", &comp, comp_sz);

    size_t count = 0;

    uint32_t sunk = 0;
    uint32_t polled = 0;

    while (sunk < compressed_size) {
        //printf("%d\n", sunk);
        //ASSERT(heatshrink_decoder_sink(&hsd, &comp[sunk], compressed_size - sunk, &count) >= 0);
        heatshrink_decoder_sink(&hsd, &comp[sunk], compressed_size - sunk, &count);
        sunk += count;
        //if (log_lvl > 1) printf("^^ sunk %zd\n", count);
        if (sunk == compressed_size) {
            //ASSERT_EQ(HSDR_FINISH_MORE, heatshrink_decoder_finish(&hsd));
            heatshrink_decoder_finish(&hsd);
        }

        HSD_poll_res pres;
        do {
            pres = heatshrink_decoder_poll(&hsd, &decomp[polled],
                decomp_sz - polled, &count);
            //ASSERT(pres >= 0);
            polled += count;
            //if (log_lvl > 1) printf("^^ polled %zd\n", count);
        } while (pres == HSDR_POLL_MORE);
        //ASSERT_EQ(HSDR_POLL_EMPTY, pres);
        if (sunk == compressed_size) {
            HSD_finish_res fres = heatshrink_decoder_finish(&hsd);
            //ASSERT_EQ(HSDR_FINISH_DONE, fres);
        }

    }

    FILE *destinationFile;

    if ((destinationFile = fopen(destination_file_name, "wb")) == NULL){
        printf("invalid output file\n");
        exit(1);
    }

    //dump_buf("output", decomp, input_size);

    /*for (int i=0; i< input_size; i++){
        uint8_t c = (uint8_t)decomp[i];
    fprintf(destinationFile, "%d\n", c);

    }*/
    /*for (int i=0; i< input_size; i+=4){
        float c = (float)decomp[i];
    fprintf(destinationFile, "%d\n", c);*/

    fwrite(&decomp, sizeof(decomp), 1, destinationFile);

    fclose(destinationFile);

    

    free(comp);
    free(decomp);
    //PASS();

}


int main(void){
    //uint16_t size = 2000;
    //uint8_t buffer[size];

    uint16_t readSize = 100;

    //struct dataPoints data[readSize];
    uint16_t numBytes = 100 * 24;
    uint8_t data[numBytes];
    
    read_data_from_file("/home/sebastian/EEE3097_Compression/processData/pData.bin", &data, numBytes);



    //printf("%d\n", sizeof(data));
    //printf("%d\n", readSize);


    //printf("Readtest\n");
    //read_data_from_file("/home/sebastian/EEE3097_Compression/processData/pData.bin",100);

    //fill_with_pseudorandom_letters(&buffer, size, 12345);
    //fill_with_ascending_numbers(&buffer, size);


    //uint32_t polled = compress("outputFile.bin", &buffer, size);
    uint32_t compressedSize = compress("outputFile.bin", &data, numBytes);
    //printf("%d\n",  polled);


    printf("compression successful!\n");

    decompress("outputFile.bin", sizeof(data), compressedSize, "destinationFile.bin");

    printf("decompression successful!\n");
    //return 1;

}
