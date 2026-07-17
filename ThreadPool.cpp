#include "ThreadPool.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
using namespace std;

template <typename T>
ThreadPool<T>::ThreadPool(int min, int max)
{
	do 
	{
		// 初始化任务队列
		taskQ = new TaskQueue<T>;
		if (taskQ == nullptr)
		{
			cout << "new taskQ fail..." << endl;
			break;
		}

		// 根据线程的最大上限给线程数组分配内存
		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)
		{
			cout << "new threadIDs fail..." << endl;
			break;
		}
		// 初始化
		memset(threadIDs, 0, sizeof(pthread_t) * max);

		this->minNum = min;
		this->maxNum = max;
		busyNum = 0;
		liveNum = min;
		exitNum = 0;
		shutdown = false;

		// 初始化互斥锁和条件变量
		if (pthread_mutex_init(&mutexPool, NULL) != 0 || pthread_cond_init(&notEmpty, NULL) != 0)
		{
			cout << "muetx or condirtion init fail..." << endl;
			break;
		}

		// 创建管理者线程和工作线程 (一定要在初始化互斥锁和条件变量后创建)
		pthread_create(&managerID, NULL, ThreadPool::manager, this);
		for (int i = 0; i < minNum; i++)
		{
			pthread_create(&threadIDs[i], NULL, worker, this);
		}

		return;
	} while (0);	

	// 释放资源
	if (threadIDs)delete []threadIDs;
	if (taskQ)delete taskQ;
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
	// 关闭线程池
	pthread_mutex_lock(&mutexPool);
	shutdown = true;
	pthread_mutex_unlock(&mutexPool);

	// 阻塞回收管理者线程
	pthread_join(managerID, NULL);

	// 唤醒阻塞的工作线程，由于shutdown已经为true，阻塞中的工作线程会自杀
	for (int i = 0; i < liveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}

	// 阻塞回收正在工作的工作线程
	for (int i = 0; i < maxNum; i++)
	{
		if (threadIDs[i] != 0)
		{
			pthread_join(threadIDs[i], NULL);
		}
	}

	// 释放堆内存
	if (taskQ)
	{
		delete taskQ;
		taskQ = nullptr;
	}
	if (threadIDs)
	{
		delete []threadIDs;
		threadIDs = nullptr;
	}

	// 销毁互斥锁和条件变量
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);
}

template <typename T>
void ThreadPool<T>::addTask(Task<T> task)  // 由于任务队列类内部的addTask已经加互斥锁保持同步，因此这里不需要加锁了
{
	if (shutdown)
	{
		pthread_mutex_unlock(&mutexPool);
		return;
	}

	// 添加任务
	taskQ->addTask(task);

	// 唤醒工作线程工作
	pthread_cond_signal(&notEmpty);
}

template <typename T>
int ThreadPool<T>::getBusyNum()
{
	pthread_mutex_lock(&mutexPool);
	int busyNum = this->busyNum;
	pthread_mutex_unlock(&mutexPool);

	return busyNum;
}

template <typename T>
int ThreadPool<T>::getAliveNum()
{
	pthread_mutex_lock(&mutexPool);
	int aliveNum = this->liveNum;
	pthread_mutex_unlock(&mutexPool);

	return aliveNum;
}

template <typename T>
void* ThreadPool<T>::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*> (arg);

	while (true)
	{
		pthread_mutex_lock(&pool->mutexPool);
		// 判断当前任务队列是否为空
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			// 阻塞工作线程
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

			// 判断是否要销毁当前线程
			if (pool->exitNum > 0)
			{
				pool->exitNum--;  // 无论是否成功销毁都要求自减

				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexPool);
					pool->threadExit();
				}
			}
		}

		// 判断线程池是否关闭
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			pool->threadExit();
		}

		// 从任务队列中取出一个任务
		Task<T> task = pool->taskQ->takeTask();
		pool->busyNum++;
		
		// 解锁
		pthread_mutex_unlock(&pool->mutexPool);

		// 执行任务
		cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		// 结束任务
		cout << "thread " << to_string(pthread_self()) << " end working..." << endl;
		pthread_mutex_lock(&pool->mutexPool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexPool);
	}

	return nullptr;
}

template <typename T>
void* ThreadPool<T>::manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*> (arg);

	while (!pool->shutdown)
	{
		// 每隔3s检测一次
		sleep(3);

		// 取出任务数、存活线程数、忙的线程数、最大线程数、最小线程数
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		int maxNum = pool->maxNum;
		int minNum = pool->minNum;
		pthread_mutex_unlock(&pool->mutexPool);

		// 添加线程
		// 任务的个数 > 存活线程的个数 && 存活线程数 < 最大线程数
		if (queueSize > liveNum && liveNum < maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			
			int counter = 0;
			for (int i = 0; i < maxNum && counter < NUMBER && pool->liveNum < maxNum; i++)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					counter++;
					pool->liveNum++;
					liveNum = pool->liveNum;
				}
			}

			pthread_mutex_unlock(&pool->mutexPool);
		}

		// 销毁线程
		// 忙的线程 * 2 < 存活的线程数 && 存活的线程 > 最小线程数
		if (busyNum * 2 < liveNum && liveNum > minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);

			// 唤醒工作线程，让其自杀
			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}

	return nullptr;
}

template <typename T>
void ThreadPool<T>::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; i++)
	{
		if (threadIDs[i] == tid)
		{
			pthread_mutex_lock(&mutexPool);
			threadIDs[i] = 0;
			pthread_mutex_unlock(&mutexPool);
			cout << "threadExit() called, " << to_string(tid) << " exiting..." << endl;
			break;
		}
	}
	pthread_exit(NULL);
}
