#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <algorithm>
#include <unistd.h>
#include <vector>
#include <list>
#include <string>
#include <queue>
#include "mp1.h"

using namespace std;

class node{

	public:
		vector<int> timestamp;
		int length;
		int source;
		int seq_num;
		char* message;
};

struct msgNode {
	int src;
	int dest;
	std::vector<int> timestamp;
	char* msg;
};
/* 
	HASH TABLE
	Used to store the timestamps
*/
const int TABLE_SIZE = 24;

class TimeI {
	private:
	int key;
	int* vector;

	public:
	TimeI(int key, int* vector) {
	    this->key = key;
	    this->vector = vector;
	}

	int getKey() {
	    return key;
	}

	int* getVector() {
	    return vector;
	}

	void setVector(int* vector) {
		this->vector = vector;
	}

	void setVectorN(int n, int val) {
		vector[n] = val;
	}

	~TimeI(){
		delete[] vector;
		vector = NULL;
	}
};
 
class Timekeeper {
private:
      TimeI **table;
      int size;
public:
      Timekeeper() {
      		size = 0;
            table = new TimeI*[TABLE_SIZE];
            for (int i = 0; i < TABLE_SIZE; i++)
                  table[i] = NULL;
      }
 
      int* get(int key) {
      		if (table == NULL)
      			return NULL;

            for(int i = 0; i < TABLE_SIZE; i++){
            	if(table[i]->getKey() == key)
            		return table[i]->getVector();
            }

            return NULL;
      }

      TimeI* getN(int n){
      	if(n < TABLE_SIZE) 
      		return table[n];
      }
 
      void put(int key, int* vector) {
            int index = keyExists(key);
            if(index != -1) 
            	table[index]->setVector(vector);
            else{
            	table[size] = new TimeI(key, vector);
            	size ++;
            }
      }     

      int keyExists(int key) {

      	for(int i = 0; size != 0 && i < size; i++)
      		if(key == table[i]->getKey())
      			return i;
      	return -1;
      }

      int getSize() {
      	//if(table == NULL)
      	//	return 0;
      	return size;
      }

      //Mem leaks probable...
      void incrementTime(int key) {
      	int index = keyExists(key);

      	if(index != -1) {
      		int* temp = table[index]->getVector();
      		temp[index]++;
      		table[index]->setVector(temp);
      	}

      }

      void print() {
      	for(int i = 0; i < size; i++) {
      		printf("%i : ", table[i]->getKey());
      		for( int j = 0; j < size; j++) {
      			printf("%i ", (table[i]->getVector())[j]);
      		}
      		printf("\n");
      	}
      }
 
      ~Timekeeper() {
            for (int i = 0; i < TABLE_SIZE; i++)
                  if (table[i] != NULL)

                        delete table[i];
            delete[] table;
      }
};

class CompareTime {
public:
    bool operator()(msgNode l, msgNode r)
    {
		std::vector<int> lTime = l.timestamp;
		std::vector<int> rTime = r.timestamp;
		int i, diffL = 0, diffR = 0;

		for(int i = 0; i < lTime.size(); i++) {
			if(lTime[i] < rTime[i])
				diffL ++;
			else if (lTime[i] > rTime[i])
				diffR ++;
		}

		if(diffL > 0 && diffR == 0)
			return true;

		return false;
    }
};

list<node> msg_queue;
list<node> sent_msgs;

/* Josh's Section */
Timekeeper* TIMEKEEPER;
priority_queue<msgNode, vector<msgNode>, CompareTime> MESSAGES;
int MESSAGE_SLEEP_TIME = 100;

/* Tommy's Section */
#define MESSAGE_ID 1
#define NACK_ID 2
#define HEARTBEAT_ID 3
#define HEARTBEAT_INTERVAL 6
#define PROCESS_FAILURE -666


int curr_seq_num = 0;
int total_sequence = 0;
vector<int> prev_sequence;
vector<int> curr_sequence;
vector<int> mcast_mapping;
vector<int> curr_tmstmp;
int vector_len = 0;
int curr_i = 0;

void sort_mcast();
void send_nack(int curr, int next);


void multicast_init(void) {
	TIMEKEEPER = new Timekeeper();

    unicast_init();   
}

char* preProcessMessage(int key, const char* message){
	char member[10] = "";
	char timestamp[100] = "";
	int* vect = TIMEKEEPER->get(key);

	sprintf(member, "%d:", key);
	char temp[10];
	for(int i; i < TIMEKEEPER->getSize() - 1; i++) {
		sprintf(temp, "%d ",  vect[i]);
		strcat(timestamp, temp);
	}
	sprintf(temp, "%d:", vect[TIMEKEEPER->getSize()-1]);
	strcat(timestamp,temp);

	char* result = (char*) malloc(strlen(message) + strlen(member) + strlen(timestamp));

	strcat(result, member);
	strcat(result,timestamp);
	strcat(result,message);

	printf("%s\n", result );
	return result;
}

/*
	Checks to see if an event is old. If the timestamps are euqal or the event one is equal to some components of the current one and less than other components then the event time is considered old.
*/
bool isOldEvent(vector<int> curr, vector<int> event) {
	int diffL = 0, diffR = 0;

	for(int i = 0; i < curr.size(); i ++) {
		if(curr[i] > event[i])
			diffL++;
		else if (curr[i] < event[i])
			diffR++;
	}
	if((diffL + diffR) == 0)
		return true;
	else if (diffL > 0 && diffR == 0)
		return true;
	else
		return false;
}

void multicast(const char *message) {
	//TIMEKEEPER->print();
    int member = my_id;
    TIMEKEEPER->incrementTime(member);
    char* resultMessage = preProcessMessage(member, message);

    pthread_mutex_lock(&member_lock);
    for (int i = 0; i < mcast_num_members; i++) {
        usend(mcast_members[i], resultMessage, strlen(resultMessage)+1);
    }
    pthread_mutex_unlock(&member_lock);
    //TIMEKEEPER->print();
    deliver(my_id, message);
}

msgNode postProcessMessage(int source, char* msg, int len){
	int member;
	sscanf(strtok(msg,":"),"%i", &member);

	vector<int> timestamp;
	for(int i = 0; i < TIMEKEEPER->getSize()-1; i++){
		int temp = 0;
		sscanf(strtok(NULL," "),"%i", &temp);
		timestamp.push_back(temp);
	}
	
	int temp = 0;
	sscanf(strtok(NULL,":"),"%i", &temp);
	timestamp.push_back(temp);
	int dest = my_id;

	char* resultMsg = strtok(NULL, "\0");

	msgNode result;
	result.src = source;
	result.dest = dest;
	result.timestamp = timestamp;
	result.msg = resultMsg;

	return result;
}

vector<int> updateTimeStamp(vector<int> curr, vector<int> msg, int index) {
	vector<int> result;
	for(int i = 0; i < TIMEKEEPER->getSize(); i ++) {
		if( i == index)
			result.push_back(curr[i] + 1);
		else
			if(curr[i] < msg[i])
				result.push_back(msg[i]);
			else
				result.push_back(curr[i]);
	}

	return result;
}

/*
	Simple test to see if a message is a Chat Message or a Heartbeat message. Heatbeat messages never contain :'s and Chat messages always will.
*/
bool isChatMessage(const char* msg) {
	const void * test = strchr(msg, ':');
	return test != NULL;
}

void receive(int source, const char *message, int len) {
	//TIMEKEEPER->print();

	printf("%s\n", message);
    assert(message[len-1] == 0);

    if(isChatMessage(message)) {
		char * msgCpy = (char*) malloc(strlen(message));
	    strcpy(msgCpy, message);

	    msgNode entry = postProcessMessage(source, msgCpy, len);

	    pthread_mutex_lock(&member_lock);
		int* currTime = TIMEKEEPER->get(entry.dest);
		int index = TIMEKEEPER->keyExists(entry.dest);
		pthread_mutex_unlock(&member_lock);  

	    
	   	vector<int> currTimestamp (currTime, currTime + sizeof(currTime) / sizeof(int));

	    if(!isOldEvent(currTimestamp, entry.timestamp)) {
	    	deliver(entry.src, entry.msg);
	    	pthread_mutex_lock(&member_lock);
	    	TIMEKEEPER->put(entry.dest, updateTimeStamp(currTimestamp, entry.timestamp, index).data());
			    

		    for (int i = 0; i < mcast_num_members; i++) {
		     	if(i != index)
		        	usend(mcast_members[i], message, len);
		    }
		    pthread_mutex_unlock(&member_lock);
		    
		}
    }
}

void store(char * mess, int len, int n)
{
	node * curr = new node;

	curr->message = (char*) malloc(len * sizeof(char));
	strcpy(curr->message, mess);
	curr->length = len;
	curr->seq_num = n;

	sent_msgs.push_back(*curr);
}

void resend(int n, int source)
{
	if(sent_msgs.empty())
		return;

	for (list<int>::iterator it=sent_msgs.begin(); it != sent_msgs.end(); ++it)
    	if(it->seq_num == n)
    	{
    		usend(source, it->message, it->length);
    		return;
    	}
    	

}

void check_messages(int curr_pid, vector<int> vector_in, int * is_buffer, int * reject, int in_buffer)
{
	for(int i = 0; i < vector_len; i++)
	{
		if(prev_sequence[i] = PROCESS_FAILURE)
			continue;

		if(i != curr_pid)
		{
			if(vector_in[i] < curr_tmstmp[i])
			{
				*reject = 1;
				return;
			}
			if(curr_tmstmp[i] != vector_in[i])
			{
				*is_buffer = 1;

				if(!in_buffer)
					for(int j = curr_tmstmp[i] + 1; j < vector_in[i]; j++)
						send_nack(j, mcast_mapping[i]);
			}
		}

		else
		{
			if(vector_in[i] <= curr_tmstmp[i])
			{
				*reject = 1;
				return;
			}
			if(vector_in[i] - curr_tmstmp[i] > 1)
			{
				*is_buffer = 1;

				if(!in_buffer)
					for(int j = curr_tmstmp[i] + 1; j < vector_in[i]; j++)
						send_nack(j, mcast_mapping[i]);
			}
		}
	}
}

void send_nack(int curr, int next)
{
	char message[5];
	sprintf(message, "%d, %d", NACK_ID, curr);
	message[4] = 0;

	usend(next, message, 5);
}

void addnode(char * mess, vector<int> tmstmp, int src)
{
	node * curr = new node;

	curr->source = src;
	curr->timestamp = tmstmp;
	strcpy(curr->message, mess);

	//for(int i = 0; i < vector_len; i++)
	//	curr->timestamp[i] = tmstmp[i];

	msg_queue.push_back(*curr);
}

void removenode(int deliver)
{
	if(deliver)
	{
		deliver(msg_queue.front().source, msg_queue.front().message);
	}

	msg_queue.pop_front();
}

int getindex(int pid)
{
	for( int i = 0; i < mcast_mapping.size(); i++ )
		if(mcast_mapping[i] == pid)
			return i;

	return -1;
}

/**
	Auxilary function used to expand the vector when a new member is added to the chat.

	arr - vector to be expanded
	returns - new expanded vector with 0 as the newest entry
*/
int* expand_vector(int* arr, int count) {
	int* result = (int*) malloc(sizeof(int) * count);

	for(int i = 0; i < count - 1; i++ ) {
		result[i] = arr[i];
	}

	//delete[] arr;

	result[count - 1] = 0;

	return result;
}

/*
	Auxilary function for creating a new vector.

	returns - a new vector initialized to 0
*/
int* create_vector(int count) {
	int* result = (int*) malloc(sizeof(int) * count);
	for(int i = 0; i < count; i ++)
		result[i] = 0;

	return result;
}

void mcast_join(int member) {
	vector_len++;
	curr_tmstmp.resize(vector_len);

	// Joshua Code 
	int* tVector = create_vector(TIMEKEEPER->getSize() + 1);

	// Tommy Code
	for(int i = 0; i < vector_len; i++)
	{
		curr_tmstmp[i] = 0;
	}

	thread_mutex_lock(&member_lock);

	//Joshua Code
	for(int i = 0; i < TIMEKEEPER->getSize(); i++) {
		TimeI* temp = TIMEKEEPER->getN(i);
		int* tempVector = temp->getVector();
		temp->setVector(expand_vector(tempVector, TIMEKEEPER->getSize() + 1));
	}

	TIMEKEEPER->put(member, tVector);

	//Tommy Code
	if(total_sequence != mcast_num_members)
	{

		prev_sequence.resize(mcast_num_members, -1);
		curr_sequence.resize(mcast_num_members, 0);

		total_sequence = mcast_num_members;
	}

	pthread_mutex_unlock(&member_lock);

	sort_mcast();
}

void sort_mcast()
{
	mcast_mapping.resize(mcast_num_members, 0);

	for(int i = 0; i < mcast_num_members; i++)
	{
		mcast_mapping[i] = mcast_members[i];
	}

	sort(mcast_mapping.begin(), mcast_mapping.end());
}

void* heartbeat()
{
	int n = 0;
	int counter = 0;

	while(1)
	{
		counter++;
		pthread_mutex_lock(&member_lock);

		//Send heartbeats
		for(int i = 0; i < mcast_num_members; i++)
		{
			if(mcast_members[i] == my_id)
			{
				curr_i = i;
				continue;
			}
			curr_seq_num++;
			char message[5];
			sprintf(message, "%d, %d", HEARTBEAT_ID, total_sequence);
			message[4] = 0;
			usend(mcast_members[i], message, strlen(message) + 1);
		}

		pthread_mutex_unlock(&member_lock);

		if(counter % 4 == 0)
		{
			for(int i = 0; i < total_sequence; i++)
			{
				if( i != curr_i && curr_sequence[i] <= prev_sequence[i] && curr_sequence[i] != -1)
				{
					curr_sequence[i] = PROCESS_FAILURE; //failure
					pthread_mutex_lock(&member_lock);
					mcast_num_members--;
					pthread_mutex_unlock(&member_lock);
				}

				prev_sequence[i] = curr_sequence[i];
			}
		}

		sleep(HEARTBEAT_INTERVAL);
	}

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


/* Buffer message + timestamp to linked list
*/
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

