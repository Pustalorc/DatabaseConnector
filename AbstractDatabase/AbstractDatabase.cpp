#include <boost/bind.hpp>
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include "AbstractDatabaseConfiguration.hpp"
#include "QueueableQueries.hpp"
#include "ReaderCache.hpp"
#include "BackgroundWorker.hpp"

namespace AbstractDatabase {

	class DatabaseQueue
	{
	private:
		std::vector<QueueableQuery*> _queries;
		std::mutex _lock;
		BackgroundWorker* _worker;

		void DoWork()
		{
			std::condition_variable cv{};
			while (true)
			{
				QueueableQuery* item;

				auto skipEmptyCheck = false;

				std::unique_lock<std::mutex> lck { _lock };
				
				if (_queries.size() <= 0)
					return;

				if (_queries.size() > 1)
					skipEmptyCheck = true;

				cv.wait(lck);

				item = _queries[0];
				_queries.erase(_queries.begin());

				item->OnProcess();

				if (!skipEmptyCheck) continue;

				cv.wait(lck);
				if (_queries.size() <= 0)
					return;
			}
		}

	public:
		DatabaseQueue()
		{
			_worker = new BackgroundWorker();
			_worker->DoWork.connect(boost::bind(&DoWork, this));
		}

		void Enqueue(QueueableQuery* query)
		{
			_queries.push_back(query);
		}
	};

	template <class T> class AbstractDatabase;

	template <>
	class AbstractDatabase<AbstractDatabaseConfiguration>
	{
	private:
		DatabaseQueue* _databaseQueue;
		std::vector<ReaderCache> _readerCache;
	protected:
		AbstractDatabaseConfiguration Configuration;

		explicit AbstractDatabase(AbstractDatabaseConfiguration configuration)
		{
			Configuration = std::move(configuration);
			_databaseQueue = new DatabaseQueue();

			QueueableNonQuery::OnProcessNonQuery.connect(boost::bind(&PreExecuteNonQuery, this));
			QueueableReaderQuery::OnProcessReaderQuery.connect(boost::bind(&PreExecuteReader, this));

			auto connection = CreateConnection();

			try
			{
				if (!connection->isClosed)
					connection->close();
			}
			catch (std::exception ex)
			{
				std::cout << ex.what();
			}
		}

		sql::Connection* CreateConnection()
		{
			sql::Driver *driver;
			sql::Connection *connection;

			try
			{
				if (Configuration.DatabasePort == 0)
					Configuration.DatabasePort = 3306;

				driver = get_driver_instance();
				connection = driver->connect("tcp://" + Configuration.DatabaseAddress + ":" + std::to_string(Configuration.DatabasePort), Configuration.DatabaseUsername, Configuration.DatabasePassword);
				connection->setSchema(Configuration.DatabaseName);
			}
			catch (std::exception ex)
			{
				std::cout << ex.what();
			}

			return connection;
		}
		
		[[deprecated("Use only on startup (first try) or when checking tables.")]]
		sql::ResultSet* ExecuteScalar(std::string query)
		{
			sql::ResultSet* result = NULL;
			auto connection = CreateConnection();

			try
			{
				auto command = connection->createStatement();

				result = command->executeQuery(query);
			}
			catch (std::exception ex)
			{
				std::cout << ex.what();
			}

			connection->close();
			return result;
		}

		void RequestNonQuery(std::string query)
		{
			_databaseQueue->Enqueue(new QueueableNonQuery(query));
		}

		void RequestMultipleNonQuery(std::vector<std::string> queries)
		{
			for (auto i = 0; i < queries.size(); i++)
				_databaseQueue->Enqueue(new QueueableNonQuery(queries[i]));
		}

		sql::ResultSet* RequestReader(std::string query)
		{
			if (!Configuration.UseCache) return ExecuteReader(query);

			auto cacheIterator = std::find_if(_readerCache.begin(), _readerCache.end(), [&query](const ReaderCache& obj) {return obj.Query == query; });
			
			if (cacheIterator == _readerCache.end()) return ExecuteReader(query);

			auto cache = cacheIterator[0];
			
			if (!(difftime(std::time(0), cache.LastCacheUpdate) >
				Configuration.CacheRefreshMilliseconds)) return cache.Output;

			_databaseQueue->Enqueue(new QueueableReaderQuery(query));
			return cache.Output;
		}

	private:
		void PreExecuteNonQuery(std::string query)
		{
			if (!Configuration.UseSeparateThread) ExecuteNonQuery(query);

			std::thread t1(std::bind(&ExecuteNonQuery, this), query);
		}

		void ExecuteNonQuery(std::string query)
		{
			auto connection = CreateConnection();

			try
			{
				auto command = connection->createStatement();

				command->execute(query);
			}
			catch (std::exception ex)
			{
				std::cout << ex.what();
			}

			connection->close();
		}

		sql::ResultSet* PreExecuteReader(std::string query)
		{
			if (!Configuration.UseSeparateThread) ExecuteReader(query);

			std::thread t1(std::bind(&ExecuteReader, this), query);
			return NULL;
		}

		sql::ResultSet* ExecuteReader(std::string query)
		{
			sql::ResultSet* result;
			auto connection = CreateConnection();

			try
			{
				auto command = connection->createStatement();

				result = command->executeQuery(query);
			}
			catch (std::exception ex)
			{
				std::cout << ex.what();
			}

			connection->close();

			if (!Configuration.UseCache) return result;

			auto cacheIterator = std::find_if(_readerCache.begin(), _readerCache.end(), [&query](const ReaderCache& obj) {return obj.Query == query; });
			if (cacheIterator == _readerCache.end())
			{
				_readerCache.push_back(ReaderCache(query, result));
				return result;
			}

			auto cache = cacheIterator[0];

			if (!(difftime(std::time(0), cache.LastCacheUpdate) > Configuration.CacheRefreshMilliseconds)) return result;

			auto updated = ReaderCache(cache.Query, result);
			_readerCache.erase(std::remove(_readerCache.begin(), _readerCache.end(), cache), _readerCache.end());
			_readerCache.push_back(updated);
			return result;
		}
	};
}