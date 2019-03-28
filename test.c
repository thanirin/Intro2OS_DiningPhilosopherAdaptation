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
#include <sched.h>
#include <errno.h>
// #include <evos/tasks.h>

#define STATE_LEARN 0
#define STATE_HELPER 1
#define STATE_FREE -1

struct timespec start;
sem_t *flag;
pthread_mutex_t mutex;
sem_t simulMutex;
sem_t simulLock[100];
pthread_mutex_t serverMutex;
pthread_t* tachi_threads;

// typedef struct ticket_lock {
// 	pthread_cond_t cond;
// 	pthread_mutex_t mutex;
// 	unsigned long queue_head, queue_tail;
// } ticket_lock_t;
//
// #define TICKET_LOCK_INITIALIZER { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }

int N;
int M;
int E;
int T;

long* learningTime;
int* totalReport;
int report;

int *tachi_state;

long timeUsed = 0;

// void ticket_lock(ticket_lock_t *ticket) {
// 	unsigned long queue_me;
// 	pthread_mutex_lock(&ticket->mutex);
// 	queue_me = ticket->queue_tail++;
// 	while(queue_me != ticket->queue_head) {
// 		pthread_cond_wait(&ticket->cond, &ticket->mutex);
// 	}
// 	pthread_mutex_unlock(&ticket->mutex);
// }
//
// void ticket_unlock(ticket_lock_t *ticket) {
// 	pthread_mutex_lock(&ticket->mutex);
// 	ticket->queue_head++;
// 	pthread_cond_broadcast(&ticket->cond);
// 	pthread_mutex_unlock(&ticket->mutex);
// }

// void set_priority() {
// 	pthread_t this_thread = pthread_self();
// 	int policy = 0;
// 	struct sched_param params;
// 	int ret;
//
// 	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
// 	ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
// 	if(ret != 0 ) {
// 		printf("Couldn't retrieve real-time shceduling parameters..\n");
// 		return;
// 	}
// 	if(policy != SCHED_FIFO) {
// 		printf("Scheduling is NOT SCHED_FIFO..\n");
// 	}
// }
	//
	// void high_priority(pthread_t thread) {
	// 	struct sched_param params;
	// 	// int policy = get_sched_policy();
	// 	// params.sched_priority = sched_get_priority_max(SCHED_OTHER);
	// 	// printf("%d\n", params.sched_priority);
	// 	params.sched_priority = 10;
	// 	printf("%d\n", params.sched_priority);
	//
	// 	int s = pthread_setschedparam(thread, SCHED_FIFO, &params);
	// 	if(s!=0) {
	// 		if(s==EPERM) {
	// 			printf("EPERM\n");
	// 		}
	// 		else if(s==EINVAL) {
	// 			printf("EINVAL\n");
	// 		}
	// 		else {
	// 			printf("none\n");
	// 		}
	// 	}
	// }
	//
	// void normal_priority(pthread_t thread) {
	// 	struct sched_param params;
	// 	// int policy = get_sched_policy();
	// 	params.sched_priority = sched_get_priority_min(SCHED_OTHER);
	//
	// 	int s = pthread_setschedparam(thread, SCHED_OTHER, &params);
	// 	if(s!=0) {
	// 		printf("norm pri ERROR! s = %d\n", s);
	// 	}
	// }

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

	// pthread_t tachi_threads[N];
	tachi_threads = calloc(sizeof(*tachi_threads), N);
	pthread_attr_t attr;
	struct sched_param param;
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

	pthread_attr_init(&attr);
	pthread_attr_getschedparam(&attr, &param);
	(param.sched_priority) ++;
	pthread_attr_getschedparam(&attr, &param);

	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	int i;
	for(i=0; i<N; i++) {
		tachi_state[i] = STATE_FREE;
        	totalReport[i] = 0;
        	tachikomaId[i] = i;
        	learningTime[i] = 0;
		sem_init(&simulLock[i], 0, 0);
	}
	int issuccessful;
	for(i=0; i<N; i++) {
		issuccessful = pthread_create(&tachi_threads[i], &attr, tachikoma, (void*)&tachikomaId[i]);
		if (issuccessful < 0) {
			printf("Pthread %d failed..\n", i);
		}
	}

	sleep(T);

	if(getTime() > T*1000000) {
		for(i=0; i<N; i++) {
			tachikomaId[i] = -1;
		}
	}

	// for(i = 0; i<N; i++) {
	// 	pthread_join(tachi_threads[i], NULL);
	// 	// printf("already joined bruh\n");
	// }

	for(i=0; i<N; i++) {
		printf("%d: %ld, %d\n", i, learningTime[i] / 1000000, totalReport[i]);
	}

	printf("MASTER: %d\n", report);
	printf("Time used %ld\n", getTime());

	free(tachi_state);
	free(totalReport);
	free(learningTime);
	sem_close(flag);
	sem_unlink("/semmie");

	return 0;
}

void *tachikoma (void *arg) {
	int* tachi = (int*) arg;
	int tid = *(int*) arg;
	int left = (tid + N - 1) % N;
	int right = (tid + 1) % N;
	while(*tachi > 0) {
		learn(tid);
		sleep(0);
		done(tid);
		//timeUsed = getTime();
		//printf(">>>>>>>>>>>> %d\n", (int)*tachi);
	}
	// if (*tachi < 0) {
	// 	printf("WE HAVE REACHED THE END\nAnd tid is %d\n", tid);
	// }
}

void learn(int tid) {
	//tachi_state[tid] = STATE_FREE;
	check(tid);
	sem_wait(&simulLock[tid]);
	// if(test == -1) printf("it's at learn\n");
}

void check(int tid) {
	int left = (tid + N - 1) % N;
	int right = (tid + 1) % N;
	struct timespec curr;

	if(tachi_state[tid] == STATE_FREE) {
		if(learningTime[tid] > learningTime[right] || learningTime[tid] > learningTime[left]) sleep(1);
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

			// high_priority(tachi_threads[left]);
			// high_priority(tachi_threads[right]);
			// normal_priority(pthread_self());

    	sem_post(flag);
			sem_post(&simulLock[tid]);
			pthread_mutex_lock(&serverMutex);
		  report++;
		  totalReport[tid] += 1;
			printf("UPDATE[%ld]: %d, %d\n", getTime(), tid, report);
			pthread_mutex_unlock(&serverMutex);

		}
		else {
			pthread_mutex_unlock(&mutex);
		}

	}
	// if(test == -1) printf("it's at check\n");

}

void done(int tid) {
	int left = (tid + N - 1) % N;
	int right = (tid + 1) % N;

	sem_wait(&simulMutex);
	// if(isgood!=0) printf("sem_wait err at done tid %d\n", tid);
	tachi_state[tid] = STATE_FREE;
	sem_post(&simulMutex);
	// if(isgood!=0) printf("sem_post err at done tid %d\n", tid);

	check(left);
	check(right);

	// if(test == -1) printf("it's at done\n");
}
