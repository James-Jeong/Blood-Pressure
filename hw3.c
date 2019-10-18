#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<signal.h>
#include<time.h>
#include<unistd.h>

#define SIGTIMER SIGRTMAX
#define Q_SIZE 500

pthread_mutex_t mutex, mutex2;
pthread_cond_t cond_data, cond_bp;
timer_t timer;

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Queue code

typedef struct Node
{
	struct Node* prev;
	struct Node* next;
	int data;
}N;

typedef struct Queue{
	N* front_Node;
	N* rear_Node;
	int cnt;
}Q;

typedef struct ARG{
	Q* my_q;
	int th_id;
}A;


void InitQ(Q* set_q)
{
	set_q->front_Node = NULL;
	set_q->rear_Node = NULL;
	set_q->cnt = 0;
}

Q* insertQ(Q* queue, int i)
{
	Q* temp_q = queue;
	N* temp_n = NULL;

	pthread_mutex_lock(&mutex2);
	temp_n = (N*)malloc(sizeof(N));
	temp_n->data = i;
	if(temp_q->cnt == 0){
		temp_q->front_Node = temp_n;
		temp_q->rear_Node = NULL;
	}
	else{
		temp_q->rear_Node->next = temp_n;
		temp_n->prev = temp_q->rear_Node;
	}
	temp_q->rear_Node = temp_n;
	temp_q->cnt++;
	pthread_mutex_unlock(&mutex2);
	return temp_q;
}

int deleteQ(Q* queue)
{
	pthread_mutex_lock(&mutex2);
	if(queue->cnt <= 0){ return -1; }
	Q* temp_q = queue;
	if(temp_q->cnt == 1){
		free(temp_q->front_Node);
		temp_q->front_Node = NULL;
		temp_q->rear_Node = NULL;
		temp_q->cnt = 0;
	}
	else{
		temp_q->front_Node = temp_q->front_Node->next;
		free(temp_q->front_Node->prev);
		temp_q->front_Node->prev = NULL;
		temp_q->cnt--;
	}
	pthread_mutex_unlock(&mutex2);
	return 1;
}

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Signal Timer code

void set_Timer(int sig_no, int sec){
	struct sigevent sig_ev;
	struct itimerspec itval, oitval;

	sig_ev.sigev_notify = SIGEV_SIGNAL;
	sig_ev.sigev_signo = sig_no;
	sig_ev.sigev_value.sival_ptr = &timer;

	if(timer_create(CLOCK_REALTIME, &sig_ev, &timer) == 0){
		itval.it_value.tv_sec = 0;
		itval.it_value.tv_nsec = (long)(sec / 10) * (1000000L);
		itval.it_interval.tv_sec = 0;
		itval.it_interval.tv_nsec = (long)(sec / 10) * (1000000L);
		if(timer_settime(timer, 0, &itval, &oitval) != 0){
			perror("\ntime_settime ERROR: "); exit(-1);
		}
	}
	else{ perror("\ntimer_create ERROR: "); exit(-1); }
}

void signal_Handler(int sig_no, siginfo_t *info, void* context){
	if(sig_no == SIGTIMER){
		pthread_cond_signal(&cond_data);
	}
	else if(sig_no == SIGINT){
		timer_delete(timer);
		perror("\nCtrl + C: "); exit(1);
	}
}

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Thread functions code

void* data_Acquisition(void* args){
	int turn = 0, random = 0;
	Q* queue = (Q*)(args);
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_data, &mutex);
		if(turn % 2 == 0){ random = rand() % 30 + 60; }
		else{ random = rand() % 40 + 110; }
		queue = insertQ(queue, random);
		turn++;
		if(turn == 10){ turn = 0; pthread_cond_signal(&cond_bp); }
		pthread_mutex_unlock(&mutex);
	}
}

void* bp_Processing(void* args){
	int turn = 0, avg = 0, cnt = 0, data = 0;
	struct tm *t; timer_t timer;
	Q* queue = (Q*)(args);
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_bp, &mutex);
		avg = 0; cnt = 0;
		while(queue->cnt != 0){
			data = queue->front_Node->data;
			avg += data;
			//printf("DATA: %d\n", data);
			deleteQ(queue);
			cnt++;
		}
		if(avg != 0){
			timer = time(NULL);
			t = localtime(&timer);
			char current_time[20]; char current_avg[20];
			sprintf(current_time, "%04d-%02d-%02d %02d:%02d:%02d", 
				t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, 
				t->tm_hour, t->tm_min, t->tm_sec);
			avg = avg / cnt;
			//printf("AVG: %d\n", avg);
			if(turn % 2 == 0){ printf("TIME: %s | Diastolic bp: %d\n", current_time, avg); }
			else{ printf("TIME: %s | Systolic bp: %d\n", current_time,  avg); }
			sprintf(current_avg, "%d", avg);
			FILE* fp = fopen("./record.txt", "a");
			fputs(current_time, fp); fputs(" | ", fp);
			fputs(current_avg, fp);
			fputs("\n", fp);
			fclose(fp);
			turn++;
			if(turn == 10){ turn = 0;}
		}
		pthread_mutex_unlock(&mutex);
	}
}

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

int main(){
	pthread_t da_thread, bp_thread;
	int mutex_state = pthread_mutex_init(&mutex, NULL);
	if(mutex_state){ perror("\nmutex CREATE ERROR: "); exit(-1); }
	mutex_state = pthread_mutex_init(&mutex2, NULL);
	if(mutex_state){ perror("\nmutex2 CREATE ERROR: "); exit(-1); }

	int cond_state = pthread_cond_init(&cond_data, NULL);
	if(cond_state){ perror("\ncond_data CREATE ERROR: "); exit(-1); }
	cond_state = pthread_cond_init(&cond_bp, NULL);
	if(cond_state){ perror("\ncond_bp CREATE ERROR: "); exit(-1); }

	Q* main_q = (Q*)malloc(sizeof(Q));
	InitQ(main_q);
	A* arg = (A*)malloc(sizeof(A));
	arg->my_q = main_q; arg->th_id = 1;
	if((pthread_create(&da_thread, NULL, data_Acquisition, (void*)(arg))) != 0){
		perror("\nda_thread thread CREATE ERROR: "); exit(-1);
	}
	if((pthread_create(&bp_thread, NULL, bp_Processing, (void*)(arg))) != 0){
		perror("\nbp_thread thread CREATE ERROR: "); exit(-1);
	}

	struct sigaction sig_act;
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = signal_Handler;
	if(sigaction(SIGTIMER, &sig_act, NULL) == -1){ perror("\nSIGTIMER SIGACTION FAIL: "); exit(-1); }
	if(sigaction(SIGINT, &sig_act, NULL) == -1){ perror("\nSIGINT SIGACTION FAIL: "); exit(-1); }

	set_Timer(SIGTIMER, 100);
	while(1){ pause(); };
	pthread_join(da_thread, NULL); pthread_join(bp_thread, NULL);
	pthread_mutex_destroy(&mutex);
}
