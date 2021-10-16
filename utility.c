#include "myshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void initial_stats(command* cmd){
    cmd->arg[0] = NULL;
    cmd->argc = 0;
}

void parsePipe(command* cmd, char* line){
    char* copy_line = strdup(line);//make a duplicated line
    cmd->pipe = 1;
    char* token = NULL;
    char* saveptr;
    token = strtok_r(copy_line, "|", &saveptr);

    if(token != NULL){
        cmd->pipe = 1;
        parseSpaces(cmd, token);
    }

    if(saveptr != NULL){
        cmd->one = malloc(sizeof(command));
        initialize(cmd->one);
        parseSpaces(cmd->one, saveptr);
    }
}


void parseSpaces(command* cmd, char* line){
    char* copy_line = strdup(line);
    char* token = NULL;
    char* saveptr;

    int index = 0;
    token = strtok_r(copy_line, " ", &saveptr);
    while(token != NULL){
        cmd->arg[index] = strdup(token);
        index+=1;
        token = strtok_r(NULL, " ", &saveptr);
    }
    cmd->argc = index;
    cmd->arg[index] = NULL;
    free(line_dup);
}

void parseBackground(command* cmd, char* line){
    cmd->background = 1;
    char* ptr = line;
    int index = 0;
    while( *(ptr+index) != '\0' ){
        if(*(ptr+index) == '&'){
            *(ptr+index) = '\0';
        }
        index+=1;
    }
    parseSpaces(cmd, line);
}

void input_redirection(command* cmd, char* line){

    char* copy_line = strdup(line);
    
    char* saveptr;
    char* token;
    token = strtok_r(copy_line, "<", &saveptr);
    if(token != NULL){
        parseSpaces(cmd, token);
    }
    if(saveptr != NULL){
        if(*saveptr == '<'){
            cmd->inputMod = 2;
            saveptr = saveptr+1;
        }
        while(*saveptr == ' '){
            saveptr+=1;
        }
        cmd->file = strdup(saveptr);
    }
}


void output_redirection(command* cmd, char* line){
    char* copy_line = strdup(line);
    char* saveptr;
    char* token;
    token = strtok_r(copy_line, ">", &saveptr);
    if(token != NULL){
        parseSpaces(cmd, token);
    }
    if(saveptr != NULL){
        if(*saveptr == '>'){
            cmd->outputMod = 2;
            saveptr = saveptr+1;
        }else{
            cmd->outputMod = 1;
        }
        while(*saveptr == ' '){
            saveptr+=1;
        }
        cmd->file = strdup(saveptr);
    }
    free(line_dup);
}
void input_output_redirection(command* cmd, char* line){
    cmd->one = malloc(sizeof(command));
    initialize(cmd->one);
    char* copy_line = strdup(line);
    char* saveptr;
    char* token = strtok_r(copy_line, "<", &saveptr);
    if(token != NULL){
        parseSpaces(cmd, token);
    }
    if(saveptr != NULL){
        if(*(saveptr) == '<'){
            cmd->inputMod = 2;
            saveptr+=1;
        }else{
            cmd->inputMod = 1;
        }
    }
    char* token1 = strtok_r(NULL, ">", &saveptr);
    if(token1 != NULL){
        cmd->file = strdup(token1);
    }
    
    if(saveptr != NULL){
        if(*(saveptr) == '>'){
            saveptr+=1;
            cmd->outputMod = 2;
        }else{
            cmd->outputMod = 1;
        }
        cmd->one->file = strdup(saveptr);
    }
    
    
}
void cd_(command* cmd){
    static char pwd[PATH_SIZE]; //local
    getcwd(pwd, sizeof(pwd));
    
    if(cmd->argc > 2){ //error auguments amount
        puts("Enter only one argument.");
        return;
    }
          
    if(cmd->argc == 1){
        puts(PWD);
        return;
    }
    
    if (chdir(cmd->arg[1]) == 0){
        getcwd(pwd, sizeof(PWD));
        setenv("PWD", pwd, 1);
    }else{
        puts("chdir fails.");
    }
    
}

void clr_(){
    printf("\033[H\033[2J"); // clean the screen
}

void dir_ (command* cmd){
    
    if(cmd->argc > 2){ //error arguments amount
        puts("Enter only one argument.");
        return;
    }
    
    char pwd[PATH_SIZE]; //local
    if(getcwd(pwd, sizeof(pwd)) == NULL){
        perror("cannot get current dir");
        return;
    }
    
    char file_path[PATH_SIZE] = "";

    if(cmd->argc == 1){
        strcat(file_path, pwd);
    }else{
        strcat(file_path, cmd->arg[1]);
    }
    
    DIR* dir = opendir(filePath);

    if(dir == NULL){
        perror("open file failed");
        return;
    }
    
    struct dirent* dirList = NULL;
    while((dirList = readdir(dir) )!= NULL){
        printf("%s\n", dirList->d_name);
    }
    closedir(dir);
    
}

void environ_(){
    int i =0;
    while(environ[i] != NULL){
        printf("%s\n", environ[i]);
        i += 1;
    }
}

void echo_(command* cmd){
    
    if(cmd->argc == 1){
        puts("");
    }
    else{
        for(int i = 1; i < cmd->argc && cmd->arg[i] != NULL; i++){
            printf("%s ", cmd->arg[i]);
        }
        printf("\n");
    }
}
void help_(){
    char pwd[PATH_SIZE];
    getcwd(pwd, sizeof(pwd);
    
    strcat(pwd, "/readme");
    FILE* fptr = fopen(pwd, "r");

    char content;
    content = fgetc(fptr);
    while(content != EOF){
        printf("%c", content);
        content = fgetc(fptr);
    }
           
    fclose(fptr);
    puts("");
}

void pause_(){
    while(getchar() != '\n'){
        ;
    }
}

void quit_(){
    exit(EXIT_SUCCESS);
}
