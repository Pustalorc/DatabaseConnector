/*
	DatabaseQueue.hpp
	Purpose: Base class for a queue system, in this case designed for database queue.

	@author Vicente Pastor
	@version 1.0 20/03/19
*/

# include <functional>
# include "QueueableQuery.h"
# include "BackgroundWorker.h"

class DatabaseQueue
{
private:
	std::vector<QueueableQuery*> _queries;
	std::vector<std::function<void(QueueableQuery*)>> _subscribedToProcess;
	BackgroundWorker* _worker;

	void DoWork()
	{
		std::condition_variable cv{};
		while (true)
		{
			QueueableQuery* item;

			auto skipEmptyCheck = false;

			if (_queries.size() <= 0)
				return;

			if (_queries.size() > 1)
				skipEmptyCheck = true;


			item = _queries[0];

			for (auto f : _subscribedToProcess) f(item);

			_queries.erase(_queries.begin());

			if (!skipEmptyCheck) continue;

			if (_queries.size() <= 0)
				return;
		}
	}

public:
	DatabaseQueue()
	{
		_worker = new BackgroundWorker();
		_worker->SubscribeToWork([&]{DoWork();});
	}

	void SubscribeToProcess(std::function<void(QueueableQuery*)> func)
	{
		_subscribedToProcess.push_back(func);
	}

	void Enqueue(QueueableQuery* query)
	{
		_queries.push_back(query);
	}
};