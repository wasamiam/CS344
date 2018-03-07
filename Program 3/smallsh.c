#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void replace_pid (char line[], pid_t pid);
void SIGTSTPhandler(int sig);

// Global variables
int force_fg;

int main(int argc, char **argv){
  // Variables
  char line[3000] = "";
  char* token;
  char status[200];
  pid_t pid_array[100]; // static arrays are initialized as 0
  int n_pid = 0; // Number of process ids in the pid_array - used with exit command to minimize array search time.
  int err = 0; // Hold error values from functions
  int i = 0; // hold number for loops
  char arg_list[512][100];
  char* command_array[512]; // Holds command and args to pass to execvp()
  char* prev_tok; // Holds pointer to previous token - used with &
  int ca_i = 0; // Holds current index in command_array
  int arg_flag = 0; // 0 means arguments can be entered, 1 means they cannot. Changed once redirection is found.
  int bg_flag = 0; // 0 means foreground, 1 means background.
  pid_t pid = getpid();

  force_fg = 0; // 0 means background processes allowed, 1 means processes only run in foreground.



  /*
  *  SIGNAL HANDLING
  */
  sigset_t mask;
  sigfillset(&mask);

  struct sigaction SIGTSTP_action = {
    SIGTSTPhandler,
    mask,
    SA_RESTART,
    NULL
  };
  signal(SIGINT, SIG_IGN);

  if (sigaction(SIGTSTP, &SIGTSTP_action, NULL) == -1) {
    perror("Error");
  }

  /*
  * Commandline loop
  */
  do {
    // Check for completed background tasks
    if (n_pid > 0) {
      for (i = 0; i < 100; i++) {
        if (n_pid == 0) {
          break;
        }
        // Check if valid id
        // Wait for child
        if(pid_array[i] != 0){
          pid_t t;
          int s;
          if ( (t = waitpid(pid_array[i], &s, WNOHANG) ) == -1) {
            perror("Error");
          }
          else if(t > 0){
            // Set the status
            if (WIFSIGNALED(s)) {
              sprintf(status, "terminated by signal %d", WTERMSIG(s));
              fflush(stdout);
            }
            else if (WIFEXITED(s)) {
              sprintf(status, "exit value %d", WEXITSTATUS(s));
              fflush(stdout);
            }
            else {
              printf("something weird happened.\n");
              fflush(stdout);
            }
            // Print, remove pid from array, and deduct 1 from the array's counter.
            printf("background pid %d is done: %s\n", t, status);
            fflush(stdout);
            pid_array[i] = 0; // Set pid to 0
            n_pid--;
          }
        }
      }

    }
    // Print prompt
    printf(": ");
    fflush(stdout);

    strcpy(line, ""); // Clear line
    // Read line
    // Check for error
    if (fgets(line, 2048, stdin) == NULL) {
      printf("%s\n", "Failed to get line.");
      fflush(stdout);
    }
    else{
      // reset flags
      bg_flag = 0;
      arg_flag = 0;

      // Remove newline from input string.
      int l = strlen (line);
      if (l > 0 && line[l - 1] == '\n'){line [l - 1] = '\0';}

      // Check for & and remove.
      l = strlen (line);
      if (l > 0 && line[l - 1] == '&'){
        line [l - 1] = '\0';
        line [l - 2] = '\0';
        // If forced foreground is off, allow processes to run in the background.
        if (force_fg == 0) {
          bg_flag = 1;
        }
      }

      replace_pid(line, pid);// Replace all $$ with process id

      if(line[0] == '#'){} // Skip if comment
      // Else: NOT a comment
      else{

        // Get first token, which should be the command.
        token = strtok(line, " ");

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
          printf("%s\n",status);
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
        /*
        ** Else: command is NOT built-in
        */
        else{
          pid_t returnid = -2;
          returnid = fork();
          switch (returnid) {
            case -1:
              printf("Error - child not created");
              fflush(stdout);
              break;

            /*
            **  Child
            */
            case 0:
              signal (SIGINT, SIG_DFL);
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
                      printf("cannot open %s for input\n", token);
                      fflush(stdout);
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
                    fo = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open for write only
                    if (fo == -1) {
                      printf("cannot open %s for output\n", token);
                      fflush(stdout);
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

              if (arg_flag == 0 && bg_flag == 1 ) {
                fr = open("/dev/null", O_RDONLY); // Open for read only
                fo = open("/dev/null", O_WRONLY); // Open for write only
                fcntl(fr, F_SETFD, FD_CLOEXEC);
                fcntl(fo, F_SETFD, FD_CLOEXEC);
                dup2(fr, 0);
                dup2(fo, 1);
              }
              command_array[ca_i] = NULL; // Make sure last pointer is NULL

              err = execvp(command_array[0], command_array);
              if (err == -1) {
                perror("badfile");
                exit(1);
              }
              break;

            /*
            **  Parent
            */
            default:
              // Check for background.
              if (bg_flag == 0) {
                int tmp = 0;
                pid_t t;
                if ( waitpid(returnid, &tmp, 0) == -1) {
                  perror("Error");
                }
                else{
                  if (WIFSIGNALED(tmp)) {
                    sprintf(status, "terminated by signal %d", WTERMSIG(tmp));
                    fflush(stdout);
                    printf("%s\n", status);
                    fflush(stdout);
                  }
                  else if (WIFEXITED(tmp)) {
                    sprintf(status, "exit value %d", WEXITSTATUS(tmp));
                    fflush(stdout);
                  }
                  else {
                    printf("something weird happened.\n");
                    fflush(stdout);
                  }
                }
              }
              // Else: Background case - add child id to an empty spot in pid array and then break.
              else{
                for (i = 0; i < 100; i++) {
                  if (pid_array[i] == 0) {
                    pid_array[i] = returnid;
                    n_pid++;
                    printf("background pid is %d\n", pid_array[i]);
                    fflush(stdout);
                    break;
                  }
                }
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

/*
* Function to replace $$ with process id.
*/
void replace_pid (char line[3000], pid_t pid){
  char tmp_line[3000] = "";
  char* tmp_tok;
  char tmp2[100] = "";
  char id[10];
  sprintf(id, "%d",(int)pid);
  fflush(stdout);

  // Check if $$ is at end.
  int l = strlen (line);
  if (line[l-1] == '$' && line[l-2] == '$') {
    // If so, get the string before $$, copy it back to the main string, and return.
    tmp_tok = strtok(line, "$$");
    strcat(tmp_line, tmp_tok);
    strcat(tmp_line, id);
    strcpy(line, tmp_line);
    return;
  }

  // This case happens if $$ is not at the end, or occurs multiple times.
  tmp_tok = strtok(line, "$$");
  strcat(tmp_line, tmp_tok);

  while ( (tmp_tok = strtok(NULL, "$$")) != NULL) {
    strcat(tmp_line, id);
    strcat(tmp_line, tmp_tok);
  }
  strcpy(line, tmp_line);
  return;
}

/*
* Funtion to handle SIGTSTP signal
*/
void SIGTSTPhandler(int sig){

  // If forced foreground mode is OFF, turn ON
  if (force_fg == 0) {
    char* message = "Entering foreground-only mode (& is now ignored)\n";
    write(STDOUT_FILENO, message, 49);
    force_fg = 1;
  }
  // If forced foreground mode is ON, turn OFF
  else{
    char* message = "Exiting foreground-only mode\n";
    write(STDOUT_FILENO, message, 29);
    force_fg = 0;
  }
}
