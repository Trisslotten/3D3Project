#include "threadpool.hpp"

#include <iostream>

void ThreadPool::init(size_t numThreads)
{
	startTask.resize(numThreads);
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

void ThreadPool::submit(std::function<void()> lambda)
{
	int id = nextThread();
	{
		std::lock_guard<std::mutex> lock(*mutexes[id]);
		startTask[id] = true;
		// TODO: add submitted task so reachable in threadfunc
	}
	cvs[id]->notify_one();
}

void ThreadPool::waitForAll()
{
	for (int i = 0; i < threads.size(); i++)
	{
		std::unique_lock<std::mutex> lock(*mutexes[i]);
		cvs[i]->wait(lock, [&] { return !startTask[i]; });
	}
}

void ThreadPool::threadfunc(int id)
{
	while (running)
	{
		std::unique_lock<std::mutex> lock(*mutexes[id]);
		cvs[id]->wait(lock, [&] { return startTask[id]; });

		// TODO: run submitted task here
		//std::this_thread::sleep_for(std::chrono::milliseconds(10));

		startTask[id] = false;
		cvs[id]->notify_one();
	}
}

