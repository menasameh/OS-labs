#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

//data structure to hold variables name/value in an assignment command
typedef struct variable{
    char * name;
    char * value;
} var;

//list of variables that user has declared
var * vars;
//variable to hold history file path
char * history ;
//variable to hold log file path
char * log ;
//array of variables holding the values of the environmental variable PATH
char * pathValues[100];
//number of elements in pathValues
int pathLength=0;
int varCurrent = 0;
int varMax = 10;
int pid;

//function to handdle signals from terminating children
void sig_handler(int signo){

  if (signo == SIGCHLD){//a child process has been terminated
    char line[25];
    sprintf(line,"%d : process terminated",pid);
    writeToFile(log,line);
    fflush(stdout);
    fflush(stderr);
  }
  if(signo == SIGINT){//CTRL-C has been triggered
    char line[25];
    sprintf(line,"%d : process terminated",getpid());
    writeToFile(log,line);
    exit(2);
  }
}
//function to add a variable and it's value to the list vars
void addValue(char * name, char * value){
if(varCurrent+1 == varMax){
    varMax*=2;
    var * varsTemp = malloc(varMax*2);
    int i;
    memcpy(varsTemp, vars,varCurrent*2);
    vars = varsTemp;
}
var v ;
v.name = name;
v.value = value;
vars[varCurrent] = v;
varCurrent++;
}
//function to retreive value of variable
//or "" if the variable is not stored
char * getValue(char * str){
    int i=0;
    while(i < varCurrent){
        if(strcmp(str, vars[i].name)==0){
            return vars[i].value;
        }
        i++;
    }
    char *res = getenv(str);
    return res == NULL?"":res;
}
//function to print help message to the user
void printHelp(){
system("clear");
fprintf(stderr, "Usage: Shell [batchFile]\nbatchFile                  absolute path to a batch file to be executed\nspecial commands other than the default shell ones\n    help                  show this help message\n    history               show commands history\n    log                   show log file content\n    exit                  exit the shell\n");
}

//function to initialize history, log and pathValues
void init(char * path){
    int lastSlash=0;
    int i;
    //get last / in the path
    for(i=0;i<strlen(path);i++){
        if(path[i] == '/'){
            lastSlash = i;
        }
    }
    //end the char * at the index of the last slash
    path[lastSlash+1] = '\0';

    history = malloc(8+strlen(path));
    history = strdup(path);
    strcat(history, "history");

    log = malloc(4+strlen(path));
    log = strdup(path);
    strcat(log, "log");

    char *line = getenv("PATH");
    int len = strlen(line);
    char *ch;
    ch = strtok(line, ":");
    pathLength=0;
    while (ch != NULL) {
        pathValues[pathLength] = malloc(strlen(ch) + 1);
        strcpy(pathValues[pathLength], ch);
        pathLength++;
        ch = strtok(NULL, ":");
    }
}
//function to write a string to a file
void writeToFile(char * filePath, char * text){
FILE *file;
file=fopen(filePath, "a");
if(file==NULL) {
    fprintf(stderr, "%s:cant open file\n",filePath);
    return ;
}
fprintf(file, "%s\n", text);
fclose(file);
}

//function toprint the content of a file to the screen
void printFileContent(char * filePath){
    FILE *file;
    char buffer[256];
    file=fopen(filePath, "r");
    if(file==NULL) {
        fprintf(stderr, "%s:cant open file\n",filePath);
        return ;
    }
    while(fgets(buffer, sizeof(buffer), file)) {
        printf("%s",buffer);
    }
    fclose(file);
}
//function to remove whitespace characters from begining and ending of string str
void trim(char *str){
    int i;
    int begin = 0;
    int end = strlen(str) - 1;
    //finding the first character that is not a whitespace
    while (isspace(str[begin]))
        begin++;

    //finding the last character that is not a whitespace
    while ((end >= begin) && isspace(str[end]))
        end--;
    //shifting characters
    for (i = begin; i <= end; i++)
        str[i - begin] = str[i];
    //terminating string with a '\0'
    str[i - begin] = '\0';
}


//a function to check if a char is can be part of a name
int isNameEnd(char x){
    if(x=='\n' || x=='\"' || x=='\'' || x==' ' || x=='\t' )
        return 1;
    return 0;
}

//a function to substitute characters : $, ~ with their values
char * substitute(char *str){
    int i;
    int begin = 0;
    int end = strlen(str);
    char * result;
    int newlen = 0;

    //calculating the new length of th command
    for(i=0;i<end;i++){
        if(str[i] == '$'){
            int index=i+1;
            int varLen =0;
            while (!isNameEnd(str[index]) && index < end){
                varLen++;
                index++;
            }
            index = i+1;
            char * name = malloc(varLen+1);
            while (!isNameEnd(str[index]) && index < end){
                name[index-i-1]=str[index];
                index++;
            }
            name[index]='\0';
            newlen+=strlen(getValue(name));
            i+=strlen(name);
        }
        else if(str[i] == '~'){
            newlen+=strlen(getenv("HOME"));
        }
        else{
        newlen++;
        }
    }

    result = malloc(newlen+1);
    int nameIndex = 0;
    int valIndex = 0;

    while(nameIndex < end){
        if(str[nameIndex] == '$'){
            int index=nameIndex+1;
            int varLen =0;
            while (!isNameEnd(str[index]) && index < end){
                varLen++;
                index++;
            }
            index = nameIndex+1;
            int idx=0;
            char * name = malloc(varLen+1);
            while (!isNameEnd(str[index]) && index < end){
                name[idx++]=str[index];
                index++;
            }
            name[idx]='\0';
            nameIndex+=strlen(name);
            valIndex+=strlen(getValue(name));
            strcat(result, getValue(name));
        }
        else if(str[nameIndex] == '~'){
            valIndex+=strlen(getenv("HOME"));
            strcat(result, getenv("HOME"));
        }
        else{
            result[valIndex++] = str[nameIndex];
        }
        nameIndex++;
    }
    return result;
}
//function to try get the actual path of name
void getFilePath(char ** name, char * path){
    int i;
    if( access( *name, R_OK|X_OK ) != -1 ) {
            return ;
        }
    int len = strlen(*name)+2;
    int startWithSlash = *name[0] == '/';
    path = malloc(40+len+1);
    for(i=0;i<pathLength;i++){
        int newlen = strlen(pathValues[i])+len;
        path = strdup(pathValues[i]);
        if(!startWithSlash){
            strcat(path, "/");
        }
        strcat(path, *name);
        if( access( path, R_OK|X_OK ) != -1 ) {
            *name = strdup(path);
            return ;
        }
    }
}

//function to split the command parts and store them in strArray
int split(char **strArray, char* l){
    int i;
    char *line = strdup(l);
    int len = strlen(line);
    for(i=0;i<len;i++){
        if(line[i]=='"'){
            while(line[++i]!='"' && i < len){
                if(line[i]==' '){
                    line[i] = 5;
                }
                else if(line[i]=='\t'){
                    line[i] = 6;
                }
            }
            break;
        }
        else if(line[i]=='\''){
            while(line[++i]!='\'' && i < len){
                if(line[i]==' '){
                    line[i] = 5;
                }
                else if(line[i]=='\t'){
                    line[i] = 6;
                }
            }
            break;
        }
    }
    char *ch;
    ch = strtok(line, " \t");
    i=0;
    while (ch != NULL) {
        strArray[i] = malloc(strlen(ch) + 1);
        strcpy(strArray[i], ch);
        int j=0;
        for(;j<strlen(strArray[i]);j++){
            if(strArray[i][j]==5){
                strArray[i][j] = ' ';
            }
            else if(strArray[i][j]==6){
                strArray[i][j] = '\t';
            }
        }
        i++;
        ch = strtok(NULL, " \t");
    }
    return i;
}
//function to send command and parameters for execution
void execute(char *command){
    int background = 0;
    trim(command);
    printf("%s\n", command);
    if(command[0]=='#'){
        return;
    }
    int i;
    int assignmentStatement=0;
    for(i=0;i<strlen(command);i++){
        if(command[i]=='='){
            assignmentStatement = 1;
            break;
        }
        else if(command[i]==' ' || command[i]=='\t' || command[i]=='\"' || command[i]=='\''){
            assignmentStatement = 0;
            break;
        }
    }
    if(strstr(command,"=")>0 && assignmentStatement){
        char * after = strdup(command);
        char * before = strtok(after, "=");
        after = strtok(NULL, "\n");
        addValue(before, after);
        return ;
    }
    command = substitute(command);
    if(strcmp(command,"") ==0){
        fprintf(stderr, "empty command\n");
        return;
    }
    char *strArray[500];
    int parts = split(strArray,command);
    char * path;
    getFilePath(&strArray[0],path);
    i=0;
    if(strcmp(strArray[parts-1],"&") ==0){
        if(parts == 1){
            fprintf(stderr, "& : invalid operation\n");
            return ;
        }
        strArray[parts-1]=NULL;
        background = 1;
    }
    else{
        strArray[parts++]=NULL;
    }
    if(strArray[0] == NULL){
        return ;
    }
    if(strcmp(strArray[0], "cd")==0){
        int status = chdir(strArray[1]);
        if(status){
            status = chdir(strcat(getenv("PWD"),strArray[1]));
            if(status){
                fprintf(stderr, "%s : invalid path\n",strArray[0]);
            }
        }
        return ;
    }
    if(strcmp(strArray[0], "exit") == 0){
        char line[25];
        sprintf(line,"%d : process terminated",getpid());
        writeToFile(log,line);
        exit(0);
    }
    if(strcmp(strArray[0], "history") == 0){
        printFileContent(history);
        return;
    }
    if(strcmp(strArray[0], "help") == 0){
        printHelp();
        return;
    }
    if(strcmp(strArray[0], "log") == 0){
        printFileContent(log);
        return;
    }
            fflush(stdout);
        fflush(stderr);
    pid=fork();
    if(pid == 0){
        char line[25];
        sprintf(line,"%d : process created",getpid());
        writeToFile(log,line);

        execv(strArray[0],strArray);
        fprintf(stderr, "%s : invalid operation\n",strArray[0]);
    }
    if(pid < 0){
        fprintf(stderr,"problem creating process \n");
    }
    if(pid > 0){
        if(!background)
            wait(pid);
        else
            return;
    }
}

int main(int argc, char* argv[]){
    init(argv[0]);
    char line[25];
    sprintf(line,"%d : process created",getpid());
    writeToFile(log,line);
    vars = malloc(2*10);
    char command[520];
    signal(SIGCHLD, sig_handler);
    signal(SIGINT, sig_handler);
    if(argc > 2){
        fprintf(stderr, "too many arguments\n");
        printHelp();
    }
    //reading from Batch File
    if(argc == 2){
        FILE *file;
        file=fopen(argv[1], "r");
        if(file==NULL) {
            fprintf(stderr, "%s:cant open file\n",argv[1]);
            return ;
        }
        while(fgets(command, 520, file)) {
            if(strlen(command) > 512){
                fprintf(stderr, "operation is too long \(>512 char\)\n");
            }
            else if(command[0] == '\0'){
                exit(0);
            }
            else{
                execute(command);
                writeToFile(history,command);
            }
        }
        exit(0);
    }

while(1){
//reading from user input
    fflush(stderr);
    fflush(stdout);
    printf("shell >");
    fgets(command, 520, stdin);
    if(strlen(command) > 512){
        fprintf(stderr, "operation is too long \(>512 char\)\n");
    }
    else{
        execute(command);
        writeToFile(history,command);
        fflush(stdout);
        fflush(stderr);
        }
}
    return 0;
}
