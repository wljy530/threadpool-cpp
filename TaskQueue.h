#pragma once
#include <queue>
#include <pthread.h>

using callback = void (*)(void*);  // 给函数指针起别名
// 任务结构体
struct Task
{
	Task()
	{
		function = nullptr;
		arg = nullptr;
	}

	Task(callback f, void* arg)
	{
		function = f;
		this->arg = arg;
	}

	callback function;
	void* arg;
};

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	// 添加任务
	void addTask(Task task);
	void addTask(callback f, void* arg);

	// 取出一个任务
	Task takeTask();

	// 获取当前任务的个数 (内联函数)
	inline int taskNumber()
	{
		return m_taskQ.size();
	}

private:
	pthread_mutex_t m_mutex;   // 互斥锁
	std::queue<Task> m_taskQ;  // 任务队列
};

