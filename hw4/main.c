#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define split(x) (x+1)/2

typedef struct { /* B-tree node */ int n; /* Number of keys in node */ int *key; /* Node's keys */ long *child; /* Node's child subtree offsets */ } btree_node;
typedef struct q_element{
    long offset;
    struct q_element* next;
}q_element;
long find(int key, int order, FILE* in, long* path, int* nodesCount, int print);
void add(int key, int order, FILE *in);
int splitNode(int* left, int* right, int mid, btree_node* target, int order, int nodeLength, int key);
long search(FILE* in, long* path, int* status, int* nodesCount, int key, long off, int order);
char** partitionInput(char* input);
void writer(btree_node *node, long off, FILE *in, int order);
void updateNode(btree_node* node, int* newNode, int n, int mid);
void addHelper(FILE* in, long new_off, int order, int key, int nodesCount, long* path);
void copyNodes(int nodeLength, int position, long* res, btree_node* node, long insert);
void printer(FILE *in, int order);
int compare(const void * a, const void * b);
int reader(btree_node *node, long off, int order, FILE *in);
btree_node* makeNewNode(int order);
long inNodeSearch(btree_node *node, int key, int nodeLength, long off, int *status);

int main(int argc, char *argv[]) {
    int order = atoi(argv[2]);
    btree_node* node = makeNewNode(order);
    char *filename = argv[1];
    FILE* fp = fopen( filename, "r+b" );
    if ( fp == NULL ){
        long root;
        root = (long) sizeof(long);
        fp = fopen( filename, "w+b" );
        fwrite( &root, sizeof( long ), 1, fp );
        writer(node, -1, fp, order);
    }
    while(1){
        char *input = NULL;
        size_t len = 0;
        ssize_t read;
        read = getline(&input, &len, stdin);
        if(read == -1) continue;
        char **inputs = partitionInput(input);
        if(strcmp(inputs[0], "find") == 0){
            long* path = malloc(sizeof(long));
            int nodesCount = 1;
            find(atoi(inputs[1]), order, fp, path, &nodesCount, 1);
            //free(path);            
        }else if(strcmp(inputs[0], "add") == 0){
            add(atoi(inputs[1]), order, fp);
        }else if(strcmp(inputs[0], "print\n") == 0){
            printer(fp, order);
        }else if(strcmp(inputs[0], "end\n") == 0){
            break;
        }else{
            printf("Wrong Argument!\n");
            exit(1);
        }
    }
    fclose(fp);
    return 0;
}

int reader(btree_node *node, long off, int order, FILE *in){
    fseek(in, off, SEEK_SET);
    fread(&(node->n), sizeof(int), 1, in);
    fread(node -> key, sizeof(int), (size_t) (order - 1), in);
    fread(node -> child, sizeof(long), (size_t) order, in);
    int i = 0;
    if(node->child[0] == 0){
        while(i < order){
            node->child[i++] = 0;
        }
    }
    return node -> n;
}

long search(FILE* in, long* path, int* status, int* nodesCount, int key, long off, int order){
    *path = off;
    btree_node *node = makeNewNode(order);
    int nodeLength = reader(node, off, order, in);
    int i;
    *status = -1;
    while(*status != 0 && *status != 1){
        off = inNodeSearch(node, key, nodeLength, off, status);
        reader(node, off, order, in);
        if(*status == 2 && off){
            long *tmp = realloc(path, ++(*nodesCount)*sizeof(int));
            tmp[*nodesCount-1] = off;
            path = tmp;
        }
    }
    free(node);
    return off;
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

long inNodeSearch(btree_node *node, int key, int nodeLength, long off, int *status) {
    int i = 0;
    while(i < node->n){
        if(node->key[i++] == key){
            *status = 1;
            return off;
        }
    }
    if(node->key[0] > key){
        if(node->child[0]){
            *status = 2;
            return node->child[0];
        }
        *status = 0;
        return off;         
    }
    if(node->key[node->n-1] < key){
        if(node->child[node->n]){
            *status = 2;
            return node->child[node->n];
        }
        *status = 0;
        return off;
    }
    for(i = 0; i < node->n-1; ++i){
        if(node->key[i] < key && node->key[i+1] > key){
            if(node->child[i+1]){
                *status = 2;
                return node->child[i+1];
            }
            *status = 0;
            return off;
        }
    }
    *status = 0;
    return off;
}

long find(int key, int order, FILE* in, long* path, int* nodesCount, int print){
    long root;
    fseek(in, 0, SEEK_SET);
    fread(&root, sizeof(long), 1, in);
    int* status = malloc(sizeof(int));    
    *status = -1;
    long resOff = search(in, path, status, nodesCount, key, root, order);
    if(*status){
        if(print == 2)
            printf("Entry with key=%d already exists\n", key);
        else
            printf("Entry with key=%d exists\n", key);
        return -1;
    }
    else if(*status == 0 && print == 1)
        printf("Entry with key=%d does not exist\n", key); 
    return resOff;
}

int splitNode(int* left, int* right, int mid, btree_node* target, int order, int nodeLength, int key){
    int *tmp = malloc(order * sizeof(int));
    int i;
    for(i = 0; i < nodeLength; ++i)
        tmp[i] = target->key[i];
    tmp[nodeLength] = key;
    qsort(tmp, (size_t) order, sizeof(int), compare);
    for(i = 0; i < mid; ++i)
        left[i] = tmp[i];
    for(i = 0; i < order - mid - 1; ++i)
        right[i] = tmp[mid+i+1];
    return  tmp[mid];
}

void add(int key, int order, FILE *in){
    int nodesCount = 1;
    long* path = malloc(sizeof(long));
    long targetOffset = find(key, order, in, path, &nodesCount, 2);
    if(targetOffset == -1) return;
    btree_node* node = makeNewNode(order);
    int nodeLength = reader(node, targetOffset, order, in);
    if(nodeLength == order - 1){
        int mid = split(order);
        int *left = malloc(mid*sizeof(int));
        int *right = malloc((order-mid-1)*sizeof(int));
        int promote = splitNode(left, right, mid, node, order, nodeLength, key);
        updateNode(node, left, order-1, mid);
        node->n = mid;
        writer(node, targetOffset, in, order);
        btree_node* new_node = makeNewNode(order);
        new_node->key = (int *)realloc(right, (order - 1)*sizeof(int));
        new_node->n = order-mid-1;
        fseek(in, 0, SEEK_END);
        long new_off = ftell(in);
        writer(new_node, -1, in, order);
        addHelper(in, new_off, order, promote, nodesCount, path);
    }else{
        node->key[nodeLength] = key;
        node->n += 1;
        qsort(node -> key, (size_t) node->n, sizeof(int), compare);
        writer(node, targetOffset, in, order);
    }
    free(path);
}

void updateNode(btree_node* node, int* newNode, int n, int mid){
    int i = 0;
    for(; i < n; ++i){
        if(i < mid)
            node->key[i] = newNode[i];
        else
            node->key[i] = 0;
    }
}


void writer(btree_node *node, long off, FILE *in, int order){
    fseek(in, off == -1 ? 0 : off, off == -1 ? SEEK_END : SEEK_SET);
    fwrite( &(node->n), sizeof( int ), 1, in );
    fwrite( node->key, sizeof( int ), (size_t) (order-1), in );
    fwrite( node->child, sizeof( long ), (size_t) order, in );    
}

btree_node* makeNewNode(int order){
    btree_node *node = (btree_node*)malloc(sizeof(btree_node));
    node->key = (int *) calloc((size_t) (order - 1), sizeof( int ) );
    node->child = (long *) calloc((size_t) order, sizeof( long ) );
    return node;
}

void addHelper(FILE* in, long new_off, int order, int key, int nodesCount, long* path){
    if(nodesCount == 1){
        btree_node *new_root = makeNewNode(order);
        new_root->n = 1;
        new_root->key[0] = key;
        new_root->child[0] = path[0];
        new_root->child[1] = new_off;
        fseek(in, 0, SEEK_END);
        long end = ftell(in);
        rewind(in);
        fwrite(&end, sizeof(long), 1, in);
        writer(new_root, -1, in, order);
        return;
    }
    btree_node* target = makeNewNode(order);
    int nodeLength = reader(target, *(path+nodesCount-2), order, in);
    int position = 0, i = 0;
    while(i < nodeLength){
        if(target->key[i++] < key)
            position++;
    }
    if(nodeLength == order - 1){
        int mid = split(order);
        int *left = malloc(mid*sizeof(int));
        int *right = malloc((order-mid-1)*sizeof(int));
        int promote = splitNode(left, right, mid, target, order, nodeLength, key);
        updateNode(target, left, order-1, mid);
        target->n = mid;
        long *newNodes = malloc((nodeLength+2) * sizeof(long));
        if(position == nodeLength){
            newNodes = target->child;
            newNodes[nodeLength+1] = new_off;
        }else
            copyNodes(nodeLength, position, newNodes, target, new_off);
        btree_node* new_node = (btree_node *) malloc(sizeof(btree_node));
        new_node->key = (int *)realloc(right, (order - 1)*sizeof(int));
        new_node->child = (long *) calloc((size_t) order, sizeof( long ) );
        new_node->n = order-mid-1;
        for(i = 0; i < order-mid; ++i)
            new_node->child[i] = newNodes[mid+i+1];
        for(i = 0; i < order; ++i){
            if(i <= mid)
                target->child[i] = newNodes[i];
            else
                target->child[i] = 0;
        }
        writer(target, path[nodesCount-2], in, order);
        fseek(in, 0, SEEK_END);
        long end = ftell(in);
        writer(new_node, -1, in, order);
        nodesCount--;
        addHelper(in, end, order, promote, nodesCount, path);
    }else{
        target->key[nodeLength] = key;
        target->n += 1;
        qsort(target -> key, (size_t) target -> n, sizeof(int), compare);
        if(position == nodeLength)
            target->child[position+1] = new_off;
        else{
            long *tmp = malloc((nodeLength+2) * sizeof(long));
            copyNodes(nodeLength, position, tmp, target, new_off);
            target->child = tmp;
        }
        writer(target, path[nodesCount-2], in, order);
        nodesCount--;
    }
}

void copyNodes(int nodeLength, int position, long* res, btree_node* node, long insert){
    int j = 0, found = 0, i = 0;
    for(; i <= nodeLength; ++j){
        if(i-1 == position && !found){
            res[j] = insert;
            found = 1;
        }else{
            res[j] = node->child[i];
            i++;
        }
    }    
}

void printer(FILE* in, int order){
    q_element *root = (q_element*)malloc(sizeof(q_element));
    long root_node;
    fseek(in, 0, SEEK_SET);
    fread(&root_node, sizeof(long), 1, in);
    root->offset = root_node;
    root->next = NULL;
    int q_size = 1, steps = 0;
    q_element* head = root;
    q_element* cursor = head;
    while(q_size > 0){
        int size = q_size;
        steps++;
        printf(" %d: ", steps);
        while(size--){
            q_element* curr = head;
            btree_node* curr_node = makeNewNode(order);
            reader(curr_node, curr->offset, order, in);
            int i = 0;
            for(; i <= curr_node->n; ++i){
                if(curr_node->child[i]){
                    q_element* new_element = (q_element*)malloc(sizeof(q_element));
                    new_element->offset = curr_node->child[i];
                    new_element->next = NULL;
                    cursor->next = new_element;
                    cursor = cursor->next;
                    q_size++;
                }
                if(i != curr_node->n){
                    if(curr_node->key[i]){
                        if(i != curr_node->n-1)
                            printf("%d,", curr_node->key[i]);
                        else
                            printf("%d ", curr_node->key[i]);
                    }
                }
            }
            head = head->next;
            free(curr);
            q_size--;
        }
        printf("\n");
    }
}

int compare(const void * a, const void * b){
    return ( *(int*)a - *(int*)b );
}
