#include "threadpool.hpp"

#include <iostream>

void ThreadPool::init(size_t numThreads)
{
	taskRunning.resize(numThreads);
	tasks.resize(numThreads);
	for (int i = 0; i < numThreads; i++)
	{
		cvs.push_back(new std::condition_variable());
		mutexes.push_back(new std::mutex());
	}
	for (int i = 0; i < numThreads; i++)
	{
		threads.push_back(std::thread(&ThreadPool::threadfunc, this, i));
	}
}

void ThreadPool::submit(std::function<void()> task)
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

void ThreadPool::waitForAll()
{
	for (int i = 0; i < threads.size(); i++)
	{
		std::unique_lock<std::mutex> lock(*mutexes[i]);
		cvs[i]->wait(lock, [&] { return !taskRunning[i]; });
	}
	numInUse = 0;
}

void ThreadPool::threadfunc(int id)
{
	while (running)
	{
		std::unique_lock<std::mutex> lock(*mutexes[id]);
		cvs[id]->wait(lock, [&] { return taskRunning[id]; });

		tasks[id]();

		taskRunning[id] = false;
		cvs[id]->notify_one();
	}
}

