/* MAIN FILE */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "list.h"

#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>


#include <semaphore.h>


// Main Thread Functions
void *inputCreator(void* arguments);
void *sender(void* arguments);
void *receiver(void* arguments);
void *OutputPrint(void* arguments);

// Helper functions
char *getUserInput(char **bufp, size_t *sizep, FILE *fp);

/* THREAD LIST */
pthread_t tidKeyBoard;
pthread_t tidSender;
pthread_t tidReceiver;
pthread_t tidPrinter;

int sock_fd;
static List* outboundList;
static List* inboundList;

struct sockaddr_in myAddress;
struct sockaddr_in remoteAddress;

void encryptionKey();
int encryptDecrypt(char *message, int len, bool encrypt);
bool statusCheck = false;
bool exitOut = false;

char key;
static fd_set f_set;
static struct timeval timeout;

pthread_mutex_t lock;


int main( int argc, char *argv[] )
{
  outboundList = List_create();
  inboundList = List_create();
  encryptionKey();

  // Set timeout for !status
  timeout.tv_sec = 2;

  // Create Return values of each thread
  void **returnKeyBoard = NULL;
  void **returnSender = NULL;
  void **returnReceiver = NULL;
  void **returnPrinter = NULL;

  // Check to see if the right amount of arguments were passed in. If they weren't, output a message to the console.
  if (argc < 4)
  {
    printf("Usage: \n  %s <local port> <remote host> <remote port>\n", argv[0]);
    printf("Examples: \n  <3000> <192.168.0.513> <3001>\n");
    printf("  <3000> <some-computer-name> <3001>\n");
    return 1;
  }

  // socket creation begins here //

  // The set up of the sockets was based off of: https://stackoverflow.com/questions/14998261/2-way-udp-chat-application
  // ---------------------------------------------- //

  // store local and remote port numbers for easy access
  long local_port = strtoul(argv[1], NULL, 0);
  long remote_port = strtoul(argv[3], NULL, 0);

  // Check the first argument to ensure it's valid
  if (local_port < 1)
  {
    printf("Error - the local port must be greater than 0. \n");
    return 1;
  }

  // Check the second argument to ensure that it's valid
  if (inet_aton(argv[2], &remoteAddress.sin_addr) == 0)
  {
    printf("Error - invalid remote address '%s'\n", argv[2]);
    return 1;
  }

  // Check the third argument to ensure it's valid
  if (remote_port < 1)
  {
    printf("Error - the remote port must be greater than 0.\n");
    return 1;
  }

  /* Parse command line argument for remote host address */
  remoteAddress.sin_family = AF_INET;
  remoteAddress.sin_port = htons(remote_port);
  remoteAddress.sin_addr.s_addr = inet_addr(argv[2]);

  /* Parse command line argument for local host address */
  myAddress.sin_family = AF_INET;
  myAddress.sin_port = htons(local_port);
  myAddress.sin_addr.s_addr = inet_addr(argv[2]);

  /* Create UDP socket */
  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd < 0)
  {
    perror("Error - Failed to create the socket. \n");
    close(sock_fd);
    exit(0);
  }

  if (bind(sock_fd, (struct sockaddr *)(&myAddress),sizeof(myAddress)) < 0)
  {
    printf("Error - failed to bind the socket.\n");
    close(sock_fd);
    return 1;
  }

  // ----------------------------------------- //

  // Create all of the threads
  pthread_create(&tidKeyBoard, NULL, inputCreator, (void*) argv);
  pthread_create(&tidSender, NULL, sender, (void*) argv);
  pthread_create(&tidReceiver, NULL, receiver, (void*) argv);
  pthread_create(&tidPrinter, NULL, OutputPrint, (void*) argv);

  if (pthread_mutex_init(&lock, NULL) != 0)
  {
    printf("Error initializing mutex. \n");
    exit(0);
  }

  //Join all the threads
  pthread_join(tidKeyBoard, returnKeyBoard);
  pthread_join(tidSender, returnSender);
  pthread_join(tidReceiver, returnReceiver);
  pthread_join(tidPrinter, returnPrinter);

  pthread_mutex_destroy(&lock);

  return 0;
}



void *receiver(void *Arguments)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
  int t;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &t);  int receiveLength;

  socklen_t addrlen = sizeof(remoteAddress);
  while(true)
  {
    char userInput[4000];
    //memset(userInput, 0, 4000);

    fflush(stdout);

    if ((receiveLength = recv(sock_fd, userInput, sizeof(userInput), 0) == -1))
    {
      if (exitOut == true)
      {
        //printf("EXITING!!");
        exit(1);
      }
      else
      {
        perror("recvfrom() error. \n");
        exit(1);
      }
    }

    else if (strncmp(userInput, "!status", 7) == 0)
   {
      sendto(sock_fd, userInput, strlen(userInput), 0, (struct sockaddr*) &remoteAddress, addrlen);
      continue;
    }

    else if (strncmp(userInput, "!exit", 4) == 0)
   {
      printf("EXITING!!");
      pthread_cancel(tidKeyBoard);
      pthread_cancel(tidSender);
      pthread_cancel(tidPrinter);
      close(sock_fd);
      pthread_exit(0);
    }

    encryptDecrypt(userInput, sizeof(userInput), false);
    pthread_mutex_lock(&lock);
    List_append(inboundList, userInput);
    pthread_mutex_unlock(&lock);
  	}
  close(sock_fd);
}



void *OutputPrint(void* Arguments)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
  int t;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &t);  while(true)

  {
    if (List_first(inboundList) != NULL)
    {
      pthread_mutex_lock(&lock);
      char* incomingMessage = List_first(inboundList);
      printf("%s\n", incomingMessage);
      List_remove(inboundList);
      pthread_mutex_unlock(&lock);
    }
  }
}


void *inputCreator(void* Arguments)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
  int t;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &t);

  printf("Welcome to Lets-Talk! Please type your messages now. \n");

  // char userInput[4000];
  while(true)
  {
     char userInput[4000];
    //memset(userInput, 0, 4000);
    fgets(userInput, 4000, stdin);
    userInput[strlen(userInput) -1] = '\0';

    if ((userInput[0] == '!') && (userInput[1] == 'e') && (userInput[2] == 'x') && (userInput[3] == 'i') && (userInput[4] == 't') && ((userInput[5] == '\0') || (userInput[5] == ' ')))
    {
      exitOut = true;
      printf("Exiting. \n");
      //sendto(sock_fd, userInput, 5, 0, (struct sockaddr*) &remoteAddress, sizeof(struct sockaddr_in));
      pthread_cancel(tidReceiver);
      pthread_cancel(tidSender);
      pthread_cancel(tidPrinter);
      close(sock_fd);
      pthread_exit(0);
      //exit(1);
      //return NULL;
    }

    if ((userInput[0] == '!') && (userInput[1] == 's') && (userInput[2] == 't') && (userInput[3] == 'a') && (userInput[4] == 't') && (userInput[5] == 'u') && (userInput[6] == 's') && ((userInput[7] == '\0') || (userInput[7] == ' ')))
    {
      // This section of code was inspired from https://stackoverflow.com/questions/18681642/select-function-is-getting-timed-out-even-after-the-socket-has-data-to-read-i
      printf("Checking status \n");

      sendto(sock_fd, userInput, 7, 0, (struct sockaddr*) &remoteAddress, sizeof(struct sockaddr_in));
      FD_ZERO(&f_set);
      FD_SET(sock_fd, &f_set);

      // Check whether some data is in the socket
      if (select(sock_fd + 1, &f_set, NULL, NULL, &timeout) > 0)
      {
        printf("Online.\n");
      }

      else
      {
        printf("Offline.\n");
      }

      continue;
      }

    encryptDecrypt(userInput, sizeof(userInput), true);

    pthread_mutex_lock(&lock);
    List_append(outboundList, userInput);
    pthread_mutex_unlock(&lock);
  }
  return 0;
}

int encryptDecrypt(char *message, int len, bool encrypt)
{
  int i = 0;
  if (encrypt == true)
  {
    while (i < len)
    {
      message[i] =  message[i] + key % 256;
      i++;
    }
  }
  else if (encrypt == false)
  {
    while (i < len)
    {
      message[i] =  message[i] - key % 256;
      i++;
    }
  }
  return 0;
}

void encryptionKey()
{
  key = rand() % 25 + 5;
}


void *sender(void* Arguments)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
  int t;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &t);

  int bytes;
  while(true)
  {
    if (List_first(outboundList) != NULL)
    {
      bytes = sendto(sock_fd, List_first(outboundList), strlen(List_first(outboundList)), 0, (struct sockaddr *) &remoteAddress, sizeof(struct sockaddr_in));
      if (bytes < 0)
      {
        printf("Error - sendto error.\n");
      }
      pthread_mutex_lock(&lock);
      List_remove(outboundList);
      pthread_mutex_unlock(&lock);
    }
  }
}
