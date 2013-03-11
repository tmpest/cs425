#include "mp1.h"

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

int _INITIAL_QUEUE_SIZE = 5;

int* queue; = int[_INITIAL_QUEUE_SIZE];

int maxCapacity;
int size;

void initQueue() {
	queue = malloc 
}

int compapre(int* left, int* right) {
	int i, diffL = 0, diffR = 0;

	for (i = 0; i < mcast_num_members) {
		if (left[i] < right[i])
			diffL++;
		else if (left[i] > right[i])
			diffR++;
	}

	if ((diffL > 0 && diffR > 0) || diffL == diffR)
		return 0;
	else if (diffL > 0)
		return -1;
	else
		return 1;
}