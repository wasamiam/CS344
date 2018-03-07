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
void SIGINThandler(int sig);
void SIGTSTPhandler(int sig);

int main(int argc, char **argv){
  // Variables
  char line[3000] = "";
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
  int bg_flag = 0; // 0 means foreground, 1 means background.
  pid_t pid = getpid();


  /*
  *  SIGNAL HANDLING
  */
  sigset_t mask;
  sigfillset(&mask);

  struct sigaction SIGINT_action = {
    SIGINThandler,
    mask,
    0,
    NULL
  }, SIGTSTP_action = {
    SIGTSTPhandler,
    mask,
    0,
    NULL
  };
  signal(SIGINT, SIG_IGN);
  /*
  if (sigaction(SIGINT, &SIGINT_action, NULL) == -1) {
    perror("Error: ");
  }
  */
  if (sigaction(SIGTSTP, &SIGTSTP_action, NULL) == -1) {
    perror("Error: ");
  }

  /*
  SIGINT_action.sa_handler = catchSIGINT;
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;
  SIGTSTP_action.sa_handler = catchSIGUSR2;
  sigfillset(&SIGUSR2_action.sa_mask);
  SIGTSTP_action.sa_flags = 0;
  */
  // Commandline loop
  do {
    // Check for completed background tasks
    if (n_pid > 0) {
      for (i = 0; i < 100; i++) {
        if (n_pid == 0) {
          break;
        }
        // Check if valid id
        if(pid_array[i] == 0){
        }
        // Wait for child
        else{
          pid_t t;
          t = waitpid(pid_array[i], &status, WNOHANG);
          if (t != 0) {
            printf("Process %d complete\n", t);
            fflush(stdout);
            pid_array[i] = 0; // Set pid to 0
            n_pid--;
          }
          else{
            printf("Process %d incomplete - pi: %d\n", t, pid_array[i]);
            fflush(stdout);
          }
          /*
          if ( t > -1) {
            printf("Process %d complete\n", t);
            fflush(stdout);
            pid_array[i] = 0; // Set pid to 0
            n_pid--;
          }
          */
        }
      }

    }
    // Print prompt
    //fflush(stdout);
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

      int l = strlen (line);
      if (l > 0 && line[l - 1] == '\n'){line [l - 1] = '\0';} // Remove newline
      l = strlen (line);
      if (l > 0 && line[l - 1] == '&'){
        line [l - 1] = '\0';
        line [l - 2] = '\0';
        bg_flag = 1;
      } // Check for & and remove.

      replace_pid(line, pid);// Replace all $$ with process id
      //printf("line2:  %s\n", line);
      //fflush(stdout);

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
          printf("exit value %d\n", status);
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
                      perror("Error");
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
                      perror("Error");
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
                perror("Error");
                exit(1);
              }
              break;
            default:
              // Parent
              if (bg_flag == 0) {
                int tmp = 0;
                if (wait(&tmp) == -1) {
                  perror("Error");
                }
                else{
                  if (WIFSIGNALED(tmp)) {
                    sscanf(status, "terminated by signal %d", WTERMSIG(tmp));
                    printf("%s\n", status);
                    fflush(stdout);
                  }
                  else if (WIFEXITED(tmp)) {
                    sscanf(status, "exit value %d", WEXITSTATUS(tmp));
                    printf("%s\n", status);
                    fflush(stdout);
                  }
                  else {
                    printf("something weird happened.\n");
                    fflush(stdout);
                  }
                }

              }
              // Else: add child id to array and move on. Background case.
              else{
                for (i = 0; i < 100; i++) {
                  if (pid_array[i] == 0) {
                    pid_array[i] = returnid;
                    n_pid++;
                    printf("Child id: %d\n", pid_array[i]);
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
    tmp_tok = strtok(line, "$$");
    strcat(tmp_line, tmp_tok);
    strcat(tmp_line, id);
    strcpy(line, tmp_line);
    return;
  }

  tmp_tok = strtok(line, "$$");
  strcat(tmp_line, tmp_tok);

  while ( (tmp_tok = strtok(NULL, "$$")) != NULL) {
    strcat(tmp_line, id);
    strcat(tmp_line, tmp_tok);
  }
  strcpy(line, tmp_line);
  return;
}

void SIGINThandler(int sig){}

void SIGTSTPhandler(int sig){

}
