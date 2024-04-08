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
	int minNum;					//��С�߳���
	int maxNum;					//����߳���
	int liveNum;				//��ǰ�����߳�
	int busyNum;				//��ǰ�����е��߳�
	int exitNum;				//��Ҫ���ٵ��߳�
	pthread_mutex_t poolMutex;	//�̳߳�������֤ͬʱֻ��һ���̲߳����̳߳�
	pthread_mutex_t busyMutex;	//����busyNum��
	pthread_cond_t notFull;		//��������Ƿ���
	pthread_cond_t notEmpty;	//��������Ƿ��
	int shutdown;				//�����̳߳�,1����

}ThreadPool;
//��������ʼ���̳߳�
ThreadPool* threadPoolCreate(int minNum,int maxNum, int queueSize);
//�����߳�,���̳߳���ȡ������Ȼ��ͨ���ص�����ִ������
void* worker(ThreadPool* pool);
//��ӹ����߳�
void pthreadPoolAdd(ThreadPool* pool,void(*fun)(void*),void* arg);
//�������̣߳������̳߳صĴ���߳�������Ҫ̫�࣬Ҳ��Ҫ̫��
void* manager(ThreadPool* pool);
//�߳��˳�
void exitPthread(ThreadPool* pool);
//�̳߳�����
void pthreadPoolDestory(ThreadPool* pool);
#endif	//_THREADPOOL_H
