#pragma once

#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <functional>

class ThreadPool
{
public:
	void init(size_t numThreads = 4);
	void submit(std::function<void()> task);

	void waitForAll();
private:
	void threadfunc(int id);

	int nextThread()
	{
		currThread = (currThread + 1) % threads.size();
		return currThread;
	}
	int currThread = 0;
	int numInUse = 0;

	bool running = true;
	std::vector<std::thread> threads;
	std::vector<std::mutex*> mutexes;
	std::vector<std::condition_variable*> cvs;
	std::vector<bool> taskRunning;

	std::vector<std::function<void()>> tasks;
};