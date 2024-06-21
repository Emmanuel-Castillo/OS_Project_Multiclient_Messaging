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
static char clientFifoName[MAX_BUF_SIZE];

//Delare file descriptors 
static int writeFD, readFD;

//HW2 Addition: declare processID counter & simPID from client
static int pidCounter;

//HW2 Addition: use link list to store fds for each client
struct ClientNode {
    int simPID;
    int writeFD;
    char clientFifoName[MAX_BUF_SIZE];
    struct ClientNode* next;
};

//HW3 Addition: use link list to create queue of msgs
struct Message {
    int msg_type;
    int msg_length;
    char msg[MAX_BUF_SIZE];
    struct Message* next;
};

//HW3 Addition: use link list to create queue of waiting clients with just pid and msg_type
struct waitingClient {
    int simPID;
    int writeFD;
    int msg_type;
    struct waitingClient* next;
};

// Function declaration
void exit_client(int simPID, struct ClientNode** head);
void searchNode(int simPID, struct ClientNode* head);
struct Message* searchMessageType(int msg_type, struct Message* head);
struct waitingClient* searchWaitingClient(int msg_type, struct waitingClient* head);
void removeMsg(int msg_type, struct Message** head);
void removeWaitingClient(int msg_type, struct waitingClient** head);
void printLinkedList(struct ClientNode* head);


int main() {

    //HW2 Addition: Initialize pidCounter to 1
    pidCounter = 1;

    //HW2 Addition: Declare head node for ClientNode linkedlist
    struct ClientNode* head = NULL;

    //HW3 Addition: Declare head node for Message linkedlist
    struct Message* msg_head = NULL;

    //HW3 Addition: Declare head node for waitingClient linkedlist
    struct waitingClient* waitingClient_head = NULL;

    //Make SERVER_FIFO
    mkfifo(SERVER_FIFO, 0666);

    //Open SERVER_FIFO for reading, assigning to server read file descriptor 
    readFD = open(SERVER_FIFO, O_RDONLY);
    if(readFD == -1) {
        perror("Error opening server FIFO");
        exit(EXIT_FAILURE);
    }

    //Reading client request
    while(1){

        //HW2 Utility: Print linked list
        //printLinkedList(head);

        //Reading client request written in SERVER_FIFO
        char request[MAX_BUF_SIZE];
        read(readFD, request, sizeof(request));

        //Tokenize the syscall
        //HW3 Revision: no longer tokenizing num of param and size of param
        int syscallNumber = atoi(strtok(request, " "));
        int simPID = atoi(strtok(NULL, " "));

        //HW2 Addition: search for node by simPID and assign writeFD of server to writeFD of found ClientNode
        searchNode(simPID, head);

        //Interpret request based from syscallNumber
        switch(syscallNumber){
            char* token;

            case 1:  
                token = strtok(NULL, " ");
                strncpy(clientFifoName, token, MAX_BUF_SIZE - 1);
                
                writeFD = open(clientFifoName, O_WRONLY);

                printf("\nReceived client FIFO name: %s\n", clientFifoName);

                //HW2 Addition: Declare new node that contains simPID and writeFD for specific client 
                //and add it to the list
                struct ClientNode* newNode = (struct ClientNode*)malloc(sizeof(struct ClientNode));
                newNode->simPID = pidCounter;
                newNode->writeFD = writeFD;
                strcpy(newNode->clientFifoName, clientFifoName);
                newNode->next = NULL;

                if(head == NULL) {
                    head = newNode;
                }
                else{
                    struct ClientNode* curr = head;
                    while(curr->next != NULL){
                        curr = curr->next;
                    }
                    curr->next = newNode;
                }

                //HW2 Addition: Increment pidCounter and numActiveClients and return to client
                write(writeFD, &pidCounter, sizeof(pidCounter));
                pidCounter++;
                break;

            case 2:
                //HW3 Revision: This will be the message send system call
                //Call includes pid, syscallnum, and message
                //take the msg, put it in queue of msgs
                //if another client is waiting for a msg of this type,
                //send reply to said client with msg
                //and also send acknowledgment reply to sending client
                
                //HW3 Addition: creating new Message node
                struct Message* newMessage = (struct Message*)malloc(sizeof(struct Message));
                newMessage->msg_type = atoi(strtok(NULL," "));
                newMessage->msg_length = atoi(strtok(NULL," "));
                token = strtok(NULL,"\0");
                strcpy(newMessage->msg, token);

                //Check if waiting clients is waiting for this particular msg_type
                struct waitingClient* client = (struct waitingClient*)malloc(sizeof(struct waitingClient));
                client = searchWaitingClient(newMessage->msg_type, waitingClient_head);

                //If client has been waiting for that particular msg (known by matching msg_type values),
                //return msg back to said client and delete its node from waiting clients queue
                if(client != NULL) {

                    write(client->writeFD, newMessage->msg, newMessage->msg_length);

                    removeWaitingClient(client->msg_type, &waitingClient_head);

                }
                //Otherwise, add the new Message node to Message queue
                else {
                    printf("\nNo clients waiting for this message type. Adding to the message queue.\n");
                    if(msg_head == NULL) {
                    msg_head = newMessage;
                    }
                    else{
                        struct Message* curr = msg_head;
                        while(curr->next != NULL){
                            curr = curr->next;
                        }
                        curr->next = newMessage;
                    }
                }
                printf("\n\nRequest processed!\nSystem Call: %d\nSimulated ProcessID: %d\nMessage Type: %d\nMessage Length: %d\nMessage: %s\n\n", syscallNumber, simPID, newMessage->msg_type, newMessage->msg_length, newMessage->msg);

                break;

            case 3:
                //HW3 Revision: This will be the message receive system call
                //Call includes pid, syscallnum, and type msg requested
                //if no msg of that type is found in queue, no reply is sent (block the client), and pid should be put in a queue of processes waiting for a msg (including type of msg being waited on)
                //if 1 or more msgs of requested type is available, oldest msg should be removed from queue and returned to client

                //Grab msg_type, this is to find any potential clients waiting in the queue for the particular value
                int msg_type;
                msg_type = atoi(strtok(NULL," "));

                struct Message* msg = (struct Message*)malloc(sizeof(struct Message));;
                msg = searchMessageType(msg_type, msg_head);

                //If msg is found from the msg queue, write the msg back to the client and remove the node from the msg queue
                if (msg != NULL) {
                    printf("\nMessage found! Sending...\n");
                    write(writeFD, msg->msg, msg->msg_length);

                    //Removing msg from msg queue
                    removeMsg(msg_type, &msg_head);
                } 
                else {

                    //Otherwise, add the client to waiting client queue, including the simPID, writeFD, and msg_type
                    printf("\nMessage with msg_type %d not found\n", msg_type);
                    struct waitingClient* newWaitingClient = (struct waitingClient*)malloc(sizeof(struct waitingClient));
                    newWaitingClient->simPID = simPID;
                    newWaitingClient->writeFD = writeFD;
                    newWaitingClient->msg_type = msg_type;

                    if(waitingClient_head == NULL) {
                    waitingClient_head = newWaitingClient;
                    }
                    else{
                        struct waitingClient* curr = waitingClient_head;
                        while(curr->next != NULL){
                            curr = curr->next;
                        }
                        curr->next = newWaitingClient;
                    }
                }

                printf("\n\nRequest processed!\nSystem Call: %d\nSimulated ProcessID: %d\nMessage Type: %d\n\n", syscallNumber, simPID, msg_type);

                break;

            
            //HW2 Revision: Have terminate and shutdown calls do the same operation
            case 0:
            case -1:

                //HW2 Revision: change param list for exit_client() from HW1
                exit_client(simPID, &head);
                //HW2 Addition: If last client terminates (empty list), shutdown server program 
                if(head == NULL){
                    //Clean up
                    printf("\nClosing server FIFOS. BYE BYE!!!\n");
                    close(readFD);
                    unlink(SERVER_FIFO);
                    return 0;
                }
                break;

            }
            for (int i = 0; i < 50; i++){
                printf("#");
            }
            printf("\n");
    }
    return 0;
}

//HW2 Utility function: print linked list
void printLinkedList(struct ClientNode* head) {
    for (int i = 0; i < 10; i++) {
        printf("*");
    }
    printf("\n"); 
    struct ClientNode* curr = head;
    while(curr != NULL){
        printf("simPID: %d\nwriteFD: %d\nclientFifoName: %s\n\n", curr->simPID, curr->writeFD, curr->clientFifoName);
        curr = curr->next;
    }
    for (int i = 0; i < 10; i++) {
        printf("*");
    }
    printf("\n"); 
}

//HW2 Addition: search linked list node function and assign writeFD to found node's writeFD
void searchNode(int simPID, struct ClientNode* head) {
    struct ClientNode* current = head;
    while(current != NULL && current->simPID != simPID) {
        current = current->next;
    }
    if(current != NULL){
        writeFD = current->writeFD;
        strcpy(clientFifoName, current->clientFifoName);
    }
}

//HW3 Addition: search linked list node function to find msg_type requested by client
struct Message* searchMessageType(int msg_type, struct Message* head) {
    struct Message* current = head;
    while(current != NULL && current->msg_type != msg_type) {
        current = current->next;
    }
    if(current != NULL){
        return current;
    }
    return NULL;
}

//HW3 Addition: search linked list node function to find clients waiting on particular msg_type
struct waitingClient* searchWaitingClient(int msg_type, struct waitingClient* head) {
    struct waitingClient* current = head;
    while(current != NULL && current->msg_type != msg_type) {
        current = current->next;
    }
    if(current != NULL){
        return current;
    }
    return NULL;
}

//HW3 Addition: remove msg from msg linked list queue
void removeMsg(int msg_type, struct Message** head) {
    struct Message* temp = *head;
    struct Message* prev = NULL;

    if(temp != NULL && temp->msg_type == msg_type) { 
        *head = temp->next;
        free(temp);
        return;
    }

    while(temp != NULL && temp->msg_type != msg_type) {
        prev = temp;
        temp = temp->next;
    }

    prev->next = temp->next;
    free(temp);
}

//HW3 Addition: remove waiting client from waiting client linked list queue
void removeWaitingClient(int msg_type, struct waitingClient** head) {
    struct waitingClient* temp = *head;
    struct waitingClient* prev = NULL;

    if(temp != NULL && temp->msg_type == msg_type) { 
        *head = temp->next;
        free(temp);
        return;
    }

    while(temp != NULL && temp->msg_type != msg_type) {
        prev = temp;
        temp = temp->next;
    }

    prev->next = temp->next;
    free(temp);
}

void exit_client(int simPID, struct ClientNode** head) {

    //Server program must close the client specific fifo and continue to receive the next system call (ready for the next client to connect)
    printf("\nClosing %s\n", clientFifoName);

    //Server closes client FIFO. Also close writeFD since client FIFO will no longer exist
    close(writeFD);
    unlink(clientFifoName);

    //HW2 Addition: Delete node from list
    //Case when deleting head of list
    struct ClientNode* temp = *head;
    struct ClientNode* prev = NULL;

    if(temp != NULL && temp->simPID == simPID){
        *head = temp->next;
        free(temp);
        return;
    }

    //Case otherwise
    while(temp != NULL && temp->simPID != simPID){
        prev = temp;
        temp = temp->next;
    }
    //Node next of temp is the one to be deleted
    prev->next = temp->next;
    free(temp);
}
