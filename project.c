#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define STATE_LEARNER 1
#define STATE_HELPER 2

sem_t *flag;
int* tachi_state;

void learn(void tachi) {
	
}

void finish(int tachi) {

}

void* learn(void* tachi) {
	while (1) {
		int* num = tachi;
		sleep(1);
		learn(*num);
		sleep(0);
		finish(*num);
	}
}

int main(int argc, char *agrv[]) {
	if (argc!=5) {		// verify the correct number of arguments
		fprintf(stderr, "Please enter as following: ./main.out <int> <int> <int> <int>\n");
		exit(-1);
	}

	int tachikoma = atoi(argv[1]);		// number of tachikoma robots
	int simul = atoi(argv[2]);		// number of simulators
	int maxTime = atoi(argv[3]);		// maximum period of time that a simulator runs in one playing (in second)
	int T = ator(argv[4]);			// fixed period of time the whole learning will take (in second)
	tachi_state = (int*) malloc(tachikoma);	// set a fix size for tachi_state array

	/* create pthreads a number of tachikoma (input) */
	pthread_t thread_id[tachikoma];
	/* create semaphores */
	flag = sem_open("/sixtynine", O_CREAT, 0666, simul);
	/* check if semaphores works */
	if (flag == SEM_FAILED) {
		printf("Open semphore failed..");
		return 1;
	}
	/* create tachikoma processes */
	int i;
	for(i=0; i<tachikoma; i++) {
		pthread_create(&thread_id[i], NULL, learn, (void*) i);
		tachi_state[i] = STATE_LEARNER;
	}

	sleep(T*1000);

	/* terminate robot */
	for(i=0; i<tachikoma; i++) {
		pthread_cancel(thread_id[i], NULL);
	}

	return 0;
}
