#include "mythreadpool.hpp"

#include <iostream>

void MyThreadPool::init(size_t numThreads)
{
	taskRunning.resize(numThreads);
	tasks.resize(numThreads);
	for (int i = 0; i < numThreads; i++)
	{
		cvs.push_back(new std::condition_variable());
		mutexes.push_back(new std::mutex());
		mainMutexes.push_back(new std::mutex());
	}
	for (int i = 0; i < numThreads; i++)
	{
		threads.push_back(std::thread(&MyThreadPool::threadfunc, this, i));
	}
}

void MyThreadPool::submit(std::function<void()> task)
{
	if (numInUse == threads.size())
	{
		throw std::runtime_error("Cannot submit more tasks to thread pool, all threads occupied! (call waitForAll())");
	}
	numInUse++;

	int id = nextThread();
	{
		std::lock_guard<std::mutex> lock(*mutexes[id]);
		taskRunning[id] = true;
		tasks[id] = task;
	}
	cvs[id]->notify_one();
}

void MyThreadPool::waitForAll()
{
	for (int i = 0; i < threads.size(); i++)
	{
		std::cout << "waiting for thread " << i << "\n";
		std::unique_lock<std::mutex> lock(*mainMutexes[i]);
		while (taskRunning[i])
			cvs[i]->wait(lock, [this, i] { return !taskRunning[i]; });
	}
	std::cout << "///////////////////\n";
	numInUse = 0;
}

void MyThreadPool::threadfunc(int id)
{
	while (running)
	{
		{
			std::unique_lock<std::mutex> lock(*mutexes[id]);
			while(!taskRunning[id])
				cvs[id]->wait(lock, [this, id] { return taskRunning[id]; });
		}

		tasks[id]();

		{
			std::lock_guard<std::mutex> lock(*mainMutexes[id]);
			taskRunning[id] = false;
		}

		cvs[id]->notify_one();
	}
}

