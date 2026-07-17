#pragma once
#include <queue>
#include <pthread.h>

using callback = void (*)(void*);  // 给函数指针起别名
// 任务结构体
template <typename T>
struct Task
{
	Task<T>()
	{
		function = nullptr;
		arg = nullptr;
	}

	Task<T>(callback f, void* arg)
	{
		function = f;
		this->arg = static_cast<T*> (arg);
	}

	callback function;
	T* arg;
};

template <typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	// 添加任务
	void addTask(Task<T> task);
	void addTask(callback f, void* arg);

	// 取出一个任务
	Task<T> takeTask();

	// 获取当前任务的个数 (内联函数)
	inline size_t taskNumber()
	{
		return m_taskQ.size();
	}

private:
	pthread_mutex_t m_mutex;   // 互斥锁
	std::queue<Task<T>> m_taskQ;  // 任务队列
};


