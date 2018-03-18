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
  int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
  socklen_t sizeOfClientInfo;
  char buffer[256];
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
      else if (idbuff[0] != "1") {
        strcpy(idbuff, "0");
        send(establishedConnectionFD, idbuff, strlen(idbuff), 0);
        error("Error - not encoding client.")
      }
      else{
        strcpy(idbuff, "1");
        send(establishedConnectionFD, idbuff, strlen(idbuff), 0);
      }


      // Create full string
      char returnString[70000] = "";

      //printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));
      // Get the message from the client and display it
      do {
        memset(buffer, '\0', 256);
        charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
        if (charsRead < 0) error("ERROR reading from socket");
        printf("SERVER: I received this from the client: \"%s\"\n", buffer);
        // Send a Success message back to the client
        //charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
        //if (charsRead < 0) error("ERROR writing to socket");

      } while(strstr(buffer, "@") == NULL);

      close(establishedConnectionFD); // Close the existing socket which is connected to the client

    } // Child case
    else{

    } // Parent case

  }

  close(listenSocketFD); // Close the listening socket
  return 0;
}
