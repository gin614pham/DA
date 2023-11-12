


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>



#define MAX_COMMAND_LENGTH 1024
#define COMMAND_LIST "c", "d", "r", "m", "l", "i", "h"

void changeToHomeDirectory() {
    char *homeDirectory = getenv("HOME");
    if (homeDirectory != NULL) {
        if (chdir(homeDirectory) < 0) {
            perror("chdir");
        }
    } else {
        perror("getenv");
    }
}

// get path of current directory
char *get_current_directory(char *buffer, size_t size) {
    if (getcwd(buffer, size) == NULL) {
        perror("getcwd");
        return NULL;
    }
    return buffer;
}


// run file myFileManager
void runMyFileManager(char *command, char *path, char *cpath) {
    pid_t pid;
    if ((pid = fork()) == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        int argc = 1;
        char *argv[MAX_COMMAND_LENGTH];
        argv[0] = "./myFileManager";
        char *token = strtok(command, " ");
        while (token != NULL) {
            argv[argc] = token;
            token = strtok(NULL, " ");
            argc++;
        }

        // argv[argc] = PATH
        argv[argc] = path;
        // argv[argc+1] = NULL
        argv[argc+1] = NULL;

        if (chdir(cpath) < 0) {
        perror("chdir");
        exit(1);
        }
        // execute command
        execvp("./myFileManager", argv);
        perror("execlp");
        exit(1);
    }
    waitpid(pid, NULL, 0);

    
}


int main() {
    // save current path directory
    char cpath[MAX_COMMAND_LENGTH];
    if (getcwd(cpath, sizeof(cpath)) == NULL) {
        perror("getcwd");
        exit(1);
    }
    
    pid_t pid;
    if ((pid = fork()) == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
    execlp("gcc", "gcc", "-o", "myFileManager", "File.c", NULL);
    perror("execlp");
    exit(1);
    } else {
        waitpid(pid, NULL, 0); // Wait for the child process to finish
    }

    while(1){
        changeToHomeDirectory();
        char command[1024];
        // print current directory and prompt
        char cwd[1024];
        get_current_directory(cwd, sizeof(cwd));
        printf("%s> ", cwd);
        // read command
        fgets(command, 1024, stdin);
        // remove newline
        command[strcspn(command, "\n")] = '\0';
        printf("command: %s\n", command);
        

        // if (strncmp(command, "exit", 4) == 0) {
        //     break;
        // }
        // run file myFileManager
        runMyFileManager(command, cwd, cpath);
        
        
    }
        
}