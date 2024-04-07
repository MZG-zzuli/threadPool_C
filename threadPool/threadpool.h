#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include<pthread.h>
typedef struct Task
{
	void* (*fun)(void*);			//任务函数
	void* arg;					//任务函数需要的参数
} Task;
typedef struct ThreadPool
{
	//任务
	Task* taskQ;				//任务队列
	int queueCapacity;			//任务队列容量
	int queueSize;				//当前任务数
	int queueFront;				//头指针
	int queueTail;				//尾指针

	pthread_t managerID;		//管理线程ID
	pthread_t* pthreadIDs;		//工作线程ID数组
	int num;					//工作线程数
	pthread_mutex_t poolMutex;	//线程池锁，保证同时只有一个线程操作线程池
	pthread_cond_t notFull;		//任务队列是否满
	pthread_cond_t notEmpty;	//任务队列是否空

}ThreadPool;
//创建并初始化线程池
ThreadPool* threadPoolCreate(int num, int queueSize);
//工作线程,从线程池中取出任务，然后通过回调函数执行任务
void* worker(ThreadPool* pool);
//添加工作线程
void pthreadPoolAdd(ThreadPool* pool,void(*fun)(void*),void* arg);

#endif	//_THREADPOOL_H
