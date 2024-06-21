// client.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>

#define SERVER_FIFO "server_fifo"
#define MAX_BUF_SIZE 1024
#define STRING_SIZE 100
char clientFifoName[MAX_BUF_SIZE];

//Declare read and write file descriptors
static int readFD, writeFD;
static pid_t processId;

//Defining functions to error handle system call parameters
//and send the requests to SERVER_FIFO
void system_call_1();
void sendmsg_request_buffer(int writeFD, pid_t pid, int syscallNumber, int msg_type, int msg_length, char* msg);
void receivemsg_request_buffer(int writeFD, pid_t pid, int syscallNumber, int msg_type);
void create_request_buffer(int writeFD, pid_t pid, int syscallNumber,char* paramBuffer);

//HW2 Addition: simulated process ID
static int simPID;

int main() {
    processId = getpid();

    // Initialize write fd to only write on SERVER_FIFO 
    writeFD = open(SERVER_FIFO, O_WRONLY);
    if (writeFD == -1) {
        perror("Error opening SERVER_FIFO");
        exit(EXIT_FAILURE);
    }

    //Array to store name of client FIFO
    sprintf(clientFifoName, "client_fifo_%d", getpid());

    //Creating client FIFO
    mkfifo(clientFifoName, 0666);

    //Perform connect system call to the server
    printf("\nPeforming connect system call to server...\n");
    system_call_1();

    //Client reads input from user
    int user_input;
    //Prompt user for choice
    printf("\n1)Send request to server\n2)EXIT\n3)SHUTDOWN\nEnter choice: ");
    scanf("%d", &user_input);
    printf("\n\n");

    int syscallNumber;
    while(user_input == 1){

            //HW2 Addition: Add simPID to system call request
            printf("Simulated PID: %d\n", simPID);

            // Read input from user to form the system call request
            printf("Process ID: %d\n", processId);
            
            //HW3 Revision: Made a clearer menu to show options
            printf("System Calls:\n2 - Send Message\n3 - Receive Message\nEnter System Call Number: ");
            scanf("%d", &syscallNumber);

            // Handle rest of request params by calling
            // particular function based on syscallNumber
            switch(syscallNumber) {
                int msg_type;
                case 2:
                    //HW3 Revision: Sending message request
                    //Request must include pid, syscallnum, and msg being sent

                    //Creating msg format (type, length, data)
                    printf("Enter message type (integer): ");
                    scanf("%d", &msg_type);

                    printf("Message length: %d\n", MAX_BUF_SIZE);

                    char msg[MAX_BUF_SIZE];
                    printf("\nEnter message to send to the server:\n");
                    scanf(" %[^\n]", msg);

                    sendmsg_request_buffer(writeFD, simPID, syscallNumber, msg_type, MAX_BUF_SIZE, msg);

                    //Receive acknoledgment reply
                    //Is it acknowledgment that the server received the request?
                    //Or acknowledgement that the msg has been delivered to another client
                    break;
                case 3:
                    //HW3 Revision: Receiving message request
                    //Request must include pid, syscallNum, and msg type

                    printf("Enter message type (integer): ");
                    scanf("%d", &msg_type);

                    receivemsg_request_buffer(writeFD, simPID, syscallNumber, msg_type);
                    printf("\nWaiting to receive message...\n");

                    //Receiving message
                    char received_msg[MAX_BUF_SIZE];
                    read(readFD, received_msg, sizeof(received_msg));
                    printf("\nMessage received:\n%s\n", received_msg);
                    break;
            }

        for (int i = 0; i < 50; i++){
                printf("#");
            }
            printf("\n");

        //Prompt user for choice
        printf("\n1)Send request to server\n2)EXIT\n3)SHUTDOWN\nEnter choice: ");
        scanf("%d", &user_input);
        printf("\n\n");

    };

    //EXIT OR SHUTDOWN CALL
    if(user_input == 2){
        syscallNumber = 0;
    }
    else if(user_input == 3) {
        syscallNumber = -1;
    }


    //HW3 Revision: Send request fields to server
    char request[MAX_BUF_SIZE];
    sprintf(request, "%d %d", syscallNumber, simPID);
    write(writeFD, request, sizeof(request));

    printf("Request sent!\n\n");

    //Clean up
    close(writeFD);
    close(readFD);
    unlink(clientFifoName);
    return 0;

}

void system_call_1(){
    //HW3 Revision: Got rid of size of param and num of param for the request sending

    int syscallNumber = 1;
    printf("Process ID: %d\nSystem Call: %d\n", processId, syscallNumber);

    printf("Parameter (Client FIFO name): %s\n", clientFifoName);

    //Send all request fields to server (writing to SERVER_FIFO)
    create_request_buffer(writeFD, processId, syscallNumber, clientFifoName);

    printf("Request sent!\n");

    // Initialize read fd to only read from client FIFO 
    readFD = open(clientFifoName, O_RDONLY);
    if (readFD == -1) {
        perror("Error opening server FIFO");
        exit(EXIT_FAILURE);
    }

    //HW2 Addition: Received simPID from server
    read(readFD, &simPID, sizeof(simPID));

    printf("Simulated PID: %d\n\n", simPID);

}

void create_request_buffer(int writeFD, pid_t pid, int syscallNumber, char* buffer){

    char request[MAX_BUF_SIZE];
    sprintf(request, "%d %d %s", syscallNumber, simPID, buffer);

    write(writeFD, request, sizeof(request));
}


//HW3 Revision: Edited request to include msg_type, msg_length, and msg
void sendmsg_request_buffer(int writeFD, pid_t pid, int syscallNumber, int msg_type, int msg_length, char* msg){

    char request[MAX_BUF_SIZE];
    sprintf(request, "%d %d %d %d %s", syscallNumber, simPID, msg_type, msg_length, msg);

    write(writeFD, request, sizeof(request));
}

//HW3 Addition: Created request to include msg_type 
void receivemsg_request_buffer(int writeFD, pid_t pid, int syscallNumber, int msg_type){

    char request[MAX_BUF_SIZE];
    sprintf(request, "%d %d %d", syscallNumber, simPID, msg_type);

    write(writeFD, request, sizeof(request));
}

//HW3 Revision: Got rid of create_request_value function







