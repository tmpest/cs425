#include <string.h>
#include <assert.h>
#include <glib.h>

#include "mp1.h"

GHashTable TIMEKEEPER;
int MEMBER_COUNT;

/*Tommy Shiou*/

void multicast_init(void) {
    unicast_init();
    timekeeper = g_hash_table_new(NULL, NULL);
    MEMBER_COUNT = 0;
}

/* Basic multicast implementation */
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
	MEMBER_COUNT ++;

	int tKey = member;
	int* tVector = create_vector();

	int* keyList = g_hash_table_get_keys(TIMEKEEPER);
	for(int i = 0; i < MEMBER_COUNT -1; i++){
		int* currVector = g_hash_table_lookup(TIMEKEEPER, keyList[i]);
		int* resultVector = expand_vector(currVector);
		g_hash_table_replace(TIMEKEEPER, keyList[i], resultVector);
	}

	g_hash_table_insert(TIMEKEEPER, tKey, tVector);
}

/**
	Auxilary function used to expand the vector when a new member is added to the chat.

	arr - vector to be expanded
	returns - new expanded vector with 0 as the newest entry
*/
int* expand_vector(int* arr) {
	int* result = malloc(sizeOf(int) * MEMBER_COUNT);
	
	for(int i = 0; i < MEMBER_COUNT - 1; i++ ) {
		result[i] = arr[i];
	}
	
	delete[] arr;

	result[MEMBER_COUNT - 1] = 0;

	return result;
}

/*
	Auxilary function for creating a new vector.

	returns - a new vector initialized to 0
*/
int* create_vector() {
	int* result = malloc(sizeOf(int) * MEMBER_COUNT);
	for(int i = 0; i < MEMBER_COUNT; i ++)
		result[i] = 0;

	return result;
}
