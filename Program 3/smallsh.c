#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv){
  // Variables
  char line[2048] = "";
  char* token;
  int status = 0;
  pid_t pid_array[100]; // static arrays are initialized as 0
  int n_pid = 0; // Number of process ids in the pid_array - used with exit command to minimize array search time.
  int err = 0; // Hold error values from functions
  int i = 0; // hold number for loops

  // Commandline loop
  do {
    // Check for completed background tasks
    if (n_pid != 0) {
      for (i = 0; i < 100; i++) {
        if (n_pid == 0) {
          break;
        }
        // Check if valid id
        if(pid_array[i] == 0){}
        // Wait for child
        else{
          if (waitpid(pid_array[i], &status, WNOHANG) == pid_array[i]) {
            pid_array[i] = 0; // Set pid to 0
            n_pid--;
          }
        }
      }

    }

    // Print prompt
    printf(": ");
    fflush(stdout);
    // Read line
    // Check for error
    if (fgets(line, 2048, stdin) == NULL) {
      printf("%s\n", "Failed to get line.");
      fflush(stdout);
    }
    else{
      token = strtok(line, " "); // Get first token, which should be the command.
      if(token == "#"){} // Skip if comment
      // Else: NOT a comment
      else{
        // Command is 'exit'
        if (strcmp(token, "exit") == 0) {
          for (i = 0; i < 100; i++) {
            // Exit once all children have been handled
            if (n_pid == 0) {
              exit(0);
            }
            // Check if valid id
            if(pid_array[i] == 0){}
            // Kill child process
            else{
              err = kill(pid_array[i], SIGTERM);
              if (err != 0) {
                printf("%s\n", "Error killing child process.");
                fflush(stdout);
              }
              pid_array[i] = 0; // Set pid to 0 once killed
              n_pid--;
            }
          }
        }
        // Command is 'status'
        else if (strcmp(token, "status") == 0) {
          printf("%d\n", status);
          fflush(stdout);
        }
        // Command is 'cd'
        else if (strcmp(token, "cd") == 0) {
          token = strtok(NULL, " "); // Should be the path to a new directory, or NULL.
          if (token == NULL) {
            err = chdir(getenv("HOME"));
            if (err != 0) {
              printf("%s\n", "Error while changing directory.");
              fflush(stdout);
            }
          }
          // Else: if another token is present, use as path.
          else{
            err = chdir(token);
            if (err != 0) {
              printf("%s\n", "Error while changing directory.");
              fflush(stdout);
            }
          }
        }
        // Else: command is NOT built-in
        else{
          pid_t returnid = -2;
          returnid = fork();
          switch (returnid) {
            case -1:
              printf("Error - child not created");
              fflush(stdout);
              break;
            case 0:
              // Child
              char command_array[512][100]; // Holds command and args to pass to execvp()
              command_str[511] = NULL; // Make sure last pointer is NULL
              int ca_i = 0; // Holds current index in command_array
              int arg_flag; // 0 means arguments can be entered, 1 means they cannot. Changed once redirection is found.

              command_array[ca_i] = token; // First pointer is command string

              int fr;
              int fo;

              // Add arguments to command_array
              while ( (token = strtok(NULL, " ") ) != NULL) {
                // Input to command
                if (strcmp(token, "<")) {
                  if ((token = strtok(NULL, " ") ) != NULL) {
                    fr = open(token, O_RDONLY); // Open for read only
                    if (fr == -1) {
                      perror("Error: ");
                      exit(1);
                    }
                    fcntl(fr, F_SETFD, FD_CLOEXEC);
                    dup2(fr, 0);
                  }
                  arg_flag = 1;
                }
                // Output from command
                else if (strcmp(token, ">")) {
                  if ((token = strtok(NULL, " ") ) != NULL) {
                    fo = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open only
                    if (fo == -1) {
                      perror("Error: ");
                      exit(1);
                    }
                    fcntl(fo, F_SETFD, FD_CLOEXEC);
                    dup2(fo, 1);
                  }
                  arg_flag = 1;
                }
                // Token is an argument
                else if (arg_flag == 0){
                  command_array[ca_i] = token;
                  ca_i++;
                }
              }
              break;
            default:
              // Parent
              char* prev_tok; // Holds pointer to previous token - used with &
              while ((token = strtok(NULL, " ") ) != NULL) {
                prev_tok = token;
              }
              if (strcmp(prev_tok, "&") != 0) {
                wait(&status);
              }
              break;
          }
        }
      }
    }


  } while(1);


  // Exit
  exit(0);
}
