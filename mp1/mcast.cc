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

      // check if the key is in the table and if so returns its index
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

      //increments the time of the process given its key
      void incrementTime(int key) {
      	int index = keyExists(key);

      	if(index != -1) {
      		int* temp = table[index]->getVector();
      		temp[index]++;
      		table[index]->setVector(temp);
      	}

      }

      // Debug print statement to see the table from the console
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

// comparison function for priority queue
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

/*
	This takes the message and appends it with the origin process id seperated by a : then the timestamp of the origin process deliminated by spaces followed by a : and finnally the message.

	ex
	P_ID:0 0 0 0:Message Text Here
*/
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

/*
	Parses the message to get the information contained in the string. It returns an object containing the destination, source, timestamp, and message text.
*/
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

/*
	Updates the current timestamp given the event timestamp to match any components greater than the current timestamp component and additionally on the component for the respective process then it will simply increment it.
*/
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

    // Checks if the message is a chat message or a heartbeat
    if(isChatMessage(message)) {
		char * msgCpy = (char*) malloc(strlen(message));
	    strcpy(msgCpy, message);

	    msgNode entry = postProcessMessage(source, msgCpy, len);

	    pthread_mutex_lock(&member_lock);
		int* currTime = TIMEKEEPER->get(entry.dest);
		int index = TIMEKEEPER->keyExists(entry.dest);
		pthread_mutex_unlock(&member_lock);  

	    
	   	vector<int> currTimestamp (currTime, currTime + sizeof(currTime) / sizeof(int));

	   	// If the message is old then drop it
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

/*
	Store message
*/
void store(char * mess, int len, int n)
{
	node * curr = new node;

	curr->message = (char*) malloc(len * sizeof(char));
	strcpy(curr->message, mess);
	curr->length = len;
	curr->seq_num = n;

	sent_msgs.push_back(*curr);
}

/*
	Resend NACK (used when a message is out of order)
*/
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

/*
	Checks if a message should be delievered or buffered
	Sends NACK if message is not in causal order
*/
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

/*
	Sends a NACK message (used to send number of messages to source)
*/
void send_nack(int curr, int next)
{
	char message[5];
	sprintf(message, "%d, %d", NACK_ID, curr);
	message[4] = 0;

	usend(next, message, 5);
}

/*
	Adds a message with timestamp to the queue
*/
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

/*
	Removes a message from the queue
*/
void removenode(int deliver)
{
	if(deliver)
	{
		deliver(msg_queue.front().source, msg_queue.front().message);
	}

	msg_queue.pop_front();
}

/*
	Gets the index of a specific process on the processes listing
*/
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

/*
	Joins processes
*/
void mcast_join(int member) {
	vector_len++;
	curr_tmstmp.resize(vector_len);

	int* tVector = create_vector(TIMEKEEPER->getSize() + 1);

	for(int i = 0; i < vector_len; i++)
	{
		curr_tmstmp[i] = 0;
	}

	thread_mutex_lock(&member_lock);

	for(int i = 0; i < TIMEKEEPER->getSize(); i++) {
		TimeI* temp = TIMEKEEPER->getN(i);
		int* tempVector = temp->getVector();
		temp->setVector(expand_vector(tempVector, TIMEKEEPER->getSize() + 1));
	}

	TIMEKEEPER->put(member, tVector);

	// Updates sequences if a new member joins
	if(total_sequence != mcast_num_members)
	{

		prev_sequence.resize(mcast_num_members, -1);
		curr_sequence.resize(mcast_num_members, 0);

		total_sequence = mcast_num_members;
	}

	pthread_mutex_unlock(&member_lock);

	sort_mcast();
}

/* Sorts the procees listing */
void sort_mcast()
{
	mcast_mapping.resize(mcast_num_members, 0);

	for(int i = 0; i < mcast_num_members; i++)
	{
		mcast_mapping[i] = mcast_members[i];
	}

	sort(mcast_mapping.begin(), mcast_mapping.end());
}

/*
	Function for heartbeat process

	Sends a heartbeat every HEARTBEAT_INTERVAL

	Checks for failures
*/
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

		// Check for failure
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





