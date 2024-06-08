#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_MSG_LEN 200

// Struct for message queue
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

int count = 0;
int numAirports, msgid;
bool terminate;

// Function to initialize message queue
int initializeMessageQueue() {
    key_t key = ftok("AirTrafficController.txt", 'b');  
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }
    return msgid;
}

// Function to send messages via message queue
void sendMessage(int msgid, long mtype, struct msgbuf buffer) {

    struct msgbuf msg;
    msg = buffer;
    msg.mtype = mtype;
    if(msgsnd(msgid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
	    perror("Error sending message");
	    exit(EXIT_FAILURE);
    } 		    	        
	    
}

// Function to receive messages via message queue

void receiveMessage(int msgid){
	struct msgbuf msg;
	if(msgrcv(msgid, &msg, sizeof(msg)-sizeof(long),0,0) == -1){
		perror("Error sending message\n");
	    	exit(EXIT_FAILURE);
	}
	if(msg.mtype == 100){
		if(msg.status == 'D'){
			int portId = 200+ msg.depAirport;
			sendMessage(msgid, portId, msg);
			msg.status = 'W';
			portId = 200 + msg.ArrAirport;
			sendMessage(msgid,portId,msg);
		}
		else if(msg.status == 'A'){
			int portId = 200+ msg.ArrAirport;
			sendMessage(msgid, portId, msg);
			
		}	
	}else if(msg.mtype == 200){
		if(msg.status == 'D'){
			int planeId = 100 + msg.planeID;
			sendMessage(msgid,planeId,msg);
			FILE *file = fopen("AirTrafficController.txt", "a");
			if (file == NULL) {
			     perror("Failed to open file");
			     exit(EXIT_FAILURE);
			}
			printf("Plane %d has departed from Airport %d and will land at Airport %d.\n", msg.planeID,msg.depAirport,msg.ArrAirport);
			fprintf(file, " Plane %d has departed from Airport %d and will land at Airport %d.\n", msg.planeID,msg.depAirport,msg.ArrAirport);
			fclose(file);
		}
		else if(msg.status == 'A'){
		       	int planeId = 100+ msg.planeID;
		       	sendMessage(msgid,planeId, msg);
		       	if(terminate){
		       	
			       	for(int i=1;i<=numAirports;i++){
		    			sprintf(msg.mtext,"Terminate");
		    			int id = 200+i;
		    			sendMessage(msgid,id,msg);
		    		}
		    	}	
		}else if(msg.status == 'T'){
			count++;
			if(count == numAirports){
				if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        				perror("Failed to remove message queue");
        				exit(EXIT_FAILURE);
    				}
    			        exit(0);	

			}
				
		}
	}else if(msg.mtype == 300){
	    
		terminate = true;
	    for(int i=1;i<=numAirports;i++){
	    	sprintf(msg.mtext,"Terminate");
	    	int id = 200+i;
	    	int i= 400;
	    	sendMessage(msgid,i,msg);	
	    	sendMessage(msgid,id,msg);
	    }
	    
	    
	}else if(msg.mtype == 400){
	      int i= 400;
	      sendMessage(msgid,i,msg);
	}	
	

}

int main() {
    
    struct msgbuf buffer;
    

    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &numAirports);

    msgid = initializeMessageQueue();

    while (1) {
        receiveMessage(msgid); 
    }

    
    
    return 0;
}

