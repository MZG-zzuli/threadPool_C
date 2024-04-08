#include <stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include"threadpool.h"

//任务函数
void func(int* num)
{
    printf("pthread_id:%u print: %d!\n",pthread_self(), *num);
    sleep(1);
}
int main()
{

    ThreadPool* pool = threadPoolCreate(4, 6, 50);
    if (pool == NULL)
    {
        printf("pool error!\n");
    }
    for (int i = 0; i < 100; i++)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        pthreadPoolAdd(pool, func, num);
    }
    sleep(30);
    pthreadPoolDestory(pool);
    return 0;
}