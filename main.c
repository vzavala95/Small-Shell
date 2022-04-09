/****************************
 Name: Victoria Zavala
 Class: CS 344 - Operating Sytems 
 Program 3 - SMALLSH

 Citations are also referenced in-line but can also be found here:
 1. https://www.includehelp.com/c/process-identification-pid_t-data-type.aspx
 2. https://linuxhint.com/signal_handlers_c_programming_language/
 3. https://www.ibm.com/docs/en/ztpf/2020?topic=zca-wifexitedquery-status-see-if-child-process-ended-normally
 4. https://www.geeksforgeeks.org/fork-system-call/
 5. https://www.tutorialspoint.com/c_standard_library/c_function_perror.htm
 6. https://stackoverflow.com/questions/27541910/how-to-use-execvp
 7. https://www.journaldev.com/40793/execvp-function-c-plus-plus
 8. https://docs.microsoft.com/en-us/cpp/c-language/switch-statement-c?view=msvc-170
 9. https://linux.die.net/man/3/waitpid
 10. https://www.cplusplus.com/reference/cstdio/clearerr/
 11. https://www.geeksforgeeks.org/getline-string-c/
 12. https://www.ibm.com/docs/en/ztpf/2019?topic=zca-wtermsig-determine-which-signal-caused-child-process-exit
 13. https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
 14. https://man7.org/linux/man-pages/man3/getenv.3.html
 15. https://man7.org/linux/man-pages/man2/dup.2.html
 16. https://www.ibm.com/docs/en/aix/7.2?topic=management-process-termination
 17. https://www.geeksforgeeks.org/making-linux-shell-c/
 18. https://brennan.io/2015/01/16/write-a-shell-in-c/
 19. https://medium.com/swlh/tutorial-to-code-a-simple-shell-in-c-9405b2d3533e
 20. https://stackoverflow.com/questions/1500004/how-can-i-implement-my-own-basic-unix-shell-in-c
 21. Code from Module 5 - Exploration: Processes and I/O
******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#define MAX_PROCESSES 100

/*********************************************
Struct for running command line prompts
*********************************************/

struct shell_copy {
    // holds all required arguments for shell
    char *args[512];
    // 512 char limit 
    char *inputf;
    // path for input file
    char *outputf;
    // path for output file 
    bool bground;
    int argumentsh;
    // number of required arguments
};

/********************************************
Global variable declarations
********************************************/

int runsh = 1;
// count set to 1
int ch_stat = 0;
pid_t pid_chld;
// reference: https://www.includehelp.com/c/process-identification-pid_t-data-type.aspx
pid_t bgroundpid[MAX_PROCESSES] = {0};
struct shell_copy *usr_input;
struct sigaction SIGINT_action = {0};
// reference: https://linuxhint.com/signal_handlers_c_programming_language/
struct sigaction SIGTSTP_action = {0};
struct sigaction SIGCHILD_action = {0};
// initializing signal handlers
bool foreground_mode = false;


/********************************************
Function declarations
********************************************/

void SIGTSTP_handle(int sig);
void SIGCHILD_handle(int sig);
void make_input();
void make_output();
void display_exit(int status);
void display_stat(int status);
void free_shell();
void exit_shell();
void go_shell();
// necessary functions for main
char *grow_tok(char *in_token);
// used in replacing var $$
struct shell_copy *displaysh();
int dir_change();

/********************************************
Main Function
********************************************/

int main() {
    do {
        usr_input = displaysh();
        // display prompt
        go_shell();
        free_shell();
    } while(runsh);
    // loop until runsh is 0

    return 0;
}

/********************************************
Execute commands in shell 
********************************************/

void go_shell() {
    // executes shell 
    SIGINT_action.sa_handler = SIG_IGN;
    // handles SIGINT
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    SIGTSTP_action.sa_handler = SIGTSTP_handle;
    // handles SIGTSTP for commands
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    SIGCHILD_action.sa_handler = SIGCHILD_handle;
    // handles SIGCHILD for commands
    sigfillset(&SIGCHILD_action.sa_mask);
    SIGCHILD_action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &SIGCHILD_action, NULL);

    
    if(usr_input->args[0] != NULL && *usr_input->args[0] != '\n') {
        // if there is a command to execute, run shell
        if(strncmp(usr_input->args[0], "exit", 4) == 0) {
           // exits shell
            exit_shell();
        } else if(strncmp(usr_input->args[0], "cd", 2) == 0) {
            // if user enters cd, change directory 
            dir_change(usr_input);
        } else if(strncmp(usr_input->args[0], "status", 6) == 0) {
            // if user enters status, show status 
            if(WIFEXITED(ch_stat)) {
                // see if child process has ended normally
                // reference: https://www.ibm.com/docs/en/ztpf/2020?topic=zca-wifexitedquery-status-see-if-child-process-ended-normally
                display_exit(ch_stat);
                // if exited normally, display exit status
            } else {
                display_stat(ch_stat);
                // ir not display status 
            }
        } else {
            // create new child process and execute after fork() system call 
            // reference: https://www.geeksforgeeks.org/fork-system-call/
            pid_chld = fork();
            switch(pid_chld) {
                case -1:
                    perror("sorry! fork() unfortunately failed\n");
                    // reference: https://www.tutorialspoint.com/c_standard_library/c_function_perror.htm
                    fflush(stdout);
                    // use after outputting text to make sure text reaches the screen
                    // reference: Program 3 Assignment Page
                    break;
                case 0:
                    if(usr_input->bground) {
                        // case 2 if no redirect
                        if(usr_input->inputf != NULL) {
                            usr_input->inputf = "/dev/null";
                            make_input(usr_input);
                        }
                        if(usr_input->outputf != NULL) {
                            usr_input->outputf = "/dev/null";
                            make_output(usr_input);
                        }
                    }

                    if(usr_input->bground == false) {
                        // foreground process has to teminate for SIGINT
                        SIGINT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &SIGINT_action, NULL);
                        SIGTSTP_action.sa_handler = SIG_IGN;
                        // foreground process has to ignore SIGTSTP
                        sigaction(SIGTSTP, &SIGTSTP_action, NULL);
                    } else if(usr_input->bground == true) {
                        SIGINT_action.sa_handler = SIG_IGN;
                        // ignore SIGINT
                        sigaction(SIGINT, &SIGINT_action, NULL);
                        //ignore SIGTSTP
                        SIGTSTP_action.sa_handler = SIG_IGN;
                        sigaction(SIGTSTP, &SIGTSTP_action, NULL);
                    }

                    
                    if(usr_input->inputf != NULL) {
                        // if input file exists 
                        make_input(usr_input);
                    }
                    
                    if(usr_input->outputf != NULL) {
                        // if output file exists
                        make_output(usr_input);
                    }
                    pid_chld = getpid();

                    execvp(usr_input->args[0], usr_input->args);
                    // I did not understand how to use this command until I referenced https://stackoverflow.com/questions/27541910/how-to-use-execvp
                    // Other reference: https://www.journaldev.com/40793/execvp-function-c-plus-plus
                    perror("Error! No such file or directory\n");
                    fflush(stdout);
                    // always gotta flush when you display text!
                    exit(1);
                default:
                // executed if no case constant-expression value is equal to the value of expression
                // reference: https://docs.microsoft.com/en-us/cpp/c-language/switch-statement-c?view=msvc-170
                   
                    if(foreground_mode == true) {
                        // if true run all processes in foreground and save child pid
                        waitpid(pid_chld, &ch_stat, 0);
                        // wait for child process to finish 
                        // reference: https://linux.die.net/man/3/waitpid
                        if(WIFSIGNALED(ch_stat)){
                            // if child process ended abnormally, print status
                            display_stat(ch_stat);
                        }
                    } else {
                        if(usr_input->bground == false) {
                            // no bground prcess
                            waitpid(pid_chld, &ch_stat, 0);
                            // same logic as condition above ^ 
                            if(WIFSIGNALED(ch_stat)){
                                display_stat(ch_stat);
                            }
                        } else {
                            for(int i = 0; i < MAX_PROCESSES; i++) {
                                // iterate through and save pid as array
                                if(bgroundpid[i] == 0) {
                                    bgroundpid[i] = pid_chld;
                                    break;
                                    // stop loop 
                                }
                            }
                            printf("Background child PID %d is starting\n", pid_chld);
                            fflush(stdout);
                            // make sure to flush after displaying text
                        }
                    }
            }
        }
    }
}

/********************************************
Displays prompts to user and receives user commands in the shell 
********************************************/

struct shell_copy *displaysh() {
    int tnum = 0;
    char *temp = NULL;
    char *saveptr;
    char *token = NULL;
    size_t len = 0;
    struct shell_copy *command = malloc(sizeof(struct shell_copy));
    command->inputf = NULL;
    command->outputf = NULL;
    command->bground = false;
   
    clearerr(stdin);
    // clear errors 
    // reference: https://www.cplusplus.com/reference/cstdio/clearerr/
    write(STDOUT_FILENO, ": ", 2);
    // display : command prompt 
    fflush(stdout);
    // flush after text display
    getline(&temp, &len, stdin);
    // reads line from input 
    // reference: https://www.geeksforgeeks.org/getline-string-c/
    temp[strlen(temp)-1] = '\0';

    if(strlen(temp) != 0) {
        // receives token if shell is empty
        token = strtok_r(temp, " ", &saveptr);
    }
    if(token != NULL && *token != '#') {
        // ignore any comments
        while(token != NULL) {
            switch(*token) {
                case '<':
                // input file
                    token = strtok_r(NULL, " ", &saveptr);
                    // go to next token
                    token = grow_tok(token);
                    command->inputf = calloc(strlen(token) + 1, sizeof(char));
                    strcpy(command->inputf, token);
                    // save that token 
                    break;
                case '>':
                    // output file
                    token = strtok_r(NULL, " ", &saveptr);
                    token = grow_tok(token);
                    command->outputf = calloc(strlen(token) + 1, sizeof(char));
                    // same logic as above 
                    strcpy(command->outputf, token);
                    break;
                case '&':
                    command->bground = true;
                    break;
                default:
                    token = grow_tok(token);
                    command->args[tnum] = calloc(strlen(token) + 1, sizeof(char));
                    strcpy(command->args[tnum], token);
                    // save token
                    tnum++;
            }
            if(token != NULL) {
                if(token[0] != '&') {
                    free(token);
                    // deallocate memory for token 
                }
            }
            token = strtok_r(NULL, " ", &saveptr);
        }
        command->args[tnum] = NULL;
        // array is ended
        command->argumentsh = tnum;
    } else {
        command->args[0] = NULL;
        // condition to ignore # or ay blank spaces 
    }
    if(temp != NULL) {
        free(temp);
        // deallocate memory
    }
    return command;
}


/********************************************
Expands variable $$ and replaces with process ID
********************************************/

char *grow_tok(char *in_token) {
    char *out_tok = calloc(4 * strlen(in_token), sizeof(char));
    // limit size of token because of max pid of 7

    int i = 0;
    int j = 0;
    // init pointers
    
    while(in_token[i] != '\0') {
        if(in_token[i] == '$' && in_token[i+1] == '$') {
            int pid = getpid();
            char str_pid[8] = {'\0'};
            // convert retrived pid to string to read it 
            sprintf(str_pid, "%d", pid);
            strcat(out_tok, str_pid);
            i += 2;
            j += (int)strlen(str_pid);
            // update pointers 
        } else {
            out_tok[j] = in_token[i];
            i++;
            j++;
        }
    }
    if(in_token[i] != '\0') {
        free(in_token);
        // deallocate memory 
    }

    return out_tok;
}

/********************************************
Prints out either the exit status or the terminating signal of the last foreground process 
********************************************/

void display_exit(int status) {
    
    if(WIFEXITED(status)) {
        printf("exit status %d\n", WEXITSTATUS(status));
        // if no abnormalities display exit 
        fflush(stdout);
        // flush after displaying text 
    }
}

/********************************************
Displays terminating signal of last foreground process
********************************************/

void display_stat(int status) {
    if(WIFSIGNALED(status)) {
        // if stopped by signal 
        printf("terminated by signal %d\n", WTERMSIG(status));
        // reference: https://www.ibm.com/docs/en/ztpf/2019?topic=zca-wtermsig-determine-which-signal-caused-child-process-exit
        fflush(stdout);
    }
}

/********************************************
Handles SIGTSTP signals
********************************************/

void SIGTSTP_handle(int sig) {
    if(foreground_mode == false) {
        // if not in foreground mdoe already
        char* forenter = "entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, forenter, 49);
        fflush(stdout);
        foreground_mode = true;
    } else {
        // if already in foreground mode 
        char* foreexit = "exiting foreground-only mode\n";
        write(STDOUT_FILENO, foreexit, 29);
        fflush(stdout);
        foreground_mode = false;
    }
}

/********************************************
Handles parent signals and all children of that parent signal 
********************************************/

void SIGCHILD_handle(int sig) {
    pid_t spawned_pid = waitpid(-1, &ch_stat, WNOHANG);
    // execute in background depending on conditions 
    // reference: https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
    
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(bgroundpid[i] == spawned_pid && spawned_pid > 0) {
            if(WIFEXITED(ch_stat)){
                // if ended normally display exit status 
                printf("background child PID %d is done with exit status %d\n", spawned_pid, WEXITSTATUS(ch_stat));
                fflush(stdout);
            } else if(WIFSIGNALED(ch_stat)) {
                // if not ended normally display "terminated by signal"
                printf("background child PID %d is terminated by signal %d\n", spawned_pid, WTERMSIG(ch_stat));
                fflush(stdout);
            }
            bgroundpid[i] = 0;
            // return to 0
        }
    }
    for(int i = 0; i < MAX_PROCESSES; i++) {
        // remove from array 
        if(bgroundpid[i] == spawned_pid) {
            bgroundpid[i] = 0;
            break;
        }
    }
}

/********************************************
Free shell_copy memory 
********************************************/

void free_shell() {
    int hold = 0;
    while(usr_input->args[hold] != NULL) {
        // set pointers memory free 
        free(usr_input->args[hold]);
        hold++;
    }
    free(usr_input->args);
    if (usr_input->inputf != NULL) {
        free(usr_input->inputf);
        // deallocate input pointer 
    } else if (usr_input->outputf != NULL) {
        free(usr_input->outputf);
        // deallocate output pointer 
    }
}

/********************************************
cd command - changes working directory of smallsh
********************************************/

int dir_change() {
    char *home = NULL;
    char *pass = NULL;
    if(usr_input->argumentsh == 1) {
        home = getenv("HOME");
        // automatically directs to home directory if one is not specified
        // reference: https://man7.org/linux/man-pages/man3/getenv.3.html
        chdir(home);
    } else { 
        chdir(usr_input->args[1]);
        // changes working directory according to specified argument 
    }
    return 0;
}

/********************************************
Process input file 
********************************************/

void make_input() {
    // Process input file, if any
    int source = open(usr_input->inputf, O_RDONLY);
    // read-only
    if(source == -1) {
        // no input file specified 
        perror("Error! cannot open input file\n");
        exit(1);
    }

    int destination = dup2(source, 0);
    // duplicate file descriptor 
    // reference: https://man7.org/linux/man-pages/man2/dup.2.html
    //redirect 
    if(destination == -1) {
        perror("Error! cannot redirect from source file");
        exit(2);
    }
}

/********************************************
Process output file 
********************************************/

void make_output() {
    // similar logic to input file except different permissions of course 
    int source2 = open(usr_input->outputf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(source2 == -1) {
        // no output file specified 
        perror("Error! cannot open output file\n");
        fflush(stdout);
        exit(1);
    }
    int destination = dup2(source2, 1);
    if(destination == -1) {
        //redirect 
        perror("Error! cannot redirect to output file\n");
        fflush(stdout);
        exit(2);
    }
}

/********************************************
Kills off any existing processes and exits shell :) 
********************************************/

void exit_shell() {
    // instructs shell to stop running 
    runsh = 0;
    for(int i = 0; i < MAX_PROCESSES; i++) {
        // goodbye background processed
        if(bgroundpid[i] > 0) {
            kill(bgroundpid[i], SIGTERM);
            // terminates program 
            // reference: https://www.ibm.com/docs/en/aix/7.2?topic=management-process-termination
        }
    }
}