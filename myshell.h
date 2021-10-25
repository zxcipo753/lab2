#ifndef MYSHELL_H
#define MYSHELL_H
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

const int READ = 0;
const int WRITE = 1;
extern char **environ;

void myCd(char **cmd);
void myClr();
void myDir(char **cmd);
void myEcho(char **cmd);
void myEnviron();
void myHelp();
void myPause();
void myQuit();
void myBatch(char *batchfile);
void mainLoop();


char *readInput(void);
char **parse_line(char *input);
int myExecute(char **cmd);
void myPipe(void func(char **), char **cmd, int found);
int parse_pipe(char **cmd);

FILE *inRedirection(char **cmd) ;
FILE *outRedirection(char **cmd) ;
void resetIO(FILE *in_container, FILE *out_container, int inNum, int outNum, int errNum);
void myPrompt();
int myBackground(char **cmd);

void (*builtIn(char **cmd))(char **cmd);

#endif