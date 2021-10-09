#include "myshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void initial_stats(command* cmd){
    cmd->arg[0] = NULL;
    cmd->argc = 0;
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
