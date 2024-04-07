#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include<pthread.h>
typedef struct Task
{
	void* (*fun)(void*);			//������
	void* arg;					//��������Ҫ�Ĳ���
} Task;
typedef struct ThreadPool
{
	//����
	Task* taskQ;				//�������
	int queueCapacity;			//�����������
	int queueSize;				//��ǰ������
	int queueFront;				//ͷָ��
	int queueTail;				//βָ��

	pthread_t managerID;		//�����߳�ID
	pthread_t* pthreadIDs;		//�����߳�ID����
	int num;					//�����߳���
	pthread_mutex_t poolMutex;	//�̳߳�������֤ͬʱֻ��һ���̲߳����̳߳�
	pthread_cond_t notFull;		//��������Ƿ���
	pthread_cond_t notEmpty;	//��������Ƿ��

}ThreadPool;
//��������ʼ���̳߳�
ThreadPool* threadPoolCreate(int num, int queueSize);
//�����߳�,���̳߳���ȡ������Ȼ��ͨ���ص�����ִ������
void* worker(ThreadPool* pool);
//��ӹ����߳�
void pthreadPoolAdd(ThreadPool* pool,void(*fun)(void*),void* arg);

#endif	//_THREADPOOL_H
