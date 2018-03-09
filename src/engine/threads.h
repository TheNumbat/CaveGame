
#pragma once

template<typename E>
struct atomic_enum {

	void set(E val);
	E get();

private:
	u64 value = (u64)E::none;
	friend void make_meta_info();
};

// TODO(max): work-stealing thread local queues to minimize locking

template<typename T>
struct future {
private:
	T val;
	platform_semaphore sem;
	platform_mutex mut;
	friend void make_meta_info();

public:
	static future make(); 
	void destroy();

	T wait();
	void set(T val);
};

template<>
struct NOREFLECT future<void> {};

template<typename T>
using job_work = T(*)(void*);

struct super_job {
	float priority 		= 0.0f;
	void* data 	  		= null;
	virtual void do_work() = 0;
};

bool gt(super_job* l, super_job* r);

template<typename T>
struct job : super_job {
	future<T>* future = null;
	job_work<T> work  = null;
	void do_work() { future->set(work(data)); }
};

template<>
struct NOREFLECT job<void> : super_job {
	job_work<void> work;
	void do_work() { work(data); }
};

struct worker_param {
	locking_heap<super_job*,gt>* job_queue 	= null;
	platform_semaphore* jobs_semaphore 		= null;
	allocator* alloc 				   		= null;
	bool online			   	       			= false;
};

struct threadpool {
	i32 num_threads 	= 0;
	bool online    		= false;

	locking_heap<super_job*,gt> jobs;		

	array<platform_thread> 	threads;
	array<worker_param> 	worker_data;
	
	platform_semaphore	   jobs_semaphore;
	allocator* 			   alloc;

///////////////////////////////////////////////////////////////////////////////

	static threadpool make(i32 num_threads_ = 0);
	static threadpool make(allocator* a, i32 num_threads_ = 0);
	void destroy();
	
	template<typename T> void queue_job(future<T>* fut, job_work<T> work, void* data, float prirority = 0.0f);
	void queue_job(job_work<void> work, void* data, float prirority = 0.0f);
	
	void stop_all();
	void start_all();

	void renew_priorities(float (*eval)(super_job*,void*), void* param);
};

i32 worker(void* data_);
