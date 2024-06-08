#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PASSENGER_PLANE 1
#define CARGO_PLANE 0
#define NUM_CREW_MEMBERS 7
#define CREW_WEIGHT 75
#define NUM_PILOTS 2
#define planeATC 100
#define MAX_MSG_LEN 200

// Message queue structure
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

int calculateTotalWeight(int numPassengers, int pipe_fds[][2]) {
    int totalWeight = NUM_CREW_MEMBERS * CREW_WEIGHT; // Starting with the crew's weight
    int weights[2];  // Array to store luggage weight and body weight from each passenger
    
    for (int i = 0; i < numPassengers; i++) {
        close(pipe_fds[i][1]); // Close the writing end in the parent

        // Read weights from the pipe
        read(pipe_fds[i][0], weights, sizeof(weights));
        totalWeight += weights[0] + weights[1];
        close(pipe_fds[i][0]); // Close the reading end after done
    }
    return totalWeight;
}

void handlePassengerPlane(struct msgbuf *msg) {
    int pipe_fds[msg->numPC][2];
    for (int i = 0; i < msg->numPC; i++) {
        pipe(pipe_fds[i]);
        if (fork() == 0) {
            int weights[2];
            close(pipe_fds[i][0]);  // Close read end in child
            printf("Enter Weight of Your Luggage: ");
            scanf("%d", &weights[0]);
            printf("Enter Your Body Weight: ");
            scanf("%d", &weights[1]);
            write(pipe_fds[i][1], weights, sizeof(weights));
            close(pipe_fds[i][1]); // Close write end in parent
            exit(0);
        }
        wait(NULL);
         
    }
    while (wait(NULL) > 0);
    msg->weight = calculateTotalWeight(msg->numPC, pipe_fds);
    printf("Total weight of passengers and luggage: %d kg\n", msg->weight);
    

}


void handleCargoPlane(struct msgbuf *msg) {
    printf("Enter Number of Cargo Items: ");
    scanf("%d", &msg->numPC);
    int avgWeight;
    printf("Enter Average Weight of Cargo Items: ");
    scanf("%d", &avgWeight);
    int TcrewWeight = NUM_PILOTS*CREW_WEIGHT;
    msg->weight = msg->numPC * avgWeight + TcrewWeight;
    printf("Total weight of cargo: %d kg\n", msg->weight);
}



int main() {
    struct msgbuf msg;
    // Get input values for plane details
    printf("Enter Plane ID: ");
    scanf("%d", &msg.planeID);

    printf("Enter Type of Plane (1 for Passenger Plane, 0 for Cargo Plane): ");
    scanf("%d", &msg.planeType);

    key_t key = ftok("AirTrafficController.txt",'b');  // Path needs to match with ATC program
    int msgid = msgget(key, 0666 | IPC_CREAT);  // Connect to the message queue
    if (msgid == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    if (msg.planeType == PASSENGER_PLANE) {
        printf("Enter Number of Occupied Seats: ");
        scanf("%d", &msg.numPC);
        handlePassengerPlane(&msg);
    } else if (msg.planeType == CARGO_PLANE) {
        handleCargoPlane(&msg);
    }
	
    printf("Enter Airport Number for Departure: ");
    scanf("%d", &msg.depAirport);
    
    printf("Enter Airport Number for Arrival: ");
    scanf("%d", &msg.ArrAirport);

    msg.mtype = planeATC;
    msg.status = 'D';
    if(msgrcv(msgid, &msg, sizeof(msg)-sizeof(long),400,IPC_NOWAIT) != -1) {


            msg.mtype = 400;
	    msgsnd(msgid, &msg, sizeof(msg)-sizeof(long), 0);
	    printf("Airport Processes are closed\n");
	    exit(0);
    }
    
    if(msgsnd(msgid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
	    perror("Error sending message");
	    exit(EXIT_FAILURE);
    }
    int recieveID = planeATC + msg.planeID;
    if(msgrcv(msgid, &msg, sizeof(msg)-sizeof(long),recieveID,0) == -1) {
	    perror("Error Recieving message");
	    exit(EXIT_FAILURE);
    }
    printf("Plane %d taken off from Airport %d",msg.planeID,msg.depAirport);
    //Travel duration of 30sec
    for (int i = 30; i > 0; i--) {
        printf("\rTravelling.. %d seconds remaining", i);
        fflush(stdout);  // Force the output to be written to the terminal
        sleep(1);
    }
    printf("\rWaiting for Landing!!                \n");
    
    msg.mtype = planeATC;
    msg.status = 'A';
    if(msgsnd(msgid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
	    perror("Error sending message");
	    exit(EXIT_FAILURE);
    }
    recieveID = planeATC + msg.planeID;
    if(msgrcv(msgid, &msg, sizeof(msg)-sizeof(long),recieveID,0) ==-1) {
	    perror("Error Recieving message");
	    exit(EXIT_FAILURE);
    }
    printf("Plane %d has successfully traveled from Airport %d to Airport %d!",msg.planeID,msg.depAirport,msg.ArrAirport);

    return 0;
}

