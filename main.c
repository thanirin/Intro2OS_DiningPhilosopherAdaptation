#include <sys/time.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define STATE_LEARN 0
#define STATE_HELPER 1
#define STATE_FREE -1

struct timespec start;
sem_t *flag;
pthread_mutex_t mutex;
sem_t simulMutex;
sem_t simulLock[100];
pthread_mutex_t serverMutex;

int N;
int M;
int E;
int T;

long* learningTime;
int* totalReport;
int report;

int *tachi_state;

long timeUsed = 0;

uint64_t getTime() {
	struct timespec current;
	clock_gettime(CLOCK_MONOTONIC_RAW, &current);
	return (current.tv_sec - start.tv_sec) * 1000000 + (current.tv_nsec - start.tv_nsec) / 1000;
}

uint64_t calTime(struct timespec time) {
	struct timespec current;
	clock_gettime(CLOCK_MONOTONIC_RAW, &current);
	return (current.tv_sec - time.tv_sec) * 1000000 + (current.tv_nsec - time.tv_nsec) / 1000;
}

void *tachikoma (void *arg);
void check(int tid);
void done(int tid);
void learn(int tid);

int main (int argc, char **argv) {
	if (argc!=5) {
		fprintf(stderr, "Please enter as following: ./main.out <int> <int> <int> <int>\n");
		exit(-1);
	}

	N = atoi(argv[1]);
	M = atoi(argv[2]);
	E = atoi(argv[3]);
	T = atoi(argv[4]);

	pthread_t tachi_threads[N];
	int tachikomaId[N];
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&serverMutex, NULL);
    	sem_init(&simulMutex, 0, 1);

	learningTime = (long*)malloc(sizeof(long) * N);
	totalReport = (int*)malloc(sizeof(int) * N);
	tachi_state = (int*)malloc(sizeof(int) * N);
	report = 0;

	flag = sem_open("/semmie", O_CREAT, 0666, M);
	sem_unlink("/semmie");
	if(flag == SEM_FAILED) {
		printf("Open semaphore failed..\n");
		return 1;
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	int i;
	for(i=0; i<N; i++) {
		tachi_state[i] = STATE_FREE;
        	totalReport[i] = 0;
        	tachikomaId[i] = i;
        	learningTime[i] = 0;
		sem_init(&simulLock[i], 0, 0);
	}
	int check;
	int j=0;
	while(getTime() < T*1000000) {
		for(i=0; i<N; i++) {
			int a = j%N;
			check = pthread_create(&tachi_threads[a], NULL, tachikoma, (void*)&tachikomaId[a]);
			if (check < 0) {
				printf("Pthread %d failed..\n", i);
			}
			j++;
		}
		j++;
	}

	for(i = 0; i<N; i++) {
		pthread_join(tachi_threads[i], NULL);
	}

	for(i=0; i<N; i++) {
		printf("%d: %ld, %d\n", i, learningTime[i] / 1000000, totalReport[i]);
	}

	printf("MASTER: %d\n", report);

	free(tachi_state);
	free(totalReport);
	free(learningTime);
	sem_close(flag);
	sem_unlink("/semmie");

	return 0;
}

void *tachikoma (void *arg) {
	int tid = *(int*) arg;
	int left = (tid + N - 1) % N;
	int right = (tid + 1) % N;
	learn(tid);
	sleep(0);
	done(tid);
	timeUsed = getTime();
}

void learn(int tid) {
	check(tid);
	sem_wait(&simulLock[tid]);

}

void check(int tid) {
	int left = (tid + N - 1) % N;
	int right = (tid + 1) % N;
	struct timespec curr;

	if(tachi_state[tid] == STATE_FREE) {
		pthread_mutex_lock(&mutex);
		if(tachi_state[left] != STATE_LEARN && tachi_state[right] != STATE_LEARN) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &curr);
			tachi_state[tid] = STATE_LEARN;
			pthread_mutex_unlock(&mutex);

			sem_wait(flag);

   			printf("LEARN[%ld]: %d, %d, %d\n", getTime(), tid, left, right);
			srand(time(NULL));
			int ran_time = 1 + rand() % E;
			sleep(ran_time);
			learningTime[tid] += calTime(curr);
			printf("DONE[%ld]: %d, %d, %d\n", getTime(), tid, left, right);

    			sem_post(flag);
			sem_post(&simulLock[tid]);
    			pthread_mutex_lock(&serverMutex);
		   	report++;
		    	totalReport[tid] += 1;
			printf("UPDATE[%ld]: %d, %d\n", getTime(), tid, report);
			pthread_mutex_unlock(&serverMutex);

		}
		else pthread_mutex_unlock(&mutex);

	}

}

void done(int tid) {
	int left = (tid + N - 1) % N;
	int right = (tid + 1) % N;

	sem_wait(&simulMutex);
	tachi_state[tid] = STATE_FREE;
	sem_post(&simulMutex);

	check(left);
	check(right);
}
