#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) {
  perror(msg);
  exit(1);
} // Error function used for reporting issues

int main(int argc, char *argv[])
{
  int listenSocketFD, establishedConnectionFD, portNumber, charsWritten, charsRead;
  socklen_t sizeOfClientInfo;
  char buffer[256];
  char plaintext[70002] = "";
  char key[70002] = "";
  char returnString[70002] = "";

  struct sockaddr_in serverAddress, clientAddress;
  if (argc < 2) {
    fprintf(stderr,"USAGE: %s port\n", argv[0]);
    exit(1);
  } // Check usage & arguments

  // Set up the address struct for this process (the server)
  memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

  // Set up the socket
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (listenSocketFD < 0) error("ERROR opening socket");

  // Enable the socket to begin listening
  if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
  error("ERROR on binding");
  listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

  while (1) {
    // Accept a connection, blocking if one is not available until one connects
    sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr*)&clientAddress, &sizeOfClientInfo); // Accept
    if (establishedConnectionFD < 0) error("ERROR on accept");

    int pid = fork();
    if (pid == -1) {
      perror("Error");
    } // Error case
    else if (pid == 0) {

      char idbuff[1] = "";
      // Check if client is correct - 1 == enc, 2 == dec - should be 1 for this file.
      charsRead = recv(establishedConnectionFD, idbuff, 1, 0);
      if (charsRead < 0){
        error("ERROR reading from socket");
      }
      else if (idbuff[0] != '1') {
        strcpy(idbuff, "0");
        charsWritten = send(establishedConnectionFD, idbuff, strlen(idbuff), 0);
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
        error("Error - not encoding client.");
      }
      else{
        strcpy(idbuff, "1");
        send(establishedConnectionFD, idbuff, strlen(idbuff), 0);
      }


      // Create full string
      char returnString[70002] = "";

      //printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));

      // Get key from client
      do {
        memset(buffer, '\0', 256);
        charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
        if (charsRead < 0) error("ERROR reading from socket");
        //printf("SERVER: I received this from the client: \"%s\"\n", buffer);
        strcat(key, buffer);
        // Send a Success message back to the client
        //charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
        //if (charsRead < 0) error("ERROR writing to socket");

      } while(strstr(buffer, "@") == NULL);

      //printf("%s", key);

      // Tell client to send plaintext
      strcpy(idbuff, "1");
      charsWritten = send(establishedConnectionFD, idbuff, strlen(idbuff), 0);
      if (charsWritten < 0) error("CLIENT: ERROR writing to socket");

      // Get plaintext from client
      do {
        memset(buffer, '\0', 256);
        charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
        if (charsRead < 0) error("ERROR reading from socket");
        //printf("SERVER: I received this from the client: \"%s\"\n", buffer);
        strcat(plaintext, buffer);
        // Send a Success message back to the client
        //charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
        //if (charsRead < 0) error("ERROR writing to socket");

      } while(strstr(buffer, "@") == NULL);

      //printf("SERVER: Plaintext: \"%s\"\n", plaintext);

      strcpy(returnString, key);
      printf("Server: %s\n", returnString);
      // Send final text to client
      char *ptr = (char*) returnString;
      int length = strlen(returnString);
      while (length > 0)
      {
          int i = send(establishedConnectionFD, ptr, length, 0);
          if (i < 0) error("CLIENT: ERROR writing to socket");
          ptr += i;
          length -= i;
      }

      close(establishedConnectionFD); // Close the existing socket which is connected to the client
      exit(0);
    } // Child case
    else{

    } // Parent case

  }

  close(listenSocketFD); // Close the listening socket
  return 0;
}
