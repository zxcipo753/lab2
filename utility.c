#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/wait.h>
#define SET_SIZE 900

void myCd(char **cmd);
void myClr();
void myDir(char **cmd);
void myEcho(char **cmd);
void myEnviron();
void myHelp();
void myPause();
void myQuit();

void myBatch(char *batchFile);
void myPrompt();
int myBackground(char **cmd);

FILE *inRedirection(char **cmd) ;
FILE *outRedirection(char **cmd) ;
void resetIO(FILE *in_container, FILE *out_container, int inNum, int outNum, int errNum);


int main(int argc, char **argv) {
    char pwd[SET_SIZE];
    if(!getcwd(pwd, sizeof(pwd))) {
        printf("Cannot get current directory.");
        return EXIT_FAILURE;
    }
    strcat(pwd, "/myshell"); 
    setenv("shell", pwd, 1);
    
}

void myBatch(char *batchFile) {
    char content[SET_SIZE] = "";
    char **argument = NULL;

    int inNum  = dup(STDIN_FILENO);
    int outNum = dup(STDOUT_FILENO);
    int errNum = dup(STDERR_FILENO);
 
    FILE *readBatch = fopen(batchFile, "r");
    FILE *in_container = NULL;
    FILE *out_containter = NULL;

    if(readBatch == NULL) {
        printf("No content in batchfile to read.");
    } else {
        fgets(content, SET_SIZE, readBatch);
        while(!feof(readBatch)) {
            int i = 0;
            //raplace newline with Null
            while(content[i] != '\0') {
                if(content[i] == '\n') {
                    content[i] = '\0';
                }
                i++;
            }
            
            //call pares_line() to split content to an argument
            argument = parse_line(content);

            in_container = inRedirection(argument);
            out_containter = outRedirection(argument);

            if(in_container) {
                dup2(fileno(in_container), STDIN_FILENO);
            }
            if(out_containter) {
                dup2(fileno(out_containter), STDOUT_FILENO);
                dup2(fileno(out_containter), STDERR_FILENO);
            }

            myExecute(argument);

            resetIO(in_container, out_containter, inNum, outNum, errNum);
            free(argument);
            strcpy(content, "");
            fgets(content, sizeof(content), readBatch);
        }
        strcpy(content, "");
        fclose(readBatch);
    }
    exit(EXIT_SUCCESS);
}

/* Make sure the command line is not empty, then split input into each token to get arguments*/
char **parse_line(char *input) {
    char **cmd = malloc(BUFF_SIZE *sizeof(char *));
    char *token;
    char error_occur[50] = "The line is empty.";

    //make sure command line is not empty
    if(cmd == NULL) {
        write(STDERR_FILENO, error_occur, strlen(error_occur));
        exit(EXIT_FAILURE);
    }

    //get first token
    token = strtok(input, " \t\r\n\a");
    int i = 0;
    //continuing to get rest tokens
    while(token != NULL) {
        cmd[i] = token;
        token = strtok(NULL, " \t\r\n\a");
        i++;
    }
    cmd[i] = NULL;
    return cmd;
}


int myBackground(char **cmd) {
    int index = 0;
    while(cmd[index]) {
        index++;
    }

    if(strcmp(cmd[index-1], "&") == 0) {
        return 1;
    } 
    else {
        return 0;
    }
}

void myPrompt() {
    char pwd[1024];
    if(getcwd(pwd, sizeof(pwd))) {
        printf("%s/myshell>", pwd);
    } else {
        printf("Cannot get current directory.")
    }
}

void *builtIn(char **argument))(char **argument {
    void (*func)(char **) = NULL; 
    if(strcmp(argument[0], "clr") == 0 || strcmp(argument[0], "clear") == 0) {
        return &myClr;
        //return 1; can't use this since return makes pointer from integer without a cast
    } else if(strcmp(argument[0], "dir") == 0) {
        return &myDir;
    } else if(strcmp(argument[0], "environ") == 0) {
        return &myEnviron;
    } else if(strcmp(argument[0], "echo") == 0) {
        return &myEcho;
    } else if(strcmp(argument[0], "help") == 0) {
        return &myHelp;
    } else if(strcmp(argument[0], "pause") == 0) {
        return &myPause;
    }
    return NULL;
}

// change current directory
void myCd(char **cmd){
    if(!cmd[1] || strcmp(cmd[1], " ") == 0) {
        chdir(cmd[1]);
    } 
    else {
        chdir(getenv("HOME")); 
    }
}

/* clear the screen */
void myClr() {
    printf("\e[1;1H\e[2J");
}

/*
list content in this directory
*/
void myDir(char **cmd) {
    char *destination = cmd[1];
    DIR *d;
    struct dirent *dir;

    //perror without current dir
    if((d = opendir(destination)) == NULL) {
        printf("Cannot get current dir.");
        return;
    }
    if(d != NULL) {
        while((dir = readdir(d))) {
            if(strcmp(".", dir->d_name) == 0 || strcmp("..", dir->d_name) == 0)
                continue;
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
}

/* Echo function that prints everything after args[0] to the screen */
void myEnviron() {
    int i = 0;
    while(environ[i]) {
        printf("%s\n", environ[i]);
        i++;
    }
}

void myEcho(char **cmd) {
    if (cmd[1] == NULL)
        fprintf(stdout, "\n");
    else {
        for (int i = 1; cmd[i] != NULL; i++)
            printf("%s ", cmd[i]);
        printf("\n");
    }
}

void myHelp(char **argument) {
    char current[SIZE] = "";
    pid_t pid;
    strcat(current, getenv("PWD"));
    strcat(current, "/readme");
    if((pid = fork()) == 0) {
        execlp("more", "more", current, NULL);
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

//Till user hits Enter
void myPause() {
    while(getchar() != '\n') {
        ;
    }
}

void myQuit() {
    exit(EXIT_SUCCESS);
}



void resetIO(FILE *in_container, FILE *out_container, int inNum, int outNum, int errNum) {
    dup2(inNum, STDIN_FILENO);
    if(in_container)
        fclose(in_container);
    if(out_container)
        fclose(out_container);
    dup2(outNum, STDOUT_FILENO);
    dup2(errNum, STDERR_FILENO);
}

//Below are I/O redirection functions
FILE *inRedirection(char **cmd) {
    FILE *file1 = NULL;
    int index = 1;
    int counter = 0;

    // use a loop to travese all input
    while(cmd[index]) {
        //find the key symbol '<'
        if(cmd[index][0] == '<') {
            counter = index;
        // move forward the pointer to get the source file to input
        if(cmd[index++]) {
            file1 = fopen(cmd[index], "r");
            open(cmd[index], O_RDONLY);//open file with READ only mode
            if(file1 == NULL) {
                printf("File does not exist.");
            }
            while(cmd[counter+1]) {
                cmd[counter] = cmd[counter+1];
                counter++;
            } 
            return file1;
            }
        }
        index++;
    }
    return file1;
}

FILE *outRedirection(char **cmd) {
    FILE *file1 = NULL;
    int index = 1;
    int counter;
    char file_mode[] = ""; // determine whether the file open as read or write mode

    // use a loop to travese all input
    while(cmd[index]) {
        //find the key symbol '>'
        if(strcmp(cmd[index], ">") == 0) {
            counter = index;
            file_mode[0] = 'w';
        } else if(strcmp(cmd[index], ">>") == 0) {
            counter = index;
            file_mode[0] = 'a';
        }
             
        if(cmd[index++]) {
            file1 = fopen(cmd[index], file_mode);
            if(file1 == NULL) {
                printf("File does not exist.");
            }
            cmd[counter] = NULL;
            return file1;
            }
        
            index++;
    }
    return file1;
}