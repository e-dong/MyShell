/* 
 * *************************************************** *
 * Author: Eric Dong 						   		   *
 * Source Code Creation Date: 11/22/2016 			   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

void external_shell_cmd(char *arguments[30], int flag, int number_of_commands); // function that runs any UNIX command, with or without arguments
char input_line[100]; // array of characters that holds a line of input
char *command[30];   // array of Strings that holds the command and any arguments
char *token;         // represents a token from the line of input
int line_num = 0;    // the current line number in the shell


int main() {
    while(1) {
        // makes sure all possible elements are initialized as NULL
        int j;
        for (j = 0; j < 30; ++j) {
            command[j] = NULL; 
        }
        int num_cmds = 0; // will hold number of commands
        printf("< %d > myshell $ ", ++line_num); // will print out "< line number > myshell $ " everyline 
        fgets(input_line, sizeof(input_line), stdin); // get 1 line of input
       
        // get first token, finds the delimiters and replace them with '\0' 
        token = strtok(input_line, " ");
         
        // tokenize the rest of the input and place the tokens in to the command array
        while (token != NULL) {
           command[num_cmds] = token;
           token = strtok(NULL, " "); 
           ++num_cmds; // count the number of commands
        }
        // remove any trailing new line characters
        for (j = 0; j < num_cmds; ++j) {
            command[j] = strtok(command[j], "\n");
        }
         
        // if the user just hits enter without entering a command, make it display the same < line number >
        //  and also alow user to enter input again via continue to next iteration  
        if (command[0]  == NULL) { 
            line_num = line_num - 1;
            continue;
        } 
         
        // INTERNAL SHELL COMMAND: exit
        if(strcmp(command[0], "exit") == 0) 
        {
            exit(EXIT_SUCCESS); // exit(0), normal exit from shell
        }

        // INTERNAL SHELL COMMAND: cd (change directory)
        else if(strcmp(command[0], "cd") == 0) 
        {
            // execute this code when the user has entered a path (absolute or relative) 
            if (command[1] != NULL) {
                int cd_status = chdir(command[1]);
                // if chdir returns -1, then display the error interpreted by errno
                if (cd_status == -1) {
                    fprintf(stderr, "-myshell: cd: %s: %s\n", command[1], strerror(errno));
                }
           } else {
                chdir(getenv("HOME")); // typing "cd" with no arguments go back to the home directory
           }
                      
        }
        // if the input is not a built in function, 
        // then it must be an external program (e.g ls, pwd, more, etc.)
        else 
        {
            // check to see if the user inputed ">", ">>", or "<"
            // note: that the 2nd to last element may have ">", ">>", or "<"
            // note: least number of commands is 3, e.g ls > foo.
            if(command[num_cmds - 2] != NULL && num_cmds >= 2) {
                 
                if ( strcmp(command[num_cmds - 2], ">\0") == 0) {
                    // handle the case where the user does not input the name of file e.g user enters "ls > \n"
                    // similarly done for ">>" and "<"  
                    if (command[num_cmds - 1] == NULL || strcmp(command[num_cmds - 1], ">\n") == 0 ) {
                        fprintf(stderr, "-myshell: syntax error near unexpected token 'newline'\n");
                    } else { 
                        external_shell_cmd(command, 1, num_cmds); // middle param: flag 1 to redirect output to a file
                    } 
                } else if (strcmp(command[num_cmds - 2], ">>\0") == 0) {
                     if (command[num_cmds - 1] == NULL || strcmp(command[num_cmds - 1], ">>") == 0 ) {
                        fprintf(stderr, "-myshell: syntax error near unexpected token 'newline'\n");
                    } else {
                        external_shell_cmd(command, 2, num_cmds); // middle param: flag 2 to redirect output to append to a file
                    } 
                } else if (strcmp(command[num_cmds - 2], "<\0") == 0) {
                     if (command[num_cmds - 1] == NULL || strcmp(command[num_cmds - 1], ">") == 0 ) {
                         fprintf(stderr, "-myshell: syntax error near unexpected token 'newline'\n");
                     } else {
                         external_shell_cmd(command, 3, num_cmds); // middle param: flag 3 to redirect input from a file
                     } 
                }
                else { // if the user did not input ">" || ">>" || "<", then output to screen
                    external_shell_cmd(command, 0, num_cmds);
                }          
           } else { // this code executes when the number of commands is less than 3, e.g (ls -l)  
               external_shell_cmd(command, 0, num_cmds); 
           } 
        }    
    }    
    return 0;
}
/* This function is responsible for handling Any Unix command that is not internal to this shell
 * Generally, The new program is run by using syscall to fork and execvp. the name of the program and any arguments are
 * passed to execvp. 
 * parameters: 
 *     (1) char *command[30] - the array of tokenized commands
 *     (2) int flag          - 0 *represets whether to ouput to terminal screen
 *                           - 1 *write output to file 
 *                           - 2 *append output to file
 *                           - 3 *get input from file 
 *     (3) int num_commands  - the number of commands
 * 
 * return value:             - void
 */
void external_shell_cmd(char *command[30], int flag, int num_commands) {
     FILE *fp; 
     char *filename;
    
     
     int execvp_status = 0;
     if (flag == 0) { // if flag is 0, output to screen
         if (fork() == 0) {
            // command[0] - program name
            // command[n] - where n must be less than 30, command line arguments, may be NULL if there is no arguments inputted by the user
            
            execvp_status = execvp(command[0], command);
            // if execvp returns -1, print error
            if(execvp_status  == -1) {
                fprintf(stderr, "-myshell: %s: command not found\n",command[0]);
                exit(EXIT_FAILURE); 
            }
        } else {
            int status;
            wait(&status); // wait for child process to finish execution
        }
    } else if (flag == 1) { // if flag is 1, output to file
            filename = command[num_commands - 1]; // filename is always the last element of command array regardless of length
         if (fork() == 0) {
            // if file does not exist a new one is created with the specified name
            // if the file already exists, its content is erased and it is considered as a new file for writing
            fp = freopen(filename, "w", stdout);
            if (fp == NULL)
                printf("cannot write\n");
             //printf("%s, %s\n", command[num_commands - 2], command[num_commands - 1]);
            // change non-arguments to program to NULL and pass into execvp
            command[num_commands - 1] = NULL;  // (change filename to NULL) 
            command[num_commands - 2] = NULL;  // (change ">" to NULL)
            execvp_status = execvp(command[0], command);
            fclose(fp);

            if(execvp_status  == -1) {
                fprintf(stderr, "-myshell: %s: command not found\n",command[0]);
                exit(EXIT_FAILURE); 
            }
        } else {
            int status; // wait for child process to terminate
            wait(&status);
        }
    } else if (flag == 2) { // if flag is 2, append to file
         filename = command[num_commands - 1]; 
         if (fork() == 0) {
            // if file doesn't exist, a new one will be created with the specified name
            // if the file exists, the program will append its output to the file
            fp = freopen(filename, "a", stdout);
            // change non-arguments to program to NULL and pass into execvp
            command[num_commands - 1] = NULL;  // (change filename to NULL) 
            command[num_commands - 2] = NULL;  // (change ">>" to NULL)

            execvp_status = execvp(command[0], command);
            fclose(fp);

            if(execvp_status  == -1) {
                fprintf(stderr, "-myshell: %s: command not found\n",command[0]);
                exit(EXIT_FAILURE); 
            }
        } else {
            int status;
            wait(&status);
        }
    } else if (flag == 3) { // if flag is 3, read input from file
         filename = command[num_commands - 1];
         if (fork() == 0) {
            // File must exist inorder to read it
            fp = freopen(filename, "r", stdin);
            // if fp returns null, then the file does not exist
            if (fp == NULL) {
                fprintf(stderr, "-myshell: %s: %s\n", filename, strerror(errno));
            }
            // change non-arguments to program to NULL and pass into execvp
            command[num_commands - 1] = NULL;  // ( change filename) 
            command[num_commands - 2] = NULL;  // ( change "<" to NULL)
       
            execvp_status = execvp(command[0], command);
            fclose(fp);

            if(execvp_status  == -1) {
                fprintf(stderr, "-myshell: %s: command not found\n",command[0]);
                exit(EXIT_FAILURE); 
            }
        } else {
            int status; // wait for child process to terminate
            wait(&status);
        }

    } 
}
