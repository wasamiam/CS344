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
  char arg_list[512][100];
  char* command_array[512]; // Holds command and args to pass to execvp()
  char* prev_tok; // Holds pointer to previous token - used with &
  int ca_i = 0; // Holds current index in command_array
  int arg_flag = 0; // 0 means arguments can be entered, 1 means they cannot. Changed once redirection is found.

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
      int l = strlen (line); // Remove newline
      if (l > 0 && line[l - 1] == '\n'){line [l - 1] = '\0';}

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
              strcpy(arg_list[0], token);
              command_array[0] = arg_list[0]; // First pointer is command string
              ca_i = 1;
              int fr;
              int fo;

              // Add arguments to command_array
              while ( (token = strtok(NULL, " ") ) != NULL) {
                // Input to command
                if (strcmp(token, "<") == 0) {
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
                else if (strcmp(token, ">") == 0) {
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
                if (arg_flag == 0){
                  strcpy(arg_list[ca_i], token);
                  command_array[ca_i] = arg_list[ca_i];
                  ca_i++;
                }
              }

              command_array[ca_i] = NULL; // Make sure last pointer is NULL
              printf("%s", command_array[0]);
              fflush(stdout);
              printf("%s", arg_list[1]);
              fflush(stdout);
              printf("%s", command_array[2]);
              fflush(stdout);
              err = execvp(command_array[0], command_array);
              if (err == -1) {
                perror("Error: ");
                exit(1);
              }
              break;
            default:
              // Parent
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
