#include "TaskQueue.h"
#include "ThreadPool.h"

TaskQueue::TaskQueue()
{
	pthread_mutex_init(&m_mutex, NULL);
}

TaskQueue::~TaskQueue()
{
	pthread_mutex_destroy(&m_mutex);
}

ThreadPool::ThreadPool(int min, int max)
{
}

ThreadPool::~ThreadPool()
{
}

void TaskQueue::addTask(Task task)
{
	pthread_mutex_lock(&m_mutex);
	m_taskQ.push(task);
	pthread_mutex_unlock(&m_mutex);
}

int ThreadPool::getBusyNum()
{
	return 0;
}

int ThreadPool::getAliveNum()
{
	return 0;
}

void ThreadPool::worker(void* arg)
{
}

void ThreadPool::manager(void* arg)
{
}

void ThreadPool::threadExit()
{
}

void TaskQueue::addTask(callback f, void* arg)
{
	pthread_mutex_lock(&m_mutex);
	m_taskQ.push(Task(f, arg));
	pthread_mutex_unlock(&m_mutex);
}

Task TaskQueue::takeTask()            //这里队列为空返回t是否需要优化
{
	Task t;

	pthread_mutex_lock(&m_mutex);
	if (!m_taskQ.empty())
	{
		t = m_taskQ.front();
		m_taskQ.pop();
	}
	pthread_mutex_unlock(&m_mutex);

	return t;
}




