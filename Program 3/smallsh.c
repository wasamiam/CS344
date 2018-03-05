#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char **argv){
  // Variables
  char line[2048] = "";
  char* token;
  int status = 0;
  int pid_array[100]; // static ints are initialized as 0
  int n_pid = 0; // Number of process ids in the pid_array - used with exit command to minimize array search time.
  // Commandline loop
  do {
    // Check for completed background tasks

    // Print prompt
    printf(": ");
    fflush();
    // Read line
    int er = scanf("%s", line);
    // Check for error
    if (er < 0) {
      printf("%s\n", "Failed to get line.");
    }
    else{
      token = strtok(line, ' '); // Get first token, which should be the command.
      if(token == "#"){} // Skip if comment
      // Else: NOT a comment
      else{
        // Command is 'exit'
        if (strcmp(token, "exit") == 0) {
          int i;
          for (i = 0; i < 100; i++) {
            // Exit once all children have been handled
            if (n_pid == 0) {
              exit(0);
            }
            // Check if valid id
            if(pid_array[i] == 0){}
            // Kill child process
            else{
              int err = kill(pid_array[i], SIGTERM);
              if (err < 0) {
                printf("%s\n", "Error killing child process.");
              }
              pid_array[i] = 0; // Set pid to 0 once killed
              n_pid--;
            }
          }
        }
        // Command is 'status'
        else if (strcmp(token, "status") == 0) {
          /* code */
        }
        // Command is 'cd'
        else if (strcmp(token, "cd") == 0) {
          /* code */
        }
        // Else: command is NOT built-in
        else{

        }

      }
    }


  } while();


  // Exit
  exit(0);
}
