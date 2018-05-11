#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int key; /* Record's key */ long off; /* Record's offset in file */ } index_S;
typedef struct { int siz; /* Hole's size */ long off; /* Hole's offset in file */ } avail_S;

void readIndexBuf(index_S** index);
void readAvailBuf(avail_S** avail);
int fileLength(const char* file);
char** partitionInput(char* input);
int findIndexOffset(index_S* index, int left, int right, int target);
index_S* reallocIndex(index_S* index, int size);
avail_S* reallocAvail(avail_S* avail, int size);
long writeInFile(FILE* file, char newIndex[], long offset);
int findAvailHole(avail_S* avail, int rec_siz, int totalAvail);
void updateAvailList(avail_S* avail, int record_length, int availHole, int totalAvail, int fitMethod);
char* readFromIndex(FILE* fp, long offset);
void deleteIndex(index_S* index, int indexPos, int totalIndex);
void saveIndex(index_S* index, int rec_len);
void saveAvail(avail_S* avail, int avail_len);
void printIndex(index_S* index, int totalIndex);
void printAvail(avail_S* avail, int totalAvail);
int comparatorBest(const void* a, const void* b);
int comparatorWorst(const void* a, const void* b);
int indexComparator(const void* a, const void* b);


int main(int argc, char* argv[]){
    int fitMethod;
    char add[] = "add";
    char find[] = "find";
    char del[] = "del";
    char end[] = "end\n";
    if(argc < 3){
        printf("Wrong Argument: [script][fitMethod][file]");
        return 0;
    }
    fitMethod = strcmp(argv[1], "--first-fit") == 0 ? 0 : strcmp(argv[1], "--best-fit") == 0 ? 1 : strcmp(argv[1], "--worst-fit") == 0 ? 2 : -1;
    if(fitMethod == -1){
        printf("Wrong fitMethod: [--first-fit][--best-fit][--worst-fit]");
        return 0;
    }
    FILE *fp;
    int totalIndex = 0, totalAvail = 0;
    index_S *index = NULL;
    avail_S *avail = NULL;
   if ( ( fp = fopen( argv[2], "r+b" ) ) == NULL){
        fp = fopen( argv[2], "w+b" );
    }else{
        readIndexBuf(&index);
        readAvailBuf(&avail);
        totalIndex = fileLength("index.bin");
        totalAvail = fileLength("avail.bin");
    }
    long offset = -1;
    while(1){
        char *input = NULL;
        size_t len = 0;
        ssize_t read;
        read = getline(&input, &len, stdin);
        if(read == -1) continue;
        char **inputs = partitionInput(input);
        if(strcmp(add, inputs[0]) == 0){
            int target = atoi(inputs[1]);
            if(findIndexOffset(index, 0, totalIndex-1, target) != -1){
                printf("Record with SID=%d exists\n", target);
            }else{
                int availHole = findAvailHole(avail, (int)(sizeof(target)+strlen(inputs[2]))-1, totalAvail);
                if(availHole != -1){
                    offset = avail[availHole].off;
                    updateAvailList(avail, (int)(sizeof(target)+strlen(inputs[2]))-1, availHole, totalAvail, fitMethod);                    
                }else{
                    offset = -1;
                }
                char newIndex[(int)strlen(inputs[2])];
                strncpy(newIndex, inputs[2], (int)(strlen(inputs[2]))-1);
                newIndex[(int)(strlen(inputs[2]))-1] = '\0';
                index_S* tmp = reallocIndex(index,++totalIndex);
                if(!tmp) exit(1);
                index = tmp;
                if(offset != -1)
                    fseek(fp, offset, SEEK_SET);
                else{
                    fseek(fp, 0, SEEK_END);
                    offset = ftell(fp);                    
                }
                int num = (int)(sizeof(int)+strlen(newIndex));
                fwrite(&num, sizeof(int), 1, fp);
                fwrite(newIndex, sizeof(char), strlen(newIndex), fp);
                fseek(fp, 0, SEEK_SET);         
                index[totalIndex-1].key = target;
                index[totalIndex-1].off = offset;
                qsort(index, (size_t)totalIndex, sizeof(index_S), indexComparator);
            }
        }else if(strcmp(inputs[0], find) == 0){
            int target = atoi(inputs[1]);
            int indexPos = findIndexOffset(index, 0, totalIndex-1, target);
            if(indexPos != -1){
                char* rec = readFromIndex(fp, index[indexPos].off);
                printf("%s\n", rec);               
            }else{
                printf("No record with SID=%d exists\n", target);
            }
        }else if(strcmp(inputs[0], del) == 0){
            int target = atoi(inputs[1]);
            int indexPos = findIndexOffset(index, 0, totalIndex-1, target);
            if(indexPos != -1){
                int rec_siz;
                fseek(fp, index[indexPos].off, SEEK_SET);
                fread(&rec_siz, sizeof(int), 1, fp);
                avail_S* tmp = reallocAvail(avail, ++totalAvail);
                if(!tmp) exit(1);
                avail = tmp;
                avail[totalAvail-1].off = index[indexPos].off;
                avail[totalAvail-1].siz = rec_siz;
                if(fitMethod)
                    qsort(avail, (size_t)totalAvail, sizeof(avail_S), fitMethod == 1 ? comparatorBest : comparatorWorst);
                deleteIndex(index, indexPos, totalIndex);
                totalIndex--;            
            }else{
                printf("No record with SID=%d exists\n", target);
            }
        }else if(strcmp(end, inputs[0]) == 0){
            if(input) free(input);
            if(inputs) free(inputs);
            break;
        }else{
            printf("Invalid Argument");
            exit(1);
        }
    }
    printIndex(index, totalIndex);
    printAvail(avail, totalAvail);
    saveIndex(index, totalIndex);
    saveAvail(avail, totalAvail);
    fclose(fp);
    if(index) free(index);
    if(avail) free(avail);
    return 0;
}

void readIndexBuf(index_S** index){
    FILE *file = fopen("index.bin", "rb");
    fseek(file, 0, SEEK_END);
    size_t size = (size_t)ftell(file);
    *index = malloc(size);
    fseek(file, 0, SEEK_SET);
    fread(*index, sizeof(index_S), size/sizeof(index_S), file);
    fclose(file); 
}

void readAvailBuf(avail_S** avail){
    FILE *file = fopen("avail.bin", "rb");
    fseek(file, 0, SEEK_END);
    size_t size = (size_t)ftell(file);
    *avail = malloc(size);
    fseek(file, 0, SEEK_SET);
    fread(*avail, sizeof(avail_S), size/sizeof(avail_S), file);
    fclose(file);     
}

int fileLength(const char* file){
    int res = 0;
    FILE *in = fopen(file, "rb");
    fseek(in, 0, SEEK_END);
    res = (int)((size_t)ftell(in)/sizeof(index_S));
    fclose(in);
    return res;   
}

char** partitionInput(char* input){
    char** res = malloc(3*sizeof(char*));
    char* pch, running;
    const char delimiters[] = " ";
    int i = 0;
    while(i < 3){
        pch = strsep(&input, delimiters); 
        if(pch != NULL && strcmp(pch, "") == 0)
            continue;
        res[i++] = pch;
    }
    return res;
}

int findIndexOffset(index_S* index, int left, int right, int target){
    while(left <= right){
        int mid = left+(right-left)/2;
        if(index[mid].key == target)
            return mid;
        else if(index[mid].key < target)
            left = mid+1;
        else
            right = mid-1;
    }
    return -1;
}

index_S* reallocIndex(index_S* index, int size){
    index_S *tmp = realloc(index, (size)*sizeof(index_S));
    return tmp;
}

avail_S* reallocAvail(avail_S* avail, int size){
    avail_S* tmp = realloc(avail, (size)*sizeof(avail_S));
    return tmp;
}

int findAvailHole(avail_S* avail, int rec_siz, int totalAvail){
    int i = 0;
    while(i < totalAvail){
        if(rec_siz < avail[i].siz)
            return i;
        i++;
    }
    return -1;
}

void updateAvailList(avail_S* avail, int rec_siz, int availHole, int totalAvail, int fitMethod){
    int hole_siz = avail[availHole].siz;
    if(hole_siz < rec_siz){
        printf("ERROR, NO AVAILABLE HOLE");
        exit(1);
    }
    long offset = avail[availHole].off;
    int i;
    for(i = availHole; i < totalAvail-1; ++i) 
        avail[i] = avail[i+1];
    if(rec_siz < hole_siz){
        avail[totalAvail-1].off = offset+rec_siz;
        avail[totalAvail-1].siz = hole_siz-rec_siz;        
    }else{
        avail_S* tmp = reallocAvail(avail, sizeof(avail_S)*(--totalAvail));
        if (!tmp && totalAvail) exit(1);
        avail = tmp;        
    }
    if(fitMethod)
        qsort(avail, (size_t)totalAvail, sizeof(avail_S), fitMethod == 1 ? comparatorBest : comparatorWorst);
}

char* readFromIndex(FILE* file, long offset){
    fseek(file, offset, SEEK_SET);
    int rec_siz = 0;
    fread(&rec_siz, sizeof(int), 1, file);
    fseek(file, sizeof(int)+offset, SEEK_SET);
    char* res = malloc((size_t)rec_siz-sizeof(int)+1);
    fread(res, sizeof(char), (size_t)rec_siz-sizeof(int), file);
    return res;
}

void printIndex(index_S* index, int totalIndex){
    int i;
    printf("Index:\n");
    for(i = 0; i < totalIndex; ++i)
        printf( "key=%d: offset=%ld\n", index[i].key, index[i].off );
}

void printAvail(avail_S* avail, int totalAvail){
    int i, hole_siz = 0;
    printf("Availability:\n");
    for(i = 0; i < totalAvail; ++i){
        hole_siz += avail[i].siz;
        printf( "size=%d: offset=%ld\n", avail[i].siz, avail[i].off );
    }
    printf( "Number of holes: %d\n", totalAvail );
    printf( "Hole space: %d\n", hole_siz );
}

void deleteIndex(index_S* index, int indexPos, int totalIndex){
    int i;
    for(i = indexPos; i < totalIndex-1; ++i) 
        index[i] = index[i+1];
    index_S* tmp = reallocIndex(index, --totalIndex);
    if (!tmp && totalIndex) exit(1);
    index = tmp;
}

void saveIndex(index_S* index, int rec_len){
    FILE *out = fopen("index.bin", "wb");
    fwrite(index, sizeof(index_S), (size_t)rec_len, out);
    fclose(out);
}
void saveAvail(avail_S* avail, int avail_len){
    FILE* out = fopen("avail.bin", "wb");
    fwrite(avail, sizeof(avail_S), (size_t)avail_len, out);
    fclose(out);
}

int comparatorBest(const void* a, const void* b){
    return (((avail_S*)a)->siz - ((avail_S*)b)->siz);
}

int comparatorWorst(const void* a, const void* b){
    return (((avail_S*)b)->siz - ((avail_S*)a)->siz);
}

int indexComparator(const void* a, const void* b){
    return (((index_S*)a)->key - ((index_S*)b)->key);
}





