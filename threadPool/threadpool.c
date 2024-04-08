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
	while (1)		//工作线程，不停的从线程池中取出数据
	{
		pthread_mutex_lock(&pool->poolMutex);

		/*
		* 惊群现象
		* 当add发出notEmpty信号时，可能不只一个线程收到信号，此时没有竞争到
		* 锁的线程会继续在pthread_cond_wait等待竞争锁，等到获得锁后停止阻塞
		* 但此时后去的锁可能是另一个worker线程直接释放的锁，此时实际并未有任务
		*/
		while (pool->queueSize == 0&&pool->shutdown==0)	
		{
			//若任务队列为空，则释放锁，并等待任务队列非空
			//当添加工作任务时，会发出notEmpty信号，告诉工作线程此时有任务
			pthread_cond_wait(&pool->notEmpty, &pool->poolMutex);
			if (pool->exitNum > 0)
			{
				//printf("xhhh\n");
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					//线程退出

					pool->liveNum--;
					pthread_mutex_unlock(&pool->poolMutex);
					//printf("yxh\n");
					exitPthread(pool);
				}
			}
		}
		if (pool->shutdown == 1)
		{
			//线程池关闭，线程退出，由父进程回收资源
			pthread_mutex_unlock(&pool->poolMutex);
			pthread_exit(NULL);
		}
		Task task = pool->taskQ[pool->queueFront];
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		pthread_cond_signal(&pool->notFull);		//告诉add，队列不再满
		pthread_mutex_unlock(&pool->poolMutex);
		pthread_mutex_lock(&pool->busyMutex);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->busyMutex);
		task.fun(task.arg);				//执行任务
		pthread_mutex_lock(&pool->busyMutex);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->busyMutex);
	}
	return NULL;
}


void pthreadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg)
{
	pthread_mutex_lock(&pool->poolMutex);	//避免与worker线程取任务相互影响
	while (pool->queueSize == pool->queueCapacity&&pool->shutdown==0)
	{
		//当队列满时，等待队列不在满时再添加
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
	//告诉工作线程，此时任务队列有任务
	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->poolMutex);
}

void* manager(ThreadPool* pool)
{
	while (pool->shutdown==0)
	{

		//销毁
		if (pool->busyNum * 2 < pool->liveNum && pool->liveNum>pool->minNum)
		{
			pool->exitNum++;
			pthread_cond_signal(&pool->notEmpty);
		}
		//创建
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
			//分离此线程，自动回收
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
	* 修改回收位
	* 回收线程
	* 释放资源
	* 释放锁
	*/
	pool->shutdown = 1;
	pthread_join(pool->managerID,NULL);
	/*
	* 此时工作线程有两种而可能的状态：等待任务池有任务
	* 1.非阻塞，正在准备执行任务或执行中，这种等到再次遇见线程池关闭判断会退出
	* 2.阻塞等待唤醒，这种需要唤醒后退出
	*/
	//唤醒全部阻塞中的线程
	pthread_cond_broadcast(&pool->notEmpty);
	for (int i = 0; i < pool->maxNum; i++)
	{
		if (pool->pthreadIDs[i] != 0)
		{
			/*
			* 等待线程结束，回收线程资源
			* 若不等待可能会worker线程在pool释放后访问
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


