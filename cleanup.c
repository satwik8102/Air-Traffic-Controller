#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#define MAX_MSG_LEN 200

// Structure for message
struct msgbuf {
    long mtype;        
    int planeID;      
    int depAirport;
    int ArrAirport;    
    int weight;
    int planeType;
    int numPC;
    char status;
    char mtext[MAX_MSG_LEN];  
};

int main() {
    // Create message queue
    
    key_t key = ftok("AirTrafficController.txt",'b');  // Path needs to match with ATC program
    int msg_id = msgget(key, 0666 | IPC_CREAT);  // Connect to the message queue
    if (msg_id == -1) {
        perror("Error in creating message queue");
        exit(EXIT_FAILURE);
    }

    // Loop for cleanup process
    while (1) {
        printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No)\n");
        char input;
        scanf(" %c", &input);

        // Check user input
        if (input == 'Y' || input == 'y') {
            // Inform Air Traffic Controller to terminate
            struct msgbuf message;
            message.mtype = 300;
            sprintf(message.mtext, "Terminate");
            if (msgsnd(msg_id, &message, sizeof(message)-sizeof(long), 0) == -1) {
                perror("Error in sending termination message to Air Traffic Controller");
                exit(EXIT_FAILURE);
            }

            // Cleanup and exit
            printf("Cleanup process has informed Air Traffic Controller to terminate.\n");
            exit(EXIT_SUCCESS);
        } else if (input == 'N' || input == 'n') {
            // Continue running
            printf("Cleanup process continues to run.\n");
        } else {
            printf("Invalid input. Please enter Y or N.\n");
        }
    }

    return 0;
}
