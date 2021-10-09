#include "myshell.h"
#include "utility.c"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[], char* envp[])
{
    char pwd[PATH_SIZE];
    getcwd(pwd, sizeof(pwd);
    setenv("shell", pwd, 1);
    size_t size;
    command cmd;
    initialize(&cmd);
    char* commondline = NULL;
  
    FILE* batch = stdin;
    if(argc == 2){
        batch = fopen(argv[1], "r");
        if( batch == NULL){
            perror("Open file failed");
            return;
        }
    }
           
    int counter = 1;
    while(counter == 1){
        if(batch == stdin){
            printf("myshell:%s> ", dirPath(pwd));
        }
        int len = getline(&line, &size, batch);
        if(len > 0){
            while (len > 0 && (line[len - 1] == '\n' || line[len-1] == '\r')) {
                line[len-1] = '\0';
                len -= 1;
            }
            parseLine(&cmd, line);//still need to be finished
            if(strcmp(cmd.arg[0], "quit") == 0 ){
                counter = 0;
                break;
            }
       
            run_shell(&cmd);
        }else{
            if(len < 0){
                printf("line < 0\n");
            }

            continue;
        }
    }

    return 0;
}
char* dirPath(){
    char* pwd = getcwd(NULL, 0);
    char* sptr;
    char* path = strtok_r(pwd, "/", &sptr);
    while(sptr != NULL){
        path = strtok_r(NULL, "/", &sptr);
    }
    return path;
}
