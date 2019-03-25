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

struct timespec start;
int *learningTime;
long timeUsed=0;
sem_t *flag;
pthread_mutex_t mutex;

int N;
int M;
int E;
int T;

uint64_t getTime() {
	struct timespec current;
	clock_gettime(CLOCK_MONOTONIC_RAW, &current);
	return (current.tv_sec - start.tv_sec) * 1000000 + (current.tv_nsec - start.tv_nsec) / 1000;
}

void *tachikoma (void *arg) {
	int tid = *(int*) arg;
	long t;

	while(timeUsed < T*1000000) {
		timeUsed += getTime();
		printf("time used: %ld\n", timeUsed);
		pthread_mutex_lock(&mutex);
		uint64_t time = getTime();
		printf("It is %d thread. Time = %ld\n", tid, time);
		sleep(2);
		printf("done!\n");
		pthread_mutex_unlock(&mutex);
	}
}

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

	learningTime = (int*)malloc(sizeof(int) * N);

	flag = sem_open("/semmie", O_CREAT, 0666, M);
	sem_unlink("/semmie");

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	int i;
	for(i=0; i<N; i++) {
		tachikomaId[i] = i;
		learningTime[i] = 0;
		pthread_create(&tachi_threads[i], NULL, tachikoma, (void*)&tachikomaId[i]);
	}

	sleep(T);

	for (i=0; i<N; i++) {
		tachikomaId[i] = -1;
	}

	for(i = 0; i<N; i++) {
		pthread_join(tachi_threads[i], NULL);
	}

	printf("=============================================\nTime used = %ld\n", getTime());

	free(learningTime);
	sem_close(flag);
	sem_unlink("/semmie");

	return 0;
}
