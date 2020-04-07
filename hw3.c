#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<signal.h>
#include<time.h>
#include<unistd.h>

#define SIGTIMER SIGRTMAX
#define Q_SIZE 500

static timer_t timer;

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Structure

typedef struct node_s node_t
struct node_s
{
	struct node_s *prev;
	struct node_s *next;
	int data;
};

typedef struct queue_s queue_t
struct queue_s{
	node_t *front_node;
	node_t *rear_node;
	int cnt;
};

typedef struct bp_s bp_t;
struct bp_s{
	queue_t *queue;
	pthread_mutex_t mutex;
	pthread_mutex_t mutex2;
	pthread_cond_t cond_data;
	pthread_cond_t cond_bp;
};

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ BP functions

bp_t* bp_init(){
	bp_t *bp = ( bp_t*)malloc( sizeof( bp_t));
	bp->queue = queue_init();
	if( bp->queue == NULL){
		printf("(bp) { Fail to init bp's queue }\n");
		return NULL;
	}
	
	int mutex_state = pthread_mutex_init( &bp->mutex, NULL);
	if( mutex_state){ perror( "\nmutex CREATE ERROR: "); exit( -1); }
	mutex_state = pthread_mutex_init( &bp->mutex2, NULL);
	if( mutex_state){ perror( "\nmutex2 CREATE ERROR: "); exit( -1); }

	int cond_state = pthread_cond_init( &bp->cond_data, NULL);
	if( cond_state){ perror( "\ncond_data CREATE ERROR: "); exit( -1); }
	cond_state = pthread_cond_init( &bp->cond_bp, NULL);
	if( cond_state){ perror( "\ncond_bp CREATE ERROR: "); exit( -1); }

	retturn bp;
}

void bp_destroy( bp_t *bp){
	if( bp->queue){
		queue_destory( bp->queue);	
	}
	pthread_mutex_destroy( &bp->mutex);
	pthread_mutex_destroy( &bp->mutex);
	pthread_cond_destroy( &bp->cond_data);
	pthread_cond_destroy( &bp->cond_bp);
	free( bp);
}

void* bp_produce_data( void* bp){
	int turn = 0, random = 0;
	queue_t* queue = (( queue_t*)( bp))->queue;
	while( 1){
		pthread_mutex_lock( &mutex);
		pthread_cond_wait( &cond_data, &mutex);
		if( turn % 2 == 0){ random = rand() % 30 + 60; }
		else{ random = rand() % 40 + 110; }
		queue = queue_insert_data( queue, random);
		turn++;
		if( turn == 10){ turn = 0; pthread_cond_signal( &cond_bp); }
		pthread_mutex_unlock( &mutex);
	}
}

void* bp_consume_data( void* bp){
	int turn = 0, avg = 0, cnt = 0, data = 0;
	struct tm *t; timer_t timer;
	queue_t* queue = ((queue_t*)( bp))->queue;
	while( 1){
		pthread_mutex_lock( &mutex);
		pthread_cond_wait( &cond_bp, &mutex);
		avg = 0; cnt = 0;
		while( queue->cnt != 0){
			data = queue->front_node->data;
			avg += data;
			//printf( "DATA: %d\n", data);
			queue_delete_data( queue);
			cnt++;
		}
		if(avg != 0){
			timer = time( NULL);
			t = localtime( &timer);
			char current_time[20]; char current_avg[20];
			sprintf( current_time, "%04d-%02d-%02d %02d:%02d:%02d", 
				t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, 
				t->tm_hour, t->tm_min, t->tm_sec);
			avg = avg / cnt;
			//printf( "AVG: %d\n", avg);
			if( turn % 2 == 0){ printf( "TIME: %s | Diastolic bp: %d\n", current_time, avg); }
			else{ printf( "TIME: %s | Systolic bp: %d\n", current_time,  avg); }
			sprintf( current_avg, "%d", avg);
			FILE* fp = fopen( "./record.txt", "a");
			fputs( current_time, fp); fputs( " | ", fp);
			fputs( current_avg, fp);
			fputs( "\n", fp);
			fclose( fp);
			turn++;
			if( turn == 10){ turn = 0;}
		}
		pthread_mutex_unlock( &mutex);
	}
}

void bp_process_data( bp_t *bp){
	if(( pthread_create( &da_thread, NULL, bp_produce_data, ( void*)( bp))) != 0){
		perror( "\nda_thread thread CREATE ERROR: "); exit( -1);
	}
	if(( pthread_create( &bp_thread, NULL, bp_consume_data, ( void*)( bp))) != 0){
		perror( "\nbp_thread thread CREATE ERROR: "); exit( -1);
	}
}

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Queue functions

queue_t* queue_init(){
	queue_t *queue = ( queue_t*)malloc( sizeof( queue_t));
	if( queue == NULL){
		printf("(queue) { Fail to init queue }\n");
		return NULL;
	}
	queue->front_node = NULL;
	queue->rear_node = NULL;
	queue->cnt = 0;
	return queue;
}

void queue_destroy( queue_t *queue){
	if( queue->front_node){
		free( queue->front_node);
	}
	if( queue->rear_node){
		free( queue->rear_node);
	}
	free( queue);
}

queue_t* queue_insert_data( queue_t *queue, int i){
	queue_t* temp_q = queue;
	node_t* temp_n = NULL;

	pthread_mutex_lock( &mutex2);
	temp_n = ( node_t*)malloc( sizeof( node_t));
	temp_n->data = i;
	if( temp_q->cnt == 0){
		temp_q->front_node = temp_n;
		temp_q->rear_node = NULL;
	}
	else{
		temp_q->rear_node->next = temp_n;
		temp_n->prev = temp_q->rear_node;
	}
	temp_q->rear_node = temp_n;
	temp_q->cnt++;
	pthread_mutex_unlock( &mutex2);
	return temp_q;
}

int queue_delete_data( queue_t* queue)
{
	pthread_mutex_lock( &mutex2);
	if( queue->cnt <= 0){ return -1; }
	queue_t* temp_q = queue;
	if( temp_q->cnt == 1){
		free( temp_q->front_node);
		temp_q->front_node = NULL;
		temp_q->rear_node = NULL;
		temp_q->cnt = 0;
	}
	else{
		temp_q->front_node = temp_q->front_node->next;
		free( temp_q->front_node->prev);
		temp_q->front_node->prev = NULL;
		temp_q->cnt--;
	}
	pthread_mutex_unlock( &mutex2);
	return 1;
}

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Signal Timer functions

void set_timer( int sig_no, int sec){
	struct sigevent sig_ev;
	struct itimerspec itval, oitval;

	sig_ev.sigev_notify = SIGEV_SIGNAL;
	sig_ev.sigev_signo = sig_no;
	sig_ev.sigev_value.sival_ptr = &timer;

	if(timer_create( CLOCK_REALTIME, &sig_ev, &timer) == 0){
		itval.it_value.tv_sec = 0;
		itval.it_value.tv_nsec = ( long)( sec / 10) * ( 1000000L);
		itval.it_interval.tv_sec = 0;
		itval.it_interval.tv_nsec = ( long)( sec / 10) * ( 1000000L);
		if( timer_settime( timer, 0, &itval, &oitval) != 0){
			perror( "\ntime_settime ERROR: "); exit( -1);
		}
	}
	else{ perror( "\ntimer_create ERROR: "); exit( -1); }
}

void signal_handler( int sig_no, siginfo_t *info, void* context){
	if( sig_no == SIGTIMER){
		pthread_cond_signal( &cond_data);
	}
	else if( sig_no == SIGINT){
		timer_delete( timer);
		perror( "\nCtrl + C: "); exit( 1);
	}
}

void set_signal(){
	struct sigaction sig_act;
	sigemptyset( &sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = signal_handler;
	if( sigaction( SIGTIMER, &sig_act, NULL) == -1){ perror( "\nSIGTIMER SIGACTION FAIL: "); exit( -1); }
	if( sigaction( SIGINT, &sig_act, NULL) == -1){ perror( "\nSIGINT SIGACTION FAIL: "); exit( -1); }
	set_timer( SIGTIMER, 100);
}



// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Main function

int main(){
	bp_t *bp = bp_init();
	pthread_t da_thread, bp_thread;

	bp_process_data( bp);
	
	set_signal();
	
	while( 1){ pause(); };
	pthread_join( da_thread, NULL); pthread_join( bp_thread, NULL);
	
	bp_destroy( bp);
}
