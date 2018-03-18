#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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
  char codedText[70002];

  if (argc != 4) {
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]);
    exit(0);
  } // Check arguments - should be four: name, plaintext, key, and port.

  // Set up the server address struct
  memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  //serverHostInfo = "localhost"; // Convert the machine name into a special form of address
  /*
  if (serverHostInfo == NULL) {
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(0);
  }
  */
  //memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

  // Set up the socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (socketFD < 0) error("CLIENT: ERROR opening socket");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
    error("CLIENT: ERROR connecting");


  char idbuff[1] = "1";
  // Check if server is correct - 1 == enc, 2 == dec - should be 1 for this file.
  charsWritten = send(socketFD, idbuff, strlen(idbuff), 0);
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");

  charsRead = recv(socketFD, idbuff, 1, 0);
  if (charsRead < 0){
    error("ERROR reading from socket");
  }
  else if (idbuff[0] != "1") {
    perror("Error - not encoding server.");
    exit(2);
  }


  // Send key to server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

  // Get completion message back.
  

  // Send plaintext to server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

  // Get return message from server
  memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again
  for reusecharsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
  if (charsRead < 0) error("CLIENT: ERROR reading from socket");
  printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
  close(socketFD); // Close the socket

  return 0;
}
