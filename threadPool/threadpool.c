#include"threadpool.h"


ThreadPool* threadPoolCreate(int minNum,int maxNum,  int queueSize)
{
	ThreadPool* pool = malloc(sizeof(ThreadPool));
	do
	{
		if (pool == NULL)
		{
			printf("pool malloc error\n");
			break;
		}
		pool->pthreadIDs = malloc(maxNum * sizeof(pthread_t));
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
			pthread_mutex_init(&(pool->busyMutex), NULL) != NULL ||
			pthread_cond_init(&(pool->notFull), NULL) != NULL ||
			pthread_cond_init(&(pool->notEmpty), NULL) != NULL)
		{
			printf("mutex cond init error\n");
			break;
		}
		pool->minNum = minNum;
		pool->maxNum = maxNum;
		pool->queueCapacity = queueSize;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueTail = 0;
		pool->liveNum = minNum;
		pool->shutdown = 0;
		pthread_create(&pool->managerID, NULL, manager, pool);
		for (int i = 0; i < minNum; i++)
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
		while (pool->queueSize == 0&&pool->shutdown==0)	
		{
			//���������Ϊ�գ����ͷ��������ȴ�������зǿ�
			//����ӹ�������ʱ���ᷢ��notEmpty�źţ����߹����̴߳�ʱ������
			pthread_cond_wait(&pool->notEmpty, &pool->poolMutex);
			if (pool->exitNum > 0)
			{
				//printf("xhhh\n");
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					//�߳��˳�

					pool->liveNum--;
					pthread_mutex_unlock(&pool->poolMutex);
					//printf("yxh\n");
					exitPthread(pool);
				}
			}
		}
		if (pool->shutdown == 1)
		{
			//�̳߳عرգ��߳��˳����ɸ����̻�����Դ
			pthread_mutex_unlock(&pool->poolMutex);
			pthread_exit(NULL);
		}
		Task task = pool->taskQ[pool->queueFront];
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		pthread_cond_signal(&pool->notFull);		//����add�����в�����
		pthread_mutex_unlock(&pool->poolMutex);
		pthread_mutex_lock(&pool->busyMutex);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->busyMutex);
		task.fun(task.arg);				//ִ������
		pthread_mutex_lock(&pool->busyMutex);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->busyMutex);
	}
	return NULL;
}


void pthreadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg)
{
	pthread_mutex_lock(&pool->poolMutex);	//������worker�߳�ȡ�����໥Ӱ��
	while (pool->queueSize == pool->queueCapacity&&pool->shutdown==0)
	{
		//��������ʱ���ȴ����в�����ʱ�����
		pthread_cond_wait(&pool->notFull, &pool->poolMutex);
	}
	if (pool->shutdown)
	{
		pthread_mutex_unlock(&pool->poolMutex);
		pthread_exit(NULL);
	}
	pool->taskQ[pool->queueTail].fun = fun;
	pool->taskQ[pool->queueTail].arg = arg;
	pool->queueTail = (pool->queueTail + 1) % pool->queueCapacity;
	pool->queueSize++;
	//���߹����̣߳���ʱ�������������
	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->poolMutex);
}

void* manager(ThreadPool* pool)
{
	while (pool->shutdown==0)
	{

		//����
		if (pool->busyNum * 2 < pool->liveNum && pool->liveNum>pool->minNum)
		{
			pool->exitNum++;
			pthread_cond_signal(&pool->notEmpty);
		}
		//����
		if (pool->queueSize > pool->liveNum * 2 && pool->liveNum < pool->maxNum)
		{
			for (int i = 0; i < pool->maxNum; i++)
			{
				if (pool->pthreadIDs[i] == 0)
				{
					pthread_mutex_lock(&pool->poolMutex);
					pool->liveNum++;
					pthread_create(&pool->pthreadIDs[i], NULL, worker, pool);
					pthread_mutex_unlock(&pool->poolMutex);
					break;
				}
			}
		}
		sleep(1);
	}
	return NULL;
}

void exitPthread(ThreadPool* pool)
{
	for (int i = 0; i < pool->maxNum; i++)
	{
		if (pool->pthreadIDs[i] == pthread_self())
		{
			//������̣߳��Զ�����
			pthread_detach(pool->pthreadIDs[i]);
			pool->pthreadIDs[i] = 0;
			break;
		}
	}
	pthread_exit(NULL);
}

void pthreadPoolDestory(ThreadPool* pool)
{
	/*
	* �޸Ļ���λ
	* �����߳�
	* �ͷ���Դ
	* �ͷ���
	*/
	pool->shutdown = 1;
	pthread_join(pool->managerID,NULL);
	/*
	* ��ʱ�����߳������ֶ����ܵ�״̬���ȴ������������
	* 1.������������׼��ִ�������ִ���У����ֵȵ��ٴ������̳߳عر��жϻ��˳�
	* 2.�����ȴ����ѣ�������Ҫ���Ѻ��˳�
	*/
	//����ȫ�������е��߳�
	pthread_cond_broadcast(&pool->notEmpty);
	for (int i = 0; i < pool->maxNum; i++)
	{
		if (pool->pthreadIDs[i] != 0)
		{
			/*
			* �ȴ��߳̽����������߳���Դ
			* �����ȴ����ܻ�worker�߳���pool�ͷź����
			*/
			pthread_join(pool->pthreadIDs[i], NULL);
			
		}
	}
	pthread_mutex_destroy(&pool->poolMutex);
	pthread_mutex_destroy(&pool->busyMutex);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
	free(pool->taskQ);
	free(pool->pthreadIDs);
	free(pool);
}


