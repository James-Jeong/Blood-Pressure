#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<signal.h>
#include<time.h>
#include<unistd.h>

#define SIGTIMER SIGRTMAX
#define Q_SIZE 500

static timer_t timer; /**< Timer variable (static) */

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Structure

/**
  * @brief Node structure
  * @member struct node_s* prev
  * @member struct node_s* next
  * @member int data
  */
typedef struct node_s node_t
struct node_s
{
	struct node_s *prev; /**< Previous Node */
	struct node_s *next; /**< Next Node */
	int data; /**< Node's current data */
};

/**
  * @brief Queue structure
  * @member node_t* front_node
  * @member node_t* rear_node
  * @member int cnt
  */
typedef struct queue_s queue_t
struct queue_s{
	node_t *front_node; /**< Front Node of queue */
	node_t *rear_node; /**< Rear Node of queue */
	int cnt; /**< Total count of Node in queue */
};

/**
  * @brief BP structure
  * @member queue_t* queue
  * @member pthread_mutex_t mutex;
  * @member pthread_mutex_t mutex2;
  * @member pthread_cond_t cond_data;
  * @member pthread_cond_t cond_bp;
  */
typedef struct bp_s bp_t;
struct bp_s{
	queue_t *queue; /**< A queue of BP */
	pthread_mutex_t mutex; /**< Mutex for BP's data processing */
	pthread_mutex_t mutex2; /**< Mutex2 for queue's work */
	pthread_cond_t cond_data; /**< Cond_data for BP's data processing */
	pthread_cond_t cond_bp; /**< Cond_bp for BP's data processing */
};

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ BP functions

/**
  * @brief Initialize a new BP object
  * @return bp_t*
  */
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

/**
  * @brief Destroy the BP object
  * @param bp_t* bp
  */
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
/**
  * @brief Bp produces data & insert the data to queue
  * @param void* bp
  * @return void*
  */
void* bp_produce_data( void* bp){
	int turn = 0, random = 0;
	queue_t* queue = (( queue_t*)( bp))->queue;
	while( 1){
		pthread_mutex_lock( &bp->mutex);
		pthread_cond_wait( &bp->cond_data, &bp->mutex);
		if( turn % 2 == 0){ random = rand() % 30 + 60; }
		else{ random = rand() % 40 + 110; }
		queue = queue_insert_data( queue, random);
		turn++;
		if( turn == 10){ turn = 0; pthread_cond_signal( &bp->cond_bp); }
		pthread_mutex_unlock( &bp->mutex);
	}
}

/**
  * @brief Bp consumes data & delete the data from queue if the queue is full
  * @param void* bp
  * @return void*
  */
void* bp_consume_data( void* bp){
	int turn = 0, avg = 0, cnt = 0, data = 0;
	struct tm *t; timer_t timer;
	queue_t* queue = ((queue_t*)( bp))->queue;
	while( 1){
		pthread_mutex_lock( &bp->mutex);
		pthread_cond_wait( &bp->cond_bp, &bp->mutex);
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
		pthread_mutex_unlock( &bp->mutex);
	}
}

/**
  * @brief BP creates two threads for handling data
  * @param bp_t* bp
  */
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

/**
  * @brief Initialize a new queue object
  * @return queue_t*
  */
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

/**
  * @brief Destroy the queue object
  * @param queue_t* queue
  */
void queue_destroy( queue_t *queue){
	if( queue->front_node){
		free( queue->front_node);
	}
	if( queue->rear_node){
		free( queue->rear_node);
	}
	free( queue);
}

/**
  * @brief Insert data to queue
  * @param queue_t* queue
  * @param int i
  * @return queue_t*
  */
queue_t* queue_insert_data( queue_t *queue, int i){
	queue_t* temp_q = queue;
	node_t* temp_n = NULL;

	pthread_mutex_lock( &bp->mutex2);
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
	pthread_mutex_unlock( &bp->mutex2);
	return temp_q;
}

/**
  * @brief Delete data from queue
  * @param queue_t* queue
  * @return int
  */
int queue_delete_data( queue_t* queue)
{
	pthread_mutex_lock( &bp->mutex2);
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
	pthread_mutex_unlock( &bp->mutex2);
	return 1;
}

// @@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@@@@@@@@@@@@@@@@@@@@@ Signal Timer functions

/**
  * @brief Set timer's information
  * @param int sig_no
  * @param int sec
  */
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

/**
  * @brief Set signal handler
  * @param int sig_no
  * @param siginfo_t* info
  * @param void* context
  */
void signal_handler( int sig_no, siginfo_t *info, void* context){
	if( sig_no == SIGTIMER){
		pthread_cond_signal( &bp->cond_data);
	}
	else if( sig_no == SIGINT){
		timer_delete( timer);
		perror( "\nCtrl + C: "); exit( 1);
	}
}

/**
  * @brief Manage all signal information
  */
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

/**
  * @brief Main function
  * @      Create BP object & Handle data
  * @return int
  */
int main(){
	bp_t *bp = bp_init();
	pthread_t da_thread, bp_thread;

	bp_process_data( bp);
	
	set_signal();
	
	while( 1){ pause(); };
	pthread_join( da_thread, NULL); pthread_join( bp_thread, NULL);
	
	bp_destroy( bp);
}
