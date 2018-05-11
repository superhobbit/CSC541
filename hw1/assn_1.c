#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int getLength(FILE *file);
int* memLinear(int *keyArray, int *seekArray, int keyLength, int seekLength);
int* memBinary(int *keyArray, int *seekArray, int keyLength, int seekLength);
int* diskLinear(FILE *keyFile, int *seekArray, int keyLength, int seekLength);
int* diskBinary(FILE *keyFile, int *seekArray, int keyLength, int seekLength);

int main(int argc, char *argv[]){
    if(argc != 4){
        printf("Wrong Argument\n");
        return 0;
    }
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    FILE *keyFile = fopen(argv[2], "rb");
    FILE *seekFile = fopen(argv[3], "rb");
    int keyLength = getLength(keyFile);
    int seekLength = getLength(seekFile);
    if(keyLength == -1 || seekLength == -1)
        return 0;
    int *seekArray = (int *)malloc(seekLength*sizeof(int));
    fread(seekArray, sizeof(int), seekLength, seekFile);
    fclose(seekFile);
    int *hit = (int *)malloc(seekLength*sizeof(int));
    if(strcmp(argv[1], "--mem-lin") == 0){
        int *keyArray = (int *)malloc(keyLength*sizeof(int));
        fread(keyArray, sizeof(int), keyLength, keyFile);
        fclose(keyFile);
        hit = memLinear(keyArray, seekArray, keyLength, seekLength);
        free(keyArray);
    }else if(strcmp(argv[1], "--mem-bin") == 0){
        int *keyArray = (int *)malloc(keyLength*sizeof(int));
        fread(keyArray, sizeof(int), keyLength, keyFile);
        fclose(keyFile);
        hit = memBinary(keyArray, seekArray, keyLength, seekLength);
        free(keyArray);
    }else if(strcmp(argv[1], "--disk-lin") == 0){
        hit = diskLinear(keyFile, seekArray, keyLength, seekLength);
    }else if(strcmp(argv[1], "--disk-bin") == 0){
        hit = diskBinary(keyFile, seekArray, keyLength, seekLength);
    }else{
        printf("Wrong Parameter\n");
        return 0;
    }
    int i;
    for(i = 0; i < seekLength; ++i){
        if(hit[i]){
            printf("%12d: Yes\n", seekArray[i]);
        }else{
            printf("%12d: No\n", seekArray[i]);
        }
    }
	free(hit);
    free(seekArray);
    gettimeofday(&end_time, NULL);
    long tmp = end_time.tv_usec-start_time.tv_usec;
    if(tmp < 0)
        tmp -= tmp*2;
    printf("Time: %ld.%06ld", end_time.tv_sec - start_time.tv_sec,tmp);
    return 0;
};

int getLength(FILE *file){
    if(!file)
        return -1;
    fseek(file, 0, SEEK_END);
    int res = (ftell(file))/4;
    rewind(file);
    return res;
}

int* memLinear(int *keyArray, int *seekArray, int keyLength, int seekLength){
    int i, j;
    int *res = (int *)malloc(seekLength*sizeof(int));
    for(i = 0; i < seekLength; ++i){
        for( j = 0; j < keyLength; ++j){
            if(keyArray[j] == seekArray[i]){
                res[i] = 1;
                break;
            }
        }
        if(res[i] != 1)
            res[i] = 0;
    }
    return res;
};

int* memBinary(int *keyArray, int *seekArray, int keyLength, int seekLength){
    int i;
    int *res = (int *)malloc(seekLength*sizeof(int));
    for(i = 0; i < seekLength; ++i){
        int mid, left = 0, right = keyLength-1, tar = seekArray[i];
        while(left <= right){
            mid = left+((right-left) >> 1);
            if(keyArray[mid] == tar){
                res[i] = 1;
                break;
            }else if(keyArray[mid] < tar)
                left = mid+1;
            else
                right = mid-1;
        }
        if(res[i] != 1)
            res[i] = 0;
    }
    return res;
}

int* diskLinear(FILE *keyFile, int *seekArray, int keyLength, int seekLength){
    int i, j, tmp;
    int *res = (int *)malloc(seekLength*sizeof(int));
    for(i = 0; i < seekLength; ++i){
        fseek(keyFile, 0, SEEK_SET);
        for( j = 0; j < keyLength; ++j){
            fread(&tmp, sizeof(int), 1, keyFile);
            if(tmp == seekArray[i]){
                res[i] = 1;
                break;
            }
        }
        if(res[i] != 1)
            res[i] = 0;
    }
    return res;
}

int* diskBinary(FILE *keyFile, int *seekArray, int keyLength, int seekLength){
    int i, tmp;
    int *res = (int *)malloc(seekLength*sizeof(int));
    for(i = 0; i < seekLength; ++i){
        int mid, left = 0, right = keyLength-1, tar = seekArray[i];
        while(left <= right){
            mid = left+((right-left) >> 1);
            fseek(keyFile, mid*sizeof(int), SEEK_SET);
            fread(&tmp, sizeof(int), 1, keyFile);
            if(tmp == tar){
                res[i] = 1;
                break;
            }else if(tmp < tar)
                left = mid+1;
            else
                right = mid-1;
        }
        if(res[i] != 1)
            res[i] = 0;
    }
    return res;
}

