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
	while (1)		//工作线程，不停的从线程池中取出数据
	{
		pthread_mutex_lock(&pool->poolMutex);

		/*
		* 惊群现象
		* 当add发出notEmpty信号时，可能不只一个线程收到信号，此时没有竞争到
		* 锁的线程会继续在pthread_cond_wait等待竞争锁，等到获得锁后停止阻塞
		* 但此时后去的锁可能是另一个worker线程直接释放的锁，此时实际并未有任务
		*/
		while (pool->queueSize == 0)	
		{
			//若任务队列为空，则释放锁，并等待任务队列非空
			//当添加工作任务时，会发出notEmpty信号，告诉工作线程此时有任务
			pthread_cond_wait(&pool->notEmpty, &pool->poolMutex);
			
		}
		Task task = pool->taskQ[pool->queueFront];
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		pthread_cond_signal(&pool->notFull);		//告诉add，队列不再满
		pthread_mutex_unlock(&pool->poolMutex);
		task.fun(task.arg);				//执行任务
	}
	return NULL;
}

void pthreadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg)
{
	pthread_mutex_lock(&pool->poolMutex);	//避免与worker线程取任务相互影响
	while (pool->queueSize == pool->queueCapacity)
	{
		//当队列满时，等待队列不在满时再添加
		pthread_cond_wait(&pool->notFull, &pool->poolMutex);
	}
	pool->taskQ[pool->queueTail].fun = fun;
	pool->taskQ[pool->queueTail].arg = arg;
	pool->queueTail = (pool->queueTail + 1) % pool->queueCapacity;
	pool->queueSize++;
	//告诉工作线程，此时任务队列有任务
	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->poolMutex);
}


