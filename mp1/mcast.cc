#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>

#include <list>

#include "mp1.h"

using namespace std;

class node{

	public:
		int *timestamp;
		int length;
		int source;
		int seq_num;
		char* message;
};

list<node> msg_queue;

/* Josh's Section */



/* Tommy's Section */



void multicast_init(void) {
    unicast_init();
}

void multicast(const char *message) {
    int i;

    pthread_mutex_lock(&member_lock);
    for (i = 0; i < mcast_num_members; i++) {
        usend(mcast_members[i], message, strlen(message)+1);
    }
    pthread_mutex_unlock(&member_lock);
}

void receive(int source, const char *message, int len) {
    assert(message[len-1] == 0);
    deliver(source, message);
}

void mcast_join(int member) {

}












// /* HELP */
// /*	Reliable multicast with causal ordering and failure detection
//  *	CS 425 MP1
//  *	Arjan Singh Nirh
//  *	Shpendi Jashri
//  */
// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <assert.h>
// #include <signal.h>
// #include <sys/time.h>
// #include <pthread.h>
// #include <unistd.h>

// #include "mp1.h"

// #define TIMER_INTERVAL 10000
// #define TAG_NACK 3
// #define TAG_HEARTBEAT 2
// #define TAG_NORMAL_MESSAGE 1

// /* Node structure for linked list
//  * Linked list maintains buffered messages
//  */ 
// typedef struct _node_{
// 	int *timestamp;
// 	char *message;
// 	int source;
// 	int length;

// 	int seq_num;
// 	struct _node_ *prev;
// 	struct _node_ *next;
// }node;



// /* Global Variables */
// int *my_timestamp = NULL;
// int vector_len = 0;
// int sorted = 0;
// int *map = NULL;

// int *old_seq=NULL;
// int *new_seq=NULL;
// int seq_size=0;
// int sequence_num=0;
// pthread_t heartbeat_thread;
// pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
// //pthread_cond_t suspend_cond = PTHREAD_COND_INITIALIZER;

// /* Hold back queue variables */
// node *list_head = NULL;
// node *tail = NULL;

// /* Sent messages - queue variables */
// node *sentQ_head = NULL;
// node *sentQ_tail = NULL;

// /* Function declarations */
// int compare(const void *a, const void *b);
// int getindex(int pid);
// void add_node(char* original_message,int* incoming_timestamp,int source);
// char* concatenate_timestamp(const char* message);
// void pop(node ** curr_dbl_ptr, int);
// void check_buffered_messages(int current_process_index, int* is_buffer_ptr, int* is_reject_ptr, int* incoming_vector, int);
// void sort_array();
// void shout_state();
// void store_sent_message(char *message,int length,int seq_num);
// void retransmit_message(int seq_num, int nack_source);
// void send_nack(int seq_num, int dest);
// void *heartbeater(void*);

// /* Print debugging information
//  */ 
// void shout_state(){
// 	// debugprintf("vector_len = %d\n", vector_len);
// }

// /* Initilize global variables
//  */ 
// void multicast_init(void) {
// 	// debugprintf("CALLING MULTICAST INIT\n");
//     unicast_init();
	
// 	//Make thread that sends out heartbeats and also periodically checks old and new seq arrays
// 	pthread_create(&heartbeat_thread, NULL, heartbeater, NULL);
// }

// /* Basic multicast implementation */
// /*
//  * 1. increment vector timestamp (increment current process's val in vector)
//  * 2. append the vector timestamp to each message
//  */ 
// void multicast(const char *message) {


// 	//Check if this is first call to mcast
// 	if(sorted ==0){
// 		sort_array();					//sort our local pid array
// 		sorted = 1;
// 	}
	
// 	//Deliver message to myself first
//     deliver(my_id, message);

//     // increment vector timestamp (increment current process's val in vector)
//  	int myindex = getindex(my_id);

// 	if(myindex >=vector_len){
// 		shout_state();
// 	}
// 	pthread_mutex_lock(&thread_mutex);
// 	my_timestamp[myindex]+=1;
// 	pthread_mutex_unlock(&thread_mutex);

// 	/* Append timestamps to beginning of message
// 	  "hello" --> "1 0 0 hello"
// 	 */
// 	char *new_message = concatenate_timestamp(message);
// 	int new_len = strlen(new_message)+1;

// 	/* Send ucast with timestamp+message to every process except my self*/
//     int i;
//     pthread_mutex_lock(&member_lock);
//     for (i = 0; i < mcast_num_members; i++) {
// 		if(mcast_members[i] != my_id)
//         	usend(mcast_members[i], new_message, new_len);
// 		//usend(mcast_members[i], message, strlen(message)+1);
//     }
//     pthread_mutex_unlock(&member_lock);
	
// 	store_sent_message(new_message, new_len, my_timestamp[myindex]); 

// }

// //Easy function to throw into multicast send in order to buffer all sent messages.
// void store_sent_message(char *message,int length,int seq_num){		

// 	node *curr = (node*) malloc(sizeof(node));			
// 	curr->message = (char*) malloc(length*sizeof(char));
// 	strcpy(curr->message, message);
// 	curr->length = length;
// 	curr->seq_num = seq_num;

// 	if(sentQ_head == NULL){
// 			sentQ_head = curr;
// 			sentQ_tail = curr;
// 			curr->next = NULL;
// 			curr->prev = NULL;
// 	}
// 	else{
// 		sentQ_tail->next = curr;
// 		curr->prev = sentQ_tail;
// 		sentQ_tail = curr;
// 	}

// }


// /*
//  * 1. First parse incoming vector
//  * 2. Check incoming vector against current vector: It should be identical AND vector[source] shd be = incomingVec[source]+1
//  * 3. if false, then buffer queue message and source and timestamp
//  * 4. If true, then deliver and update vector
//  * 5. If deliver, then check vectors in queue like step 3 and repeat
//  */ 
// void receive(int source, const char *message, int len) {
//     assert(message[len-1] == 0);

// 	int i=0;
// 	//Check if this is first call to mcast
// 	pthread_mutex_lock(&thread_mutex);
// 	if(sorted ==0){
// 		//sort our local pid array
// 		sort_array();
// 		sorted = 1;
// 	}
// 	pthread_mutex_unlock(&thread_mutex);
	
// 	//1. Parse into vector and message
// 	//What kind of message
// 	int tag = 0;
// 	sscanf(message, "%d ", &tag);
	
// 	if(tag == TAG_NACK){
// 		/* NACK MESSAGE */
// 		int seq = 0;
// 		sscanf(message+2, "%d ", &seq);
// 		retransmit_message(seq, source);
// 	}
// 	else if(tag == TAG_HEARTBEAT){
// 		/* HEARTBEAT MESSAGE */

// 		int index = 0;
// 		int i=0;
// 		pthread_mutex_lock(&member_lock);
// 		for(i=0;i<mcast_num_members; i++){
// 			if(source == mcast_members[i]){
// 				index = i;
// 			}
// 		}
// 		pthread_mutex_unlock(&member_lock);
	
// 		int seq = 0;
// 		sscanf(message+2, "%d ", &seq);
// 		new_seq[index] = seq;
// 	}
// 	else if(tag == TAG_NORMAL_MESSAGE){

// 		/* NORMAL MESSAGE */
// 	pthread_mutex_lock(&thread_mutex);
// 	char num_str[256];
// 	char* message_ptr = (char*)message + (2);
		
// 		sscanf(message_ptr, "%s ", num_str);

// 	int incoming_timestamp[vector_len];
// 	for(i=0;i<vector_len; i++){

// 		sscanf(message_ptr, "%s ", num_str);
// 		sscanf(message_ptr, "%d ", &(incoming_timestamp[i]));
// 		message_ptr = message_ptr + strlen(num_str)+1;

// 	}
// 	char* original_message = (char*) message+((i+1)*2);					//CHECK: should we use strcpy?

// 	//2. check timestamps for ordering
// 	int is_buffer = 0;
// 	int is_reject = 0;
// 	int current_process_index = getindex(source);

// 	check_buffered_messages(current_process_index, &is_buffer, &is_reject, incoming_timestamp, 0);
// 	//3. If out of order, then store message and timestamp and the source into queue
// 	if(is_reject==1){
// 		pthread_mutex_unlock(&thread_mutex);
// 		return;
// 	}
// 	else if(is_buffer==1){
// 		add_node(original_message, incoming_timestamp, source);
// 	}
// 	else{
// 		//Copy over timestamp
// 		for(i=0;i<vector_len;i++)
// 			my_timestamp[i] = incoming_timestamp[i];

//     	deliver(source, original_message);

	
// 		//Now check the queue, if you can deliver any buffered messages
// 		node* curr = list_head;
// 		while(curr!=NULL){

// 			int index = getindex(curr->source);
// 			int is_buffer=0, is_reject=0;
// 			check_buffered_messages(index, &is_buffer, &is_reject, curr->timestamp, 1);
// 			if(is_reject == 1){
// 				node* old_curr = curr;
// 				curr = curr->next;
// 				//only pop dont deliver
// 				pop(&old_curr, 0);
				
// 			}
// 			else if(is_buffer ==0){

// 				//Copy over timestamp
// 				for(i=0;i<vector_len;i++)
// 					my_timestamp[i] = curr->timestamp[i];

// 				node* old_curr = curr;
// 				curr = curr->next;
// 				//pop and deliver
// 				pop(&old_curr, 1);
// 				curr =list_head ;

// 			}
// 			else		
// 				curr = curr->next;
// 		}
// 	}
// 	pthread_mutex_unlock(&thread_mutex);
// 	}
	
// }
// /*
//  * After receiving nack, retransmit message to source of NACK
//  */
// void retransmit_message(int seq_num, int nack_source){
	
// 	// debugprintf("RETRANSMITTING message i=%d to process = %d\n", seq_num, nack_source);

// 	if(sentQ_head ==NULL)
// 		return;

// 	node *curr = sentQ_head;
// 	while(curr->next !=NULL){
// 		if(curr->seq_num == seq_num){
// 			usend(nack_source, curr->message, curr->length);
// 			return;
// 		}
// 		else
// 			curr = curr->next;
// 	}

// }

// /* Remove a node from linked list
//  *  @param isDeliver: if after popping node, should the message be delivered
//  */ 
// void pop(node ** curr_dbl_ptr, int isDeliver){

// 	node* curr = *curr_dbl_ptr;
// 	node* first = curr->prev;
// 	node* second = curr->next;
	
// 	if(first!=NULL){
// 		first->next = second;
// 	}
// 	else
// 		list_head = curr->next;

// 	if(second!=NULL)
// 		second->prev = first;
// 	else
// 		tail = curr->prev;

	
// 	//deliver curr's message
// 	if(isDeliver == 1)
//     	deliver(curr->source, curr->message);

// 	node* deleteptr = curr;
// 	curr = second;
// 	//deallocate curr
	
// 	free(deleteptr->message);
// 	free(deleteptr->timestamp);
// 	free(deleteptr);

// }

// /*
//  * Checks if a recieved message should be buffered or rejected
//  * Sends Nacks if out of order message received
//  */
// void check_buffered_messages(int current_process_index, int* is_buffer_ptr, int* is_reject_ptr, int* incoming_vector, int is_in_buffer){

// 	int i=0;
// 	int j=0;
// 	/* Go through the timestamp */
// 	for(i=0;i<vector_len; i++){
// 		/* Skip failed processes */
// 		if(old_seq[i] == -2)
// 			continue;
// 		/* If not source process index, then everything should be equal
// 		 * If current timestamp is more than incoming timestamp then reject
// 		 * Otherwise buffer the message
// 		 */
// 		if(i!=current_process_index){
// 				if(incoming_vector[i] < my_timestamp[i]){
// 					*is_reject_ptr = 1;
// 					return;
// 				}
// 				if(my_timestamp[i] != incoming_vector[i]){
// 					*is_buffer_ptr = 1;
					
// 					if(is_in_buffer==0){
// 						//Send out NACKS 
// 						for(j=my_timestamp[i]+1; j<=incoming_vector[i]; j++){
// 							int seq = j;
// 							int dest = map[i];
// 							// debugprintf("Sending NACK for seq=%d\n", seq);
// 							send_nack(seq, dest);
// 						}
// 					}
// 				}
// 		}
// 		/* If source process index, then check if less than */
// 		else{
// 				if(incoming_vector[i] <= my_timestamp[i]){			//TODO changed this
// 					*is_reject_ptr = 1;
// 					return;
// 				}
// 				if(incoming_vector[i] - my_timestamp[i] > 1){
// 					*is_buffer_ptr = 1;
// 					//break;

// 					if(is_in_buffer==0){
// 					//Send out NACKS
// 						for(j=my_timestamp[i]+1; j<incoming_vector[i]; j++){
// 							int seq = j;
// 							int dest = map[i];
// 							// debugprintf("Sending NACK for seq=%d\n", seq);
// 							send_nack(seq, dest);
// 						}
// 					}
// 				}
// 		}
// 	}
// }

// /* Sends a NACK message with sequence number of required message to appropritate destination */
// void send_nack(int seq_num, int dest){

// 	//Construct the NACK
// 	int len = 5;
// 	char *message = (char*) malloc(len*sizeof(char));
// 	sprintf(message, "%d %d ", TAG_NACK, seq_num);
// 	message[len-1] = '\0';
    
// 	usend(dest, message, len);
// }


// /* Buffer message + timestamp to linked list
// */
// void add_node(char* original_message,int* incoming_timestamp,int source){


// 	node* curr = NULL;
// 	if(list_head == NULL){
// 		curr = (node*) malloc(sizeof(node));
// 		list_head = curr;
// 		list_head->next = NULL;
// 		list_head->prev = NULL;
// 		tail = list_head;

// 	}
// 	else{
// 		tail->next = (node*) malloc(sizeof(node));
// 		curr = tail->next;
// 		curr->next = NULL;
// 		curr->prev = tail;
// 	}

// 		curr->source = source;
// 		int len = strlen(original_message);
// 		curr->message = (char*) malloc((sizeof(char))*(len+1));
// 		strcpy(curr->message, original_message);
// 		curr->timestamp = (int*) malloc(vector_len * (sizeof(int)));

// 		int i=0;
// 		for(i=0;i<vector_len; i++){
// 			curr->timestamp[i] = incoming_timestamp[i];
// 		}

// 		tail = curr;
// }

// /* Concatenate timestamp to beginning of message
// *	Allocate new memory and append string
// *	The caller is responsible for deallocating the memory
// */
// char* concatenate_timestamp(const char* message){
	
// 	char* new_message = (char*) malloc(256);
// 	memset(new_message, 0, 256);
	
// 	//Concatenate first timestamp (null terminates the new message)
// 	char temp[256];
// 	sprintf(temp, "%d ", TAG_NORMAL_MESSAGE);
// 	//temp[2] = '\0';
// 	strcpy(new_message, temp);

// 	//concatenate each timestamp index
// 	int i=0;
// 	for(i=0;i<vector_len; i++){
// 		char temp2[256];
// 		sprintf(temp2, "%d ", my_timestamp[i]);
// 		strcat(new_message, temp2);		
// 	}

// 	//concatenate the real message
// 	strcat(new_message, message);		

// 	return new_message;
// }

// /* Get index of given pid form local array */
// int getindex(int pid){

// 	int i=0;
//     //pthread_mutex_lock(&member_lock);
// 	for(i=0;i<mcast_num_members; i++){
// 		if(map[i] == pid)
// 			return i;
// 	}
//     //pthread_mutex_unlock(&member_lock);

// 	//No pid found
// 	return 0;

// }

// /* Called everytime a new process joins */
// void mcast_join(int member) {
	
// 		vector_len++;
// 		// debugprintf("Called mcast join, vector_len=%d\n", vector_len);
// 		my_timestamp =(int*) realloc(my_timestamp, (sizeof(int))* vector_len);
// 		int i=0;
// 		for(i=0;i<vector_len; i++){
// 			my_timestamp[i] = 0;
// 		}

// 	pthread_mutex_lock(&member_lock);
	
// 	//Resize seq arrays if receive from new member
// 	if(seq_size != mcast_num_members){

// 		old_seq = (int*) realloc(old_seq, sizeof(int)* mcast_num_members);
// 		new_seq = (int*) realloc(new_seq, sizeof(int)* mcast_num_members);

// 		int i=0;
// 		for(i=seq_size; i<mcast_num_members; i++){
// 			old_seq[i] = -1;
// 			new_seq[i] = 0;
// 		}
// 		seq_size = mcast_num_members;
// 	}


// 	pthread_mutex_unlock(&member_lock);
// 		sort_array();

// }

// /* For synchronizing pid array across all processes */
// void sort_array(){
// 	int i=0;

// 	map = (int*) realloc(map, (sizeof(int))*mcast_num_members);
// 	for(i=0;i<mcast_num_members;i++){
// 		map[i] = mcast_members[i];
// 	}
	
// 	qsort(map, mcast_num_members, sizeof(int), compare);

// }

// int compare(const void *a, const void *b){

// 	if(a==NULL || b== NULL)
// 		return 0;
// 	return ( *((int*)a) - *((int*)b));

// }

// /* Heartbeat thread's function call
//  * Sends heartbeat to all alive processes every 5 seconds
//  * Checks for failures every 20 seconds
//  */ 
// void *heartbeater(void*){
// 	int i=0;
// 	int myindex = 0;
// 	int counter = 0;
// 	while(1){
// 		counter++;
// 		pthread_mutex_lock(&member_lock);

// 		//Send out heartbeats to each process in the group

// 		for(i=0;i<mcast_num_members; i++){
// 			if(mcast_members[i] == my_id){
// 				myindex = i;	
// 				continue;
// 			}
// 			sequence_num++;											//TODO: need to check for overflow? make long? reset?
// 			char message[5];
// 			sprintf(message, "%d %d ", TAG_HEARTBEAT, sequence_num);
// 			message[4] = 0;
// 			// debugprintf("Sending heartbeat message: %s\n", message);
// 			usend(mcast_members[i], message, strlen(message)+1);
			
// 		}
// 		pthread_mutex_unlock(&member_lock);

// 		if(counter%4 == 0){
// 			//check old and new arrays to find failures
// 			for(i=0; i<seq_size; i++){
// 				//check failure
				
// 				if(i!=myindex && new_seq[i] <= old_seq[i] && new_seq[i]!=-1){			//-1 means we already detected it before
// 					// debugprintf("-----[%d]: Process %d has failed.\n", my_id, mcast_members[i]); 
// 					new_seq[i] = -2;			//denote failure
// 		pthread_mutex_lock(&member_lock);
// 					mcast_num_members--;
// 		pthread_mutex_unlock(&member_lock);
// 				}
				
// 				//copy from new to old
// 				old_seq[i] = new_seq[i];
// 			}
// 		}

// 		// debugprintf("Pausing heartbeat thread \n");
// 		//pause or suspend until woken up by timer
// 		sleep(5);
// 	}

// }

