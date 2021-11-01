#include "lab02myshell.h"

int main(int argc, char **argv) {
     //Set environment
    char pwd[SET_SIZE];
    if(!getcwd(pwd, sizeof(pwd))) {
        printf("Cannot get current directory.");
        return EXIT_FAILURE;
    }
    strcat(pwd, "/myshell"); 
    setenv("shell", pwd, 1);

    //Check if argument has batchfile.txt to run; if not, enter the shell loop to allow user's input
    char *batchfile = NULL;
    if(argv[1]) {
        batchfile = argv[1];
        myBatch(argv[1]);
    }
    mainLoop();
    return EXIT_SUCCESS;
}

//Run batch file before entering the Shell Loop
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
            
            //Reset, free pointers
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


//Shell Loop
void mainLoop() {
    char *input;
    char **argument;

    int inCp  = dup(STDIN_FILENO);
    int outCp = dup(STDOUT_FILENO);
    int errCp = dup(STDERR_FILENO);

    // set file to empty before shell starts
    FILE *in = NULL;
    FILE *out = NULL;

    int start_loop = 1;
    while(start_loop) {
        myPrompt();
        input = readInput();
        //Parse and split user's input to different argument
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

        //Reset stdIN/OUT/ERR and free pointers
        resetIO(in, out, inCp, outCp, errCp);
        free(argument);
        free(input);
    }
}

/*---------Below are functions to parse user's input---------*/
char *readInput(void) {
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
        myQuit();
    } else if(strcmp(cmd[0], "cd") == 0) {
        myCd(cmd);
    } else {
        pid_t pid;

        //save builtIn function address, see builtIn() function below
        void (*func)(char **) = NULL;
        func = builtIn(cmd);

        //parse command to find if there is a command argument. 
        int found;//this means find '|'
        found = parse_pipe(cmd);
        if(found != 0) {
            myPipe(func, cmd, found);
        } else if((pid = fork()) == 0) {

            //First pipe builtIn function; if not, move forward to external function
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
            //Background processing
            if(!myBackground(cmd)){
                waitpid(pid, NULL, 0);
            }
        }
        return 0;
    }
    return 0;
}

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

//Use cmd and cmd2 to represent input argument 1 and 2. Call buildIn(cmd) to get fun and fun2 in the pipe.
void myPipe(void func(char **), char **cmd, int found) {
    int pid, pid2;
    int filedes[2];
    char **cmd2 = cmd + found;
    pipe(filedes);

    void (*func2)(char **) = NULL;
    func2 = builtIn(cmd2);//Save built-in functions address

  //Forking to create a child, and then do stdin or stdout, then close
    if((pid2 = fork()) == 0) {
        dup2(filedes[READ], STDIN_FILENO);
        if((pid = fork()) == 0) {
            dup2(filedes[WRITE], STDOUT_FILENO);
            close(filedes[READ]);
            close(filedes[WRITE]);

            //Do first argument; Do buildIn command or just execute command
            if(func) {
                (*func)(cmd);
                exit(EXIT_SUCCESS);
            } else {
                execvp(cmd[0], cmd);
            }
        }
        close(filedes[WRITE]);
        close(filedes[READ]);
        waitpid(pid, NULL, 0);

        //Do Second augument; Do buildIn command or just execute command
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


//Backgound execution in shell. The shell will immediately return when find '&' at the end of input
int myBackground(char **cmd) {
    int i = 0;
    while(cmd[i]) {
        i++;
    }
    //immediately retrun when find '&' at the end of input
    if(strcmp(cmd[i-1], "&") == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* Simple prompt function, displace the shells prompt */
void myPrompt() {
    char pwd[1024];
    if(!getcwd(pwd, sizeof(pwd))) {
        printf("Cannot get current directory.");
    } 
    else {
        printf("%s/myshell>", pwd);      
    }
}

/* Shell pointer a friend helped me with redirection problems, its basically
    another for of delimiter that replaces the next argument */
void shPointer(char **argument, int direction) {
    int i = direction;
    while(argument[i+1]) {
        argument[i] = argument[i+1];
        i++;
    }
}


/*----------------Below are built-in functions---------*/
//A BuiltIn function to return address of all of build-in command except quit() and cd()
void (*builtIn(char **argument))(char **argument) {
    void (*func)(char **) = NULL; 
    if(strcmp(argument[0], "clr") == 0) {
        return &myClr;
         //can't directly call and then 'return 1;' since return makes pointer from integer without a cast
        //Therefore, use function pointer to call function instead
    } 
    else if(strcmp(argument[0], "dir") == 0) {
        return &myDir;
    } 
    else if(strcmp(argument[0], "environ") == 0) {
        return &myEnviron;
    } 
    else if(strcmp(argument[0], "echo") == 0) {
        return &myEcho;
    } 
    else if(strcmp(argument[0], "help") == 0) {
        return &myHelp;
    } 
    else if(strcmp(argument[0], "pause") == 0) {
        return &myPause;
    }
    return NULL;
}

/* Change Directory function that we went over in class and slides */
void myCd(char **cmd) {
    if(!cmd[1] || strcmp(cmd[1], " ") == 0) {
        chdir(getenv("HOME")); 
    } else {
        chdir(cmd[1]);
    }
}

/* clean the screen */
void myClr() {
    printf("\e[1;1H\e[2J");
}

/*
list all content in this directory
*/
void myDir(char **cmd) {
    char *destination = cmd[1];
    DIR *d;
    struct dirent *dir;

    //perror without current directiory
    if((d = opendir(destination)) == NULL) {
        printf("Cannot get current dir.");
        return;
    }

    //if find directory, print all files in this directory
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

//Display help mannul, require user to press 'Enter' to continue
void myHelp(char **argument) {
    char current[900] = "";
    pid_t pid;

    
    //Get current directory and append '/readme' at the end of the path
    strcat(current, getenv("PWD"));
    strcat(current, "/readme");

    //fork and create a child process. After read all of then, kill the child process
    pid = fork();
    if(pid == 0) {
        execlp("more", "more", current, NULL);//Use 'more' to execute
        printf("Cannot find and descripting.");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

//Till user hits 'Enter'
void myPause() {
    while(getchar() != '\n') {
        ;
    }
}

//Quit myshell
void myQuit() {
    exit(EXIT_SUCCESS);
}


//Below are I/O redirection functions
//Use while loop to find '<' symbol, then inputs that file to the previous argument.
//Allow multiple arguments per time
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

                //move pointer to next argument, this will allow multi arguments as input
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

// Check if the input has symbol '>' or '>>', then outputs argument to a file(create one if no specified file).
//'>' will overwrite the file's original content;'>>' will continue to write
//Only allow one argument per time
FILE *outRedirection(char **cmd) {
    FILE *file1 = NULL;
    char mode[] = "";// determind the mode(w/a) to open file: 'a' respond to '>>'; 'w' respond to '>'
    int counter;

    // use a loop to travese all input to find symbol '>' or '>>'
    int index = 1;
    while(cmd[index]) {
        if(strcmp(cmd[index], ">") == 0) {
            mode[0] = 'w';
            counter = index;
        } else if(strcmp(cmd[index], ">>") == 0) {
            mode[0] = 'a';
            counter = index;
        }

        //mode 'a' will continue to write; mode 'w' will overwrite. If there is no specified file, create one
        if(strcmp(mode, "") != 0) {
            if(cmd[index++]) {
                file1 = fopen(cmd[index], mode);
                if(file1 == NULL) {
                    printf("File does not exist.");
                }
                cmd[counter] = NULL;
                return file1;
                }
        }
            index++;
    }
    return file1;
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
