#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_RUNWAYS 11 // Including the backup runway
#define MAX_MSG_LEN 200
#define airportATC 200

struct message {
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

struct runway {
  int id;
  int loadCapacity;
};

struct airport {
  int airportNumber;
  int numRunways;
  int msgid; // Added message queue identifier
  struct runway runways[MAX_RUNWAYS];
  pthread_mutex_t mutexes[MAX_RUNWAYS];
};

struct airport airport_instance;
int count=0;
bool terminate  = false;

void *handle_operation(void *arg) {
  struct message msg = *(struct message *)arg;
  int planeID, weight;
  char type;
  planeID = msg.planeID;
  weight = msg.weight;
  type = msg.status;

  int bestFitIndex = -1, minDiff = INT_MAX;

  // Identify the best fit runway (excluding backup)
  for (int i = 0; i < airport_instance.numRunways; i++) {
    if (airport_instance.runways[i].loadCapacity >= weight) {
      if (minDiff > airport_instance.runways[i].loadCapacity - weight) {
        minDiff = airport_instance.runways[i].loadCapacity - weight;
        bestFitIndex = i; // Update the best fit index
      }
    }
  }

  // Check if a suitable runway is found
  if (bestFitIndex != -1) {
    pthread_mutex_lock(
        &airport_instance.mutexes[bestFitIndex]); // Lock the best-fit runway or
                                                  // wait if it's occupied
  } else {
    // If no suitable runway is found, use the backup runway
    bestFitIndex = airport_instance.numRunways;
    pthread_mutex_lock(
        &airport_instance.mutexes[bestFitIndex]); // Lock the backup runway
  }

  // Continue with operations on the runway
  if (type == 'D') {
    sleep(3);// Simulate the boarding/loading process
    sleep(2); // Simulate takeoff
    printf("Plane %d has completed boarding/loading and taken off from Runway "
           "No. %d of Airport No. %d.\n",
           planeID, airport_instance.runways[bestFitIndex].id,
           airport_instance.airportNumber);
  } else if (type == 'A') {
    sleep(2); // Simulate landing
    sleep(3); // Simulate deboarding/unloading
    printf("Plane %d has landed on Runway No. %d of Airport No. %d and has "
           "completed deboarding/unloading.\n",
           planeID, airport_instance.runways[bestFitIndex].id,
           airport_instance.airportNumber);      
  }

  // Send a message to the queue about the completion
  struct message completionMsg;
  msg.mtype = 200; 
  //sprintf(completionMsg.mtext,
   //       "Plane %d has %s at Airport No. %d on Runway No. %d", planeID,
   //       type == 'D' ? "taken off" : "landed", airport_instance.airportNumber,
    //      airport_instance.runways[bestFitIndex].id);
  msgsnd(airport_instance.msgid, &msg, sizeof(msg)-sizeof(long), 0);

  pthread_mutex_unlock(&airport_instance.mutexes[bestFitIndex]);
  free(arg);
  if(type == 'A'){
  count--;
  }
  return NULL;
}

int main() {
  printf("Enter Airport Number: ");
  if (scanf("%d", &airport_instance.airportNumber) != 1) {
    perror("Failed to read airport number");
    return EXIT_FAILURE;
  }

  printf("Enter number of Runways: ");
  if (scanf("%d", &airport_instance.numRunways) != 1) {
    perror("Failed to read number of runways");
    return EXIT_FAILURE;
  }

  printf("Enter loadCapacity of Runways (give as a space separated list in a "
         "single line): ");
  for (int i = 0; i < airport_instance.numRunways; i++) {
    if (scanf("%d", &airport_instance.runways[i].loadCapacity) != 1) {
      perror("Failed to read runway load capacity");
      return EXIT_FAILURE;
    }
    airport_instance.runways[i].id = i + 1;
    pthread_mutex_init(&airport_instance.mutexes[i], NULL);
  }
  airport_instance.runways[airport_instance.numRunways].loadCapacity =
      15000; // Backup runway
  airport_instance.runways[airport_instance.numRunways].id =
      airport_instance.numRunways + 1;
  pthread_mutex_init(&airport_instance.mutexes[airport_instance.numRunways],
                     NULL);

  key_t key = ftok("AirTrafficController.txt",'b');
  airport_instance.msgid = msgget(key, 0666 | IPC_CREAT);
  if (airport_instance.msgid == -1) {
    perror("Failed to create message queue");
    return EXIT_FAILURE;
  }

  while (1) {
    struct message msg;

    int recieveId = airportATC + airport_instance.airportNumber;
    if (msgrcv(airport_instance.msgid, &msg, sizeof(msg), recieveId,0) != -1 ) {
	      if(strcmp(msg.mtext,"Terminate") == 0){
	      	 terminate = true;	
	      }
	      else{	
		      if(msg.status == 'W'){
			  count++;
		      }else{
		    
			      pthread_t tid;
			      struct message *new_msg = malloc(sizeof(struct message));
			      memcpy(new_msg, &msg, sizeof(msg));
			      if (pthread_create(&tid, NULL, handle_operation, new_msg) != 0) {
				perror("Error creating thread");
				free(new_msg);
			      }
			      pthread_detach(tid);
		      }
		      
	      }	
	      
	      if(terminate && count ==0){
	      
	             msg.status = 'T';
	             msg.mtype = 200;
	             msgsnd(airport_instance.msgid, &msg, sizeof(msg)-sizeof(long), 0);
	      
	             for (int i = 0; i <= airport_instance.numRunways; i++) {
   			pthread_mutex_destroy(&airport_instance.mutexes[i]);
  		     }
  		     
  		     return 0;
	      }      
    } 
  }

  
  return 0;
}
