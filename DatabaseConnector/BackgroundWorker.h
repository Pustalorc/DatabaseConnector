/*
	BackgroundWorker.h
	Purpose: A multithreaded worker that runs on the background, constantly calling any subscribed functions. Idea for this comes from .Net framework's background worker.

	@author Vicente Pastor
	@version 1.0 20/03/19
*/

#include <chrono>
#include <mutex>

class BackgroundWorker {
public:
	BackgroundWorker() : time{ std::chrono::milliseconds{ 10 } } {}

	void SubscribeToWork(std::function<void(void)> func)
	{
		DoWork.push_back(func);
	}

private:
	std::vector<std::function<void(void)>> DoWork;

	void wait_then_call()
	{
		std::unique_lock<std::mutex> lck{ mtx };

		for (auto f : DoWork) f();

		cv.wait_for(lck, std::chrono::milliseconds{ 10 });
	}

	std::condition_variable cv{};
	std::mutex mtx;
	std::chrono::milliseconds time;
	std::thread wait_thread{ [this]() { while (true) wait_then_call(); } };
};