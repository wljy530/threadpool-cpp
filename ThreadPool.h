#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"

template <typename T>
class ThreadPool
{
public:
	// 创建线程池并初始化
	ThreadPool(int min, int max);

	// 销毁线程池
	~ThreadPool();

	// 给线程池添加任务
	void addTask(Task<T> task);

	// 获取线程池中工作的线程的个数
	int getBusyNum();

	// 获取线程池中活着的线程的个数
	int getAliveNum();

private:
	// 工作线程任务函数
	static void* worker(void* arg);

	// 管理者线程任务函数
	static void* manager(void* arg);

	// 单个线程退出
	void threadExit();

private:
	TaskQueue<T>* taskQ;            // 任务队列
	pthread_t managerID;         // 管理者线程ID
	pthread_t* threadIDs;        // 工作线程ID
	int minNum;                  // 最小线程数
	int maxNum;                  // 最大线程数
	int busyNum;			     // 忙的线程数
	int liveNum;                 // 存活的线程数
	int exitNum;			     // 要销毁的线程数
	pthread_mutex_t mutexPool;   // 锁整个线程池
	pthread_cond_t notEmpty;     // 任务队列是不是空了

	bool shutdown;               // 是否要销毁线程池

	static const int NUMBER = 2; // 管理者线程根据情况一次销毁或者创建两个工作线程
};


