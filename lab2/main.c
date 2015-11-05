#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

//data structure to send the two parameters to thread
typedef struct cell{
    int row, column;
} cell;

//variables to hold matrices dimensions
int Arows,Acolumns,Brows,Bcolumns,Crows,Ccolumns;
//2D double arrays to hold the matrices
double **A,**B,**C;
//variables to hold files paths
char * pathA,* pathB,* pathC;

//calc C matrix dimensions & initialise the output Matrix C
void initCarray(){
    int i;
    Crows = Arows;
    Ccolumns = Bcolumns;
    //allocate memory for C matrix
    C = malloc(Crows*sizeof(double*));
    for(i=0;i<Crows;i++) C[i] = malloc(Ccolumns*sizeof(double));
}

//checks the dimensions of the matrices A and B
//to make sure they can be mutiplied
void checkDimensions(){
    if(Acolumns != Brows){
        printf("given matrices can't be multiplied (Acolumns != Brows)\n");
        exit(-1);
    }
}

//make sure that A,B matrix file exist
int validFiles(){
    if(access(pathA,F_OK) || access(pathB,F_OK))
        return 0;
    return 1;
}

//reading matrices A,B
void readFiles(){
    FILE *a = fopen(pathA,"r");
    FILE *b = fopen(pathB,"r");
    char *line = NULL;
    size_t len = 0;
    int i,j;
    getline(&line,&len,a);
    if(len>0){
        sscanf(line,"row=%d col=%d", &Arows, &Acolumns);
        //error in matrix A dimension
        if(Arows == 0 || Acolumns==0){
            printf("error in matrix A Dimensions (0 Dimension or Dimension not supplied)\n");
            exit(-1);
        }
        //allocate memory for matrix A
        A = malloc(Arows*sizeof(double*));
        for(i=0;i<Arows;i++) A[i] = malloc(Acolumns*sizeof(double));
        //scanning matrix A from the file and showing any error required
        for(i=0;i<Arows;i++){
            getline(&line,&len,a);
            char * token;
            token = strtok (line,"\t");
            for(j=0;j<Acolumns;j++){
                if(token == NULL){
                    printf("error in matrix A element:(%d %d)\n",i,j);
                    exit(-1);
                }
                sscanf(token,"%lf",&A[i][j]);
                token = strtok(NULL, "\t");
            }
        }
    }
    else{
        printf("error in matrix A file (empty file)\n");
        exit(-1);
    }
    close(a);
    getline(&line,&len,b);
    if(len>0){
        sscanf(line,"row=%d col=%d", &Brows, &Bcolumns);
        //error in matrix B dimension
        if(Brows == 0 || Bcolumns==0){
            printf("error in matrix B Dimensions (0 Dimension or Dimension not supplied)\n");
            exit(-1);
        }
        //allocate memory for matrix B
        B = malloc(Brows*sizeof(double*));
        for(i=0;i<Brows;i++) B[i] = malloc(Bcolumns*sizeof(double));
        //scanning matrix B from the file and showing any error required
        for(i=0;i<Brows;i++){
            getline(&line,&len,b);
            char * token;
            token = strtok (line,"\t");
            for(j=0;j<Bcolumns;j++){
                if(token == NULL){
                    printf("error in matrix B element:(%d %d)\n",i,j);
                    exit(-1);
                }
                sscanf(token,"%lf",&B[i][j]);
                token = strtok(NULL, "\t");
            }
        }
    }
    else{
        printf("error in matrix A file (empty file)\n");
        exit(-1);
    }
    close(b);
}

//thread function to calculate row value in C matrix
void *rowCalc(void *row){
    int rowID = (int)row;
    int i,j;
    for(j=0;j<Bcolumns;j++){
    double res=0;
        for(i=0;i<Acolumns;i++){
            res += A[rowID][i]*B[i][j];
        }
    C[rowID][j] = res;
    }
    pthread_exit(NULL);
}

//thread function to calculate cell value in C matrix
void *cellCalc(void *req){
    cell *requested = req;
    int rowID = requested->row;
    int ColumnID = requested->column;
    int i;
    double res=0;
    for(i=0;i<Acolumns;i++){
        res += A[rowID][i]*B[i][ColumnID];
    }
    C[rowID][ColumnID]=res;
    pthread_exit(NULL);
}

//running a thread for each row
void method1(){
    int i;
    pthread_t tids[Crows];
    struct timeval stop, start;
    int n=0;
    //start checking time
    gettimeofday(&start, NULL);
    //create the threads and give them
    //the row they will work on
    for(i=0;i<Crows;i++){
            pthread_create(&tids[n++],NULL,rowCalc, (void *)i);
    }
    //wait for threads to finish
    for(i=0;i<n;i++){
        pthread_join(tids[i],NULL);
    }
    //end checking time
    gettimeofday(&stop, NULL);
    printf("          Method 1\nMicroseconds taken: %lu\nThreads ran:%d\n", stop.tv_usec - start.tv_usec, Crows);
}

//running a thread for each cell
void method2(){
    int i,j;
    pthread_t tids[Crows*Ccolumns];
    struct timeval stop, start;
    //start checking time
    gettimeofday(&start, NULL);
    int n=0;
    //create the threads and give them
    //the row and column they will work on
    for(i=0;i<Crows;i++){
        for(j=0;j<Ccolumns;j++){
            cell *data = (cell *) malloc(sizeof(cell));
            data->row = i;
            data->column = j;
            //printf("%d %d\n",data->row,data->column);
            pthread_create(&tids[n++],NULL,cellCalc, data);
            //pthread_join(tids[n++],NULL);
        }
    }
    //wait for threads to finish
    for(i=0;i<n;i++){
        pthread_join(tids[i],NULL);
    }
    //end checking time
    gettimeofday(&stop, NULL);
    printf("          Method 2\nMicroseconds taken: %lu\nThreads ran:%d\n", stop.tv_usec - start.tv_usec, Crows*Ccolumns);
}

//writes C matrix to file
void writeCtoFile(int method){
    int i,j;
    char * auxPathC;
    //file name = [name].[extension]
    //replacing it with [name]_[method].[extension]
    if(strstr(pathC,".")>0){
        char * after = strdup(pathC);
        char * before = strtok(after, ".");
        after = strdup(strtok(NULL, "\n"));
        char sub[4] = "_1.\0";
        sub[1] = method+'0';
        strcat(before, sub);
        strcat(before, after);
        auxPathC = strdup(before);
    }
    //file name = [name]
    //replacing it with [name]_[method]
    else{
        char sub[3] = "_1\0";
        sub[1] = method+'0';
        auxPathC = strdup(pathC);
        strcat(auxPathC,sub);
    }
    //open C file
    FILE *c = fopen(auxPathC,"w");
    //write the matrix cells
    for(i=0;i<Crows;i++){
        for(j=0;j<Ccolumns;j++){
            fprintf(c,"%2.2lf%s",C[i][j],j==Ccolumns-1?"\n":"\t");
        }
    }
    close(c);
}

int main (int argc, char *argv[]){
    //default case not providing
    //paths to matrix files
    //using a.txt, b.txt, c.out
    if(argc ==1){
        pathA = "a.txt";
        pathB = "b.txt";
        pathC = "c.out";
    }
    //provided files paths
    else if(argc == 4){
        pathA = argv[1];
        pathB = argv[2];
        pathC = argv[3];
    }
    else{
        printf("Illegal Parameters\n");
        exit(-1);
    }
    if(!validFiles()){
        printf("Files Error\n");
        exit(-1);
    }
    readFiles();
    checkDimensions();
    initCarray();
    method1();
    writeCtoFile(1);
    initCarray();
    method2();
    writeCtoFile(2);
    exit(0);
}
