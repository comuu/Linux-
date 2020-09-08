
typedef struct{
	void (*function)(void*);
	void *arg;
}threadpool_task_t;
struct threadpool_t{
	pthread_mutex_t lock;
	pthread_mutex_t thread_counter;
	pthread_cond_t queue_not_full;
	pthread_cond_t queue_not_empty;
	
	pthread_t *threads;
	pthread_t adjust_tid;
	threadpool_task_t *task_queue;
	
	int min_thr_num;
	int max_thr_num;
	int live_thr_num;
	int busy_thr_num;
	int wait_exit_thr_num;
	
	int queue_front;
	int queue_rear;
	int queue_size;
	int queue_max_size;
	
	int shutdown;
}