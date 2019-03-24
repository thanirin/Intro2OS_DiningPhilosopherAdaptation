#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define STATE_LEARNER 1
#define STATE_HELPER 0
#define STATE_FREE -1

sem_t *flag;
pthread_mutex_t mutex;
pthread_mutex_t simulMutex;
pthread_mutex_t serverMutex;

int* tachi_state;
int tachikoma;
int simul;
int maxTime;
int T;
int report;
int* tachi_totetime;
int* tachi_totereport;
long startTime;

void finish(int tachi, int left, int right) {
        tachi_state[tachi] = STATE_FREE;
        tachi_state[left] = STATE_FREE;
        tachi_state[right] = STATE_FREE;
}

void start(int tachi, int left, int right) {
	long start_time = clock() / CLOCKS_PER_SEC;
	long finish_time;
	long currTime;

	sem_wait(flag);
	pthread_mutex_lock(&simulMutex);
	currTime = (clock() / CLOCKS_PER_SEC) - startTime;
	printf("LEARN[%ld]: Learner{%d}, Helper1{%d}, Helper{%d}", currTime, tachi, left, right);
	sleep(maxTime);
	finish_time = (clock() / CLOCKS_PER_SEC) - start_time;
	tachi_totetime[tachi] += finish_time;
	currTime = (clock() / CLOCKS_PER_SEC) - startTime;
	printf("DONE[%ld]: Learner{%d}, Helper{%d}, Helper{%d}", currTime, tachi, left, right);
	pthread_mutex_unlock(&simulMutex);
	sem_post(flag);

	pthread_mutex_lock(&serverMutex);
	report++;
	tachi_totereport[tachi] ++;
	currTime = (clock() / CLOCKS_PER_SEC) - startTime;
	printf("UPDATE[%ld]: Learner{%d}, Report Number{%d}", currTime, tachi, report);

	finish(tachi, left, right);
}


void* learn(void* tachi) {
	int* num = tachi;
	tachi_state[*num] = STATE_FREE;
	int left_tachi = (*num + tachikoma - 1) % tachikoma;
	int right_tachi = (*num + 1) % tachikoma;

	while (1) {
		if (tachi_state[*num] == STATE_FREE) {
			pthread_mutex_lock(&mutex);
			if(tachi_state[left_tachi] != STATE_LEARNER && tachi_state[right_tachi] != STATE_LEARNER){
				tachi_state[*num] = STATE_LEARNER;
				tachi_state[left_tachi] = tachi_state[right_tachi] = STATE_HELPER;
			}
			pthread_mutex_unlock(&mutex);
		}
		if (tachi_state[*num]!=STATE_LEARNER) continue;
		else {
			start(*num, left_tachi, right_tachi);
		}
	}
}

void printOutput() {
	int i;
	for(i=0; i<tachikoma; i++) {
		printf("%d: Total time {%d}, Total number of reports {%d}", i, tachi_totetime[i], tachi_totereport[i]);
	}
	printf("MASATER: Total number of reports {%d}", report);
}

int main(int argc, char *argv[]) {
	if (argc!=5) {		// verify the correct number of arguments
		fprintf(stderr, "Please enter as following: ./main.out <int> <int> <int> <int>\n");
		exit(-1);
	}

	startTime = clock() / CLOCKS_PER_SEC;	// start the timer;

	tachikoma = atoi(argv[1]);		// number of tachikoma robots
	simul = atoi(argv[2]);			// number of simulators
	maxTime = atoi(argv[3]);		// maximum period of time that a simulator runs in one playing (in second)
	T = atoi(argv[4]);			// fixed period of time the whole learning will take (in second)
	tachi_state = (int*) malloc(tachikoma);		// allocate a fix size of tachikoma for tachi_state array
	tachi_totetime = (int*) malloc(tachikoma);	// allocate a fix size of tachikoma for tachi_totetime array
	tachi_totereport = (int*) malloc(tachikoma);	// allocate a fix size of tachikoma for tachi_totereport array
	report = 0;				// initialize the number of (all) report to 0

	/* initialize 0 for every tachi total time and report number */
	int i;
	for(i=0; i<tachikoma; i++) {
		tachi_totetime[0] = 0;
		tachi_totereport[0] = 0;
	}
	/* create mutex lock */
	pthread_mutex_init(&mutex, NULL);	// for general use
	pthread_mutex_init(&simulMutex, NULL);	// for checking simulator
	pthread_mutex_init(&serverMutex, NULL);	// for updating the server
	/* create pthreads a number of tachikoma (input) */
	pthread_t thread_id[tachikoma];
	/* create semaphores */
	flag = sem_open("/sixtynine", O_CREAT, 0666, simul);
	sem_unlink("/sixtynine");		// unlink to prevent the semaphore existing forever
	/* check if semaphores works */
	if (flag == SEM_FAILED) {
		printf("Open semphore failed..");
		return 1;
	}
	/* create tachikoma threads */
	for(i=0; i<tachikoma; i++) {
		pthread_create(&thread_id[i], NULL, learn, (void*) i);
	}

	sleep(T);				// run the program for T amount of time

	/* terminate tachikoma threads */
	for(i=0; i<tachikoma; i++) {
		pthread_cancel(thread_id[i]);
	}

	free(tachi_state);			// free the allocated memory
	free(tachi_totetime);			// free the allocated memory
	free(tachi_totereport);			// free the allocate memory
	sem_close(flag);			// close the semaphore
	sem_unlink("/sixtynine");
	return 0;
}
