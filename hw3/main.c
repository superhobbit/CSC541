#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#define N 1000
#define H 750
#define B 250

void basic(char* inFile, char* outFile);
void multiStep(char* inFile, char* outFile);
void replacement(char* inFile, char* outFile);
int calculateRuns(char* inFile);
void writeRuns(int len, char* fileName, int i, FILE* inputFile);
void merge(int start, int runs, FILE* outfile, char* inputFile, int appendNum, char* appendString);
char* concatanateString(char* fileName, char* appendString, int appendNum, int i, char* inputFile);
void lessThanN(char* inputFile, char* outputFile, int len);
void heapify(int* heap, int i, int n);
void heapSort(int* heap, int n);
int fileLength(FILE* in);
int compareFunc(const void* a, const void* b);



int main(int argc, char *argv[]){
    struct timeval start, end, exec;
    char* input = strdup(argv[2]);
    gettimeofday(&start, NULL);
    if(strcmp(argv[1], "--basic") == 0)
        basic(input, argv[3]);
    else if(strcmp(argv[1], "--multistep") == 0)
        multiStep(input, argv[3]);
    else if(strcmp(argv[1], "--replacement") == 0)
        replacement(input, argv[3]);
    else{
        printf("Wrong Argument\n");
        return 0;
    }
    gettimeofday(&end, NULL);
    exec.tv_sec = end.tv_sec-start.tv_sec;
    exec.tv_usec = end.tv_usec-start.tv_usec;
    if(exec.tv_usec < 0){
        exec.tv_usec += 1000000;
        exec.tv_sec--;
    }
    printf( "Time: %ld.%06ld\n", exec.tv_sec, exec.tv_usec);
    return 0;
}

void basic(char* inFile, char* outFile){
    FILE* out = fopen(outFile, "w");
    int runs = calculateRuns(inFile);
    merge(0, runs, out, inFile, 4, ".%03d");
    fclose(out);
}

void multiStep(char* inFile, char* outFile){
    int runs = calculateRuns(inFile);
    int superRuns = runs/15 + (runs%15 ? 1 : 0);
    char *filename = malloc((strlen(inFile) + 10) * sizeof(char));
    char* appendString = ".super.%03d";
    int i;
    for(; i < superRuns - 1; ++i){
        filename = concatanateString(filename, appendString, 10, i, inFile);
        FILE *super_run_file = fopen(filename, "w");
        merge(i*15, 15, super_run_file, inFile, 4, ".%03d");
        fclose(super_run_file);
    }
    filename = concatanateString(filename, appendString, 10, superRuns-1, inFile);
    FILE *super_run_file = fopen(filename, "w");
    merge((superRuns-1) * 15, runs - 15 * (superRuns - 1), super_run_file, inFile, 4, ".%03d");
    fclose(super_run_file);
    free(filename);
    FILE *outfile = fopen(outFile, "w");
    merge(0, superRuns, outfile, inFile, 10, appendString);
}

void replacement(char* inFile, char* outFile){
    FILE *inp = fopen(inFile, "r");
    int len = fileLength(inp);
    if(len <= N){
        lessThanN(inFile, outFile, len);
        return;
    }
    int *heap = malloc(H * sizeof(int));
    int *buffer = malloc(B * sizeof(int));
    int *output = malloc(N * sizeof(int));
    int outIndex = 0;
    int bufferIndex = 0;
    int heapSize = H;
    char *filename = malloc((strlen(inFile) + 4) * sizeof(char));
    int fileIndex = 0;
    char *index = malloc(4 * sizeof(char));
    sprintf(index, ".%03d", fileIndex);
    strcpy(filename, inFile);
    strcat(filename, index);
    FILE *curr = fopen(filename, "w");
    fread(heap, sizeof(int), H, inp);
    fread(buffer, sizeof(int), B, inp);
    int left = len;
    left -= N;
    int lastPart = 0, lastBuffer = 0;
    heapSort(heap, H);
    while(1){
        if(!lastPart || (lastPart && bufferIndex != lastPart)){
            if(heapSize > 0){
                output[outIndex++] = heap[0];
                if(heap[0] > buffer[bufferIndex]){
                    heap[0] = heap[heapSize-1];
                    heap[heapSize-1] = buffer[bufferIndex++];
                    if(bufferIndex == B){
                        int readSize = (left >= B ? B : left);
                        left -= readSize;
                        if(!left && !lastPart){
                            lastPart = readSize;
                        } 
                        if(readSize){
                            fread(buffer, sizeof(int), readSize, inp);
                            bufferIndex = 0;                            
                        }
                    }
                    heapSort(heap, --heapSize);
                }else{
                    heap[0] = buffer[bufferIndex++];
                    if(bufferIndex == B){
                        int readSize = (left >= B ? B : left);
                        left -= readSize;
                        if(!left && !lastPart) lastPart = readSize;
                        if(readSize){
                            fread(buffer, sizeof(int), readSize, inp);
                            bufferIndex = 0;                            
                        }
                    }
                    heapSort(heap, heapSize);
                }
                if(outIndex == N){
                    fwrite(output, sizeof(int), N, curr);
                    outIndex = 0;
                }
            }else{
                if(outIndex > 0){
                    fwrite(output, sizeof(int), outIndex, curr);
                    outIndex = 0;
                }
                fclose(curr);
                sprintf(index, ".%03d", ++fileIndex);
                strcpy(filename, inFile);
                strcat(filename, index);
                curr = fopen(filename, "w");
                heapSize = H;
                heapSort(heap, heapSize);
            }
        }else{
            if(heapSize > 0){
                lastBuffer++;
                output[outIndex++] = heap[0];
                if(outIndex == N){
                    fwrite(output, sizeof(int), N, curr);
                    outIndex = 0;
                }
                heap[0] = heap[--heapSize];
                heapSort(heap, heapSize);
            }else{
                if(outIndex > 0){
                    fwrite(output, sizeof(int), outIndex, curr);
                    outIndex = 0;
                }
                fclose(curr);
                sprintf(index, ".%03d", ++fileIndex);
                strcpy(filename, inFile);
                strcat(filename, index);
                curr = fopen(filename, "w");
                qsort(&heap[lastBuffer], (size_t) (H-lastBuffer), sizeof(int), compareFunc);
                fwrite(&heap[lastBuffer], sizeof(int), (H-lastBuffer), curr);
                fclose(curr);
                break;
            }
        }
    }
    if(outIndex > 0){
        fwrite(output, sizeof(int), outIndex, curr);
        fclose(curr);
    }
    FILE* out = fopen(outFile, "w");
    merge(0, ++fileIndex, out, inFile, 4, ".%03d");
}

int calculateRuns(char* input){
    FILE* inFile = fopen(input, "r");
    int len = fileLength(inFile);
    int runs = len/N + (len%N ? 1 : 0);
    int i = 0;
    for(; i < runs-1; ++i)
        writeRuns(N, input, i, inFile);
    writeRuns(len-(N*(runs-1)), input, runs-1, inFile);
    fclose(inFile);
    return runs;
}

void writeRuns(int len, char* fileName, int i, FILE* inputFile){
    char* target = malloc((strlen(fileName)+4)*sizeof(char));
    char* chunk = malloc(len*sizeof(int));
    fread(chunk, sizeof(int), len, inputFile);
    qsort(chunk, len, sizeof(int), compareFunc);
    char *index = malloc(4 * sizeof(char));
    sprintf(index, ".%03d", i);
    strcpy(target, fileName);
    strcat(target, index);
    FILE* fp = fopen(target, "w");
    fwrite(chunk, sizeof(int), len, fp);
    free(target);
    free(chunk);
    free(index);
    fclose(fp);    
}

void merge(int start, int runs, FILE *outfile, char* inputFile, int appendNum, char* appendString){
    int *output = malloc(N*sizeof(int));
    char *fileName = malloc((appendNum+strlen(inputFile))*sizeof(char));
    strcpy(fileName, inputFile);
    strcat(fileName, appendString);
    int i = 0;
    FILE *files[runs];
    for(; i < runs; ++i){
        char* index = malloc((appendNum+strlen(inputFile))*sizeof(char));
        sprintf(index, fileName, i+start);
        files[i] = fopen(index, "r");
        free(index);
    }
    int* inputs[runs];
    for( i = 0; i < runs; i++ ){
        inputs[i] = malloc(8*sizeof(int));
        fread(inputs[i], sizeof(int), 8, files[i]);
    }  
    int outIndex = 0, emptyFile = 0;
    while(emptyFile < runs){
        int minV = INT_MAX, minVIndex = -1;
        int i = 0;
        for(; i < runs; ++i){
            int j = 0, tmp = -1;
            for(; j < 8; ++j){
                if(inputs[i][j] != -1){
                    tmp = inputs[i][j];
                    break;
                }
            }
            if(tmp != -1){
                if(tmp < minV){
                    minVIndex = i;
                    minV = tmp;
                }
            }else if(files[i]){
                if(feof(files[i])){
                    emptyFile++;
                    files[i] = NULL;
                    continue;
                }
                fread(inputs[i], sizeof(int), 8, files[i]);
                int k = 0;
                while(k < 8){
                    if(inputs[i][k] != -1){
                        tmp = inputs[i][k];
                        break;
                    }
                    k++;
                }
                if(tmp != -1 && tmp < minV){
                    minVIndex = i;
                    minV = tmp;
                }else if(tmp == -1){
                    emptyFile++;
                    files[i] = NULL;
                }
            }
        }
        if(minVIndex != -1){
            i = 0;
            for(; i < 8; ++i){
                if(inputs[minVIndex][i] != -1){
                    inputs[minVIndex][i] = -1;
                    break;
                }                
            }
            output[outIndex++] = minV;
            if(outIndex >= N){
                outIndex = 0;
                fwrite(output, sizeof(int), N, outfile);
            }
        }
    }
    if(outIndex != 0){
        fwrite(output, sizeof(int), (size_t)outIndex, outfile);
    }
    i = 0;
    for(; i < runs; ++i){
        free(inputs[i]);
        inputs[i] = NULL;
    }
    free(output);
    free(fileName);
}

char* concatanateString(char* fileName, char* appendString, int appendNum, int i, char* inputFile){
    char *index = malloc(appendNum * sizeof(char));
    sprintf(index, appendString, i);
    strcpy(fileName, inputFile);
    strcat(fileName, index);  
    free(index);
    return fileName;
}

void lessThanN(char* inputFile, char* outputFile, int len){
    FILE* in = fopen(inputFile, "r");
    int* input = malloc(len*sizeof(int));
    fread(input, sizeof(int), len, in);
    qsort(input, len, sizeof(int), compareFunc);
    FILE* out = fopen(outputFile, "w");
    fwrite(input, sizeof(int), len, out);
    fclose(in); fclose(out);
}

void heapify(int *heap, int i, int n){
    int l = 2*i+1;
    int r = 2*i+2;
    int smallest = i;
    if (l < n && heap[l] < heap[i])
        smallest = l;
    if (r < n && heap[r] < heap[smallest])
        smallest = r;
    if (smallest != i){
        int tmp = heap[i];
        heap[i] = heap[smallest];
        heap[smallest] = tmp;
        heapify(heap, smallest, n);
    }
}

void heapSort(int* heap, int n){
    int i = n/2;
    for(; i >= 0; --i)
        heapify(heap, i, n);
}

int fileLength(FILE *in){
    fseek(in, 0, SEEK_END);
    int res = (int)((size_t)ftell(in)/sizeof(int));
    rewind(in);
    return res;
}

int compareFunc(const void* a, const void* b){
    return (*(int*)a - *(int*)b);
}

