#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

void error(const char *msg) {
  perror(msg);
  exit(0);
} // Error function used for reporting issues

int main(int argc, char *argv[])
{
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  char buffer[256];
  char returnString[70002];

  if (argc != 4) {
    fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  } // Check arguments - should be four: name, plaintext, key, and port.

  // read key file and input into array
  char key[70002] = "";
  int fd = open(argv[2], O_RDONLY);
  if ( read(fd, key, 70001) < 0) {
    error("Error - failed open:");
  }
  close(fd);
  // remove newline
  char *charP = strchr(key, '\n');
  if (charP != NULL) {
    charP[0] = '@';
  }

  // read plaintext file and input into array.
  char plaintext[70002] = "";
  fd = open(argv[1], O_RDONLY);
  if ( read(fd, plaintext, 70001) < 0) {
    error("Error - failed open:");
  }
  close(fd);
  // Remove newline
  charP = strchr(plaintext, '\n');
  if (charP != NULL) {
    charP[0] = '@';
  }

  // Check key and plaintext for bad chars and correct size.
  if (strlen(key) < strlen(plaintext)) {
    error("Error - key is shorter than plaintext:");
  }
  // Check key
  int j;
  int arraylen = strlen(key);
  for (j = 0; j < arraylen; j++) {
    if (isupper(key[j]) == 0) {
      if (key[j] != ' ' && key[j] != '@') {
        error("Error - Bad character in key:");
      } // is char a space?
    } // is char a uppercase letter?
  }
  // Check plaintext
  arraylen = strlen(plaintext);
  for (j = 0; j < arraylen; j++) {
    if (isupper(plaintext[j]) == 0) {
      if (plaintext[j] != ' ' && plaintext[j] != '@') {
        error("Error - Bad character in plaintext:");
      } // is char a space?
    } // is char a uppercase letter?
  }

  // Set up the server address struct
  memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Set up the socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (socketFD < 0) error("CLIENT: ERROR opening socket");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
    error("CLIENT: ERROR connecting");


  char idbuff[] = "1";
  // Check if server is correct - 1 == enc, 2 == dec - should be 1 for this file.
  charsWritten = send(socketFD, idbuff, strlen(idbuff), 0);
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");

  charsRead = recv(socketFD, idbuff, 1, 0);
  if (charsRead < 0){
    error("ERROR reading from socket");
  }
  else if (idbuff[0] != '1') {
    perror("Error - not encoding server.");
    exit(2);
  }


  // Send key to server
  char *ptr = (char*) key;
  int length = strlen(key);
  while (length > 0)
  {
      int i = send(socketFD, ptr, length, 0);
      if (i < 0) error("CLIENT: ERROR writing to socket");
      ptr += i;
      length -= i;
  }

  // Get completion message back.
  charsRead = recv(socketFD, idbuff, 1, 0);
  if (charsRead < 0){
    error("ERROR reading from socket");
  }
  else if (idbuff[0] != '1') {
    error("Error - message not complete.");
  }

  // Send plaintext to server
  ptr = (char*) plaintext;
  length = strlen(plaintext);
  while (length > 0)
  {
      int i = send(socketFD, ptr, length, 0);
      if (i < 0) error("CLIENT: ERROR writing to socket");
      ptr += i;
      length -= i;
  }

  // Get return message from server
  memset(returnString, '\0', sizeof(returnString)); // Clear out the buffer again

  do {
    memset(buffer, '\0', 256);
    charsRead = recv(socketFD, buffer, 255, 0); // Read the client's message from the socket
    if (charsRead < 0) error("ERROR reading from socket");
    //printf("SERVER: I received this from the client: \"%s\"\n", buffer);
    strcat(returnString, buffer);

  } while(strstr(buffer, "\n") == NULL);

  printf("%s", returnString);
  close(socketFD); // Close the socket

  exit(0);
}
