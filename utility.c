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
#define SIZE 1024

void myCd(char **cmd);
void myClr();
void myDir(char **cmd);
void myEcho(char **cmd);
void myEnviron();
void myHelp();
void myPause();
void myQuit();
FILE *inRedirection(char **cmd) ;
FILE *outRedirection(char **cmd) ;
void shSetEnv(char *argument);
void shShowEnviron(char **argument);


int main(int argc, char **argv) {
    
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


int bg(char **argument) {
    int i = 0;
    while(argument[i]) {
        i++;
    }
    if(strcmp(argument[i-1], "&") == 0) {
        return 1;
    } else {
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

/* Change Directory function that we went over in class and slides */
void myCd(char **cmd){
    if(!cmd[1] || strcmp(cmd[1], " ") == 0) {
        chdir(cmd[1]);
    } 
    else {
        chdir(getenv("HOME")); 
    }
}

/* Simple clear argument, cant use system("clear") so it was suggested to me to use the
    ANSI encoder to clear the screen */
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