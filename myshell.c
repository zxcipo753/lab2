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

void build_fork(command* cmd){
   pid_t childPID;
   childPID = fork();
   if(childPID >= 0){
       if(childPID == 0){
           execvp(cmd->arg[0], cmd->arg);
       }else{
           waitpid(childPID, 0, 0);
       }
   }else{
       puts("Fork failed");
   }
}

void executeSingleCommand(command* cmd){
   pid_t childPID = fork();
   if(childPID < 0){
       puts("Fork failed");
       return;
   }else{
       if(childPID != 0){
           waitpid(childPID, 0, 0);
           if(runInternalCmd(cmd) == 1){
               return;
           }
           return;
       }else{
           int fd = 0;
           int oflag;
           if(cmd->inputMod != 0){
               oflag = O_RDONLY;
               fd = open(cmd->file, oflag);
    
               if(fd == -1){
                   printf("Cannot open file\n");
                   return;
               }else{
                   dup2(fd, STDIN_FILENO);
                
               }
           }
           if(cmd->outputMod != 0){
               oflag |= O_CREAT|O_WRONLY;
               if(cmd->outputMod == 2){
                   oflag |= O_APPEND;
               }else{
                   oflag |= O_TRUNC;
               }
               
               if(cmd->inputMod != 0){
                   fd = open(cmd->one->file, oflag, S_IRWXU|S_IRWXG|S_IRWXO);
               }else{
                   fd = open(cmd->file, oflag, S_IRWXU|S_IRWXG|S_IRWXO);
               }
               if(fd == -1){
                   printf("Cannot open file\n");
                   return;
               }else{
                   dup2(fd, STDOUT_FILENO);
               }
           }
           exit(execvp(cmd->arg[0], cmd->arg));
       }

   }
   
}
void run_pipe(command* cmd){
   if(cmd->one == NULL){
       perror("only one command. Cannot pipe");
       return;
   }
   int pfds[2];
   if(pipe(pfds) == 0){
       if(fork() == 0){
           close(1);
           dup2(pfds[1], STDOUT_FILENO);
           close(pfds[0]);
         
           execvp(cmd->arg[0], cmd->arg);
           exit(EXIT_SUCCESS);
       }else{
           close(0);
           dup2(pfds[0], STDIN_FILENO);
           close(pfds[1]);
           execvp(cmd->one->arg[0], cmd->one->arg);
           return;
       }
   }
}
           
void run_shell(command* cmd){
    if(cmd->background || cmd->parallel){
        while(cmd){
            build_fork(cmd);
            cmd = cmd->one;
        }
    }
    if(cmd->pipe){
        run_pipe(cmd);
    }else{
        executeSingleCommand(cmd);
    }
}
