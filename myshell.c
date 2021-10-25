#include "myshell.h"
#define SET_SIZE 900
#define BUFF_SIZE 500 

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


int main(int argc, char **argv) {
    char pwd[SET_SIZE];
    if(!getcwd(pwd, sizeof(pwd))) {
        printf("Cannot get current directory.");
        return EXIT_FAILURE;
    }
    strcat(pwd, "/myshell"); 
    setenv("shell", pwd, 1);

    char *batchfile = NULL;
    if(argv[1]) {
        batchfile = argv[1];
        myBatch(argv[1]);
    }
    mainLoop();
    return EXIT_SUCCESS;
}

void myBatch(char *batchFile) {
    char content[SET_SIZE] = "";
    char **cmd = NULL;

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
            cmd = parse_line(content);

            in_container = inRedirection(cmd);
            out_containter = outRedirection(cmd);

            if(in_container) {
                dup2(fileno(in_container), STDIN_FILENO);
            }
            if(out_containter) {
                dup2(fileno(out_containter), STDOUT_FILENO);
                dup2(fileno(out_containter), STDERR_FILENO);
            }
            //execute input argument
            myExecute(cmd);

            resetIO(in_container, out_containter, inNum, outNum, errNum);
            free(cmd);
            strcpy(content, "");
            fgets(content, sizeof(content), readBatch);
        }
        strcpy(content, "");
        fclose(readBatch);
    }
    exit(EXIT_SUCCESS);
}


void mainLoop() {
    char *input;
    char **argument;

    int inNum  = dup(STDIN_FILENO);
    int outNum = dup(STDOUT_FILENO);
    int errNum = dup(STDERR_FILENO);

    // set file to empty before shell starts
    FILE *in = NULL;
    FILE *out = NULL;

    int start_loop = 1;
    while(start_loop) {
        myPrompt();
        input = readInput();
        /* parse the argument to split the different arguments */
        argument = parse_line(input);
     
        in = inRedirection(argument);
        out = outRedirection(argument);

        if(in)
            dup2(fileno(in), STDIN_FILENO);
        if(out) {
            dup2(fileno(out), STDOUT_FILENO);
            dup2(fileno(out), STDERR_FILENO);
        }
        
        //main core function to execute command
        myExecute(argument);

        resetIO(in, out, inNum, outNum, errNum);
        free(argument);
        free(input);
    }
}


/*---------Below are functions to parse user's input---------*/
char *readInput() {
    char *input = NULL;
    size_t buff_size = 0;
    getline(&input, &buff_size, stdin);
    return input;
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


/*--------Below are external functions-----------*/
int myExecute(char **cmd) {
    //Check if user want to quit; if not, move forward.
    if(strcmp(cmd[0], "quit") == 0) {
        myQuit(cmd);
    } else if(strcmp(cmd[0], "cd") == 0) {
        myCd(cmd);
    } else {
        pid_t pid;
       
        //save builtIn function address, see builtIn() function below
        void (*func)(char **) = NULL;
        func = builtIn(cmd);

        //parse command to find if there is a command argument. 
        int found;
        found = parse_pipe(cmd);
        if(found != 0) {
            myPipe(func, cmd, found);
        } else if((pid = fork()) == 0) {

            //pipe builtIn function; if not, move forward to external function
            if(func) {
                (*func)(cmd);
                exit(EXIT_SUCCESS);
            } else if(execvp(cmd[0], cmd) == -1) {
                printf("Command has an error.");
                exit(EXIT_FAILURE);
            }
        } else if(pid < 0) {
            printf("An error occurs.");
        } else {
            if(!myBackground(cmd)){
                waitpid(pid, NULL, 0);
            }
        }
        return 0;
    }
    return 0;
}


/* External function about pipe */
// Parse command line to find out if these is a '|'(pipe) in cmd.
int parse_pipe(char **cmd) {
    int index = 0;
    while(cmd[index]) {
        //Find out if cmd has pipe '|'
        if(strcmp(cmd[index], "|") == 0) {
            cmd[index] = NULL; // set to NULL to avoid a segment fault
            index++;
            return index;
        }
        index++;
    }
    return 0;
}

//Use cmd1 and cmd2 to represent input argument 1 and 2. Call buildIn(cmd) to get fun1 and fun2 in the pipe.
void myPipe(void func1(char **), char **cmd1, int found) {
    int filedes[2];
    int pid1; // use 'int' instead of 'pid_' to increase compability in different platform
    int pid2;
    char **cmd2 = cmd1 + found;
    pipe(filedes);

    void (*func2)(char **) = NULL;
    func2 = builtIn(cmd2);//Save built-in functions address

    //Forking to create a child, and then do stdin or stdout, then close
    pid1 = fork();
    if(pid1 == 0) {
        dup2(filedes[READ], STDIN_FILENO);
        pid2 = fork();
        if(pid2 == 0) {
            dup2(filedes[WRITE], STDOUT_FILENO);
            close(filedes[READ]);
            close(filedes[WRITE]);

            //Do buildIn command or just execute command
            if(func1) {
                (*func1)(cmd1);
                exit(EXIT_SUCCESS);
            } else {
                execvp(cmd1[0], cmd1);
            }
        }
        close(filedes[WRITE]);
        close(filedes[READ]);
        waitpid(pid2, NULL, 0);

        //Do buildIn command or just execute command
        if(func2) {
            (*func2)(cmd2);
            exit(EXIT_SUCCESS);
        } else {
            execvp(cmd2[0], cmd2);
        }
    }
    close(filedes[WRITE]);
    close(filedes[READ]);
    waitpid(pid2, NULL, 0);
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

/*----------------Below are built-in functions---------*/
//A BuiltIn function to return address of all of build-in command except quit() and cd()
void (*builtIn(char **cmd)) (char **cmd) {
    void (*func)(char **) = NULL; 
    if(strcmp(cmd[0], "clr") == 0) {
        return &myClr;
        //can't directly call and then 'return 1;' since return makes pointer from integer without a cast
        //Therefore, use function pointer to call function instead
    } 
    else if(strcmp(cmd[0], "dir") == 0) {
        return &myDir;
    } 
    else if(strcmp(cmd[0], "environ") == 0) {
        return &myEnviron;
    } 
    else if(strcmp(cmd[0], "echo") == 0) {
        return &myEcho;
    } 
    else if(strcmp(cmd[0], "help") == 0) {
        return &myHelp;
    } 
    else if(strcmp(cmd[0], "pause") == 0) {
        return &myPause;
    }
    return NULL;
}

//Change directory function
void myCd(char **cmd){
    if(!cmd[1] || strcmp(cmd[1], " ") == 0) {
        chdir(cmd[1]);
    } 
    else {
        chdir(getenv("HOME")); 
    }
}

/* clean the screen */
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

    //if find directory, print them out
    if(d != NULL) {
        while((dir = readdir(d))) {
            if(strcmp(".", dir->d_name) == 0 || strcmp("..", dir->d_name) == 0)
                continue;
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
}

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

//print current dir and /readme at the begin, and then forking to create the readme child process 
void myHelp() {
    char current[SET_SIZE] = "";
    pid_t pid;
    strcat(current, getenv("PWD"));
    strcat(current, "/readme");
    pid = fork();
    if(pid == 0) {
        printf("Cannot find and descripting.");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

/* Till user hits the enter */
void myPause() {
    while(getchar() != '\n') {
        ;
    }
}

//Quit myshell
void myQuit() {
    exit(EXIT_SUCCESS);
}

/*
Give shell a "myshell>" prompt to indicate this is a built shell
*/
void myPrompt() {
    char pwd[1024];
    if(!getcwd(pwd, sizeof(pwd))) {
        printf("Cannot get current directory.");
    } 
    else {
        printf("%s/myshell>", pwd);      
    }
}

//I meet a segment fault when I write I/O redirection, I study this funtion to free rest I/O from stack overflow
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