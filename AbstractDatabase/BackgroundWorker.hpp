#include <boost/signals2.hpp>

class BackgroundWorker {
public:
	boost::signals2::signal<void(void)> DoWork;

	BackgroundWorker() : time{ std::chrono::milliseconds{ 10 } } {}
	~BackgroundWorker() { wait_thread.join(); }

private:
	void wait_then_call()
	{
		std::unique_lock<std::mutex> lck{ mtx };
		DoWork();
		cv.wait_for(lck, std::chrono::milliseconds{ 10 });
	}
	std::condition_variable cv{};
	std::mutex mtx;
	std::chrono::milliseconds time;
	std::thread wait_thread{ [this]() {wait_then_call(); } };
};