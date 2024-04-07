#include"threadpool.h"


ThreadPool* threadPoolCreate(int num,  int queueSize)
{
	ThreadPool* pool = malloc(sizeof(ThreadPool));
	do
	{
		if (pool == NULL)
		{
			printf("pool malloc error\n");
			break;
		}
		pool->pthreadIDs = malloc(num * sizeof(pthread_t));
		if (pool->pthreadIDs == NULL)
		{
			printf("pool_threads malloc error\n");
			break;
		}
		pool->taskQ = malloc(sizeof(Task) * queueSize);
		if (pool->taskQ == NULL)
		{
			printf("taskQ malloc error\n");
			break;
		}
		if (pthread_mutex_init(&(pool->poolMutex), NULL) != NULL ||
			pthread_cond_init(&(pool->notFull), NULL) != NULL ||
			pthread_cond_init(&(pool->notEmpty), NULL) != NULL)
		{
			printf("mutex cond init error\n");
			break;
		}
		pool->num = num;
		pool->queueCapacity = queueSize;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueTail = 0;
		for (int i = 0; i < num; i++)
		{
			pthread_create(&pool->pthreadIDs[i], NULL, worker, pool);
		}
		return pool;
	} while (0);
	if (pool && pool->pthreadIDs)
	{
		free(pool->pthreadIDs);
	}
	if (pool && pool->taskQ)
	{
		free(pool->taskQ);
	}
	if (pool)
	{
		free(pool);
	}
	return NULL;
}

void* worker(ThreadPool* pool)
{
	while (1)		//�����̣߳���ͣ�Ĵ��̳߳���ȡ������
	{
		pthread_mutex_lock(&pool->poolMutex);

		/*
		* ��Ⱥ����
		* ��add����notEmpty�ź�ʱ�����ܲ�ֻһ���߳��յ��źţ���ʱû�о�����
		* �����̻߳������pthread_cond_wait�ȴ����������ȵ��������ֹͣ����
		* ����ʱ��ȥ������������һ��worker�߳�ֱ���ͷŵ�������ʱʵ�ʲ�δ������
		*/
		while (pool->queueSize == 0)	
		{
			//���������Ϊ�գ����ͷ��������ȴ�������зǿ�
			//����ӹ�������ʱ���ᷢ��notEmpty�źţ����߹����̴߳�ʱ������
			pthread_cond_wait(&pool->notEmpty, &pool->poolMutex);
			
		}
		Task task = pool->taskQ[pool->queueFront];
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		pthread_cond_signal(&pool->notFull);		//����add�����в�����
		pthread_mutex_unlock(&pool->poolMutex);
		task.fun(task.arg);				//ִ������
	}
	return NULL;
}

void pthreadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg)
{
	pthread_mutex_lock(&pool->poolMutex);	//������worker�߳�ȡ�����໥Ӱ��
	while (pool->queueSize == pool->queueCapacity)
	{
		//��������ʱ���ȴ����в�����ʱ�����
		pthread_cond_wait(&pool->notFull, &pool->poolMutex);
	}
	pool->taskQ[pool->queueTail].fun = fun;
	pool->taskQ[pool->queueTail].arg = arg;
	pool->queueTail = (pool->queueTail + 1) % pool->queueCapacity;
	pool->queueSize++;
	//���߹����̣߳���ʱ�������������
	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->poolMutex);
}


