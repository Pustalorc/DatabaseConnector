/*
	DatabaseConnector.hpp
	Purpose: Basic connector instance for a nice queued, cached and multithreaded execution of MySql queries.

	@author Vicente Pastor
	@version 1.0 20/03/19
*/

#include <jdbc\cppconn\driver.h>
#include <jdbc\cppconn\statement.h>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include "DatabaseQueue.hpp"
#include "DatabaseConfiguration.h"
#include "Cache.h"

class DatabaseConnector
{
private:
	DatabaseQueue* _databaseQueue;
	std::vector<Cache> _cache;
	sql::Connection* connection = NULL;

public:
	DatabaseConfiguration Configuration;

	explicit DatabaseConnector(DatabaseConfiguration configuration)
	{
		Configuration = std::move(configuration);
		_databaseQueue = new DatabaseQueue();
		_databaseQueue->SubscribeToProcess([&](QueueableQuery* obj) { OnQueryProcess(obj); });

		while (connection == NULL) connection = CreateConnection();
	}

	void OnQueryProcess(QueueableQuery* query)
	{
		if (query->Type == EQueryType::NonQuery)
			PreExecuteNonQuery(query->Query);
		else
			PreExecuteReader(query->Query);
	}

	sql::Connection* CreateConnection()
	{
		sql::Driver *driver;
		sql::Connection *conn = NULL;

		try
		{
			if (Configuration.DatabasePort == 0)
				Configuration.DatabasePort = 3306;

			// STARTCOPY
			// Basic connection example from MySql: https://dev.mysql.com/doc/connector-cpp/1.1/en/connector-cpp-examples-complete-example-1.html
			driver = get_driver_instance();
			conn = driver->connect("tcp://" + Configuration.DatabaseAddress + ":" + std::to_string(Configuration.DatabasePort), Configuration.DatabaseUsername, Configuration.DatabasePassword);
			conn->setSchema(Configuration.DatabaseName);
			// ENDCOPY
		}
		catch (std::exception ex)
		{
			std::cout << ex.what();
		}

		return conn;
	}

	void RequestNonQuery(std::string query)
	{
		_databaseQueue->Enqueue(new QueueableQuery(query, EQueryType::NonQuery));
	}

	void RequestMultipleNonQuery(std::vector<std::string> queries)
	{
		for (auto query : queries)
			RequestNonQuery(query);
	}

	sql::ResultSet* RequestReader(std::string query)
	{
		if (!Configuration.UseCache) return ExecuteReader(query);
		
		auto cacheIterator = std::find_if(_cache.begin(), _cache.end(), [&query](const Cache& obj) {return obj.Query == query; });

		if (cacheIterator == _cache.end()) return ExecuteReader(query);

		auto cache = *cacheIterator;

		_databaseQueue->Enqueue(new QueueableQuery(query, EQueryType::Reader));
		return cache.Output;
	}

	[[deprecated("Do not use unless it's on startup or you are multithreading it.")]]
	void ExecuteNonQuery(std::string query)
	{
		try
		{
			auto command = connection->createStatement();

			command->execute(query);
		}
		catch (std::exception ex)
		{
			std::cout << ex.what();
		}
	}

	[[deprecated("Do not use unless it's on startup or you are multithreading it.")]]
	sql::ResultSet* ExecuteReader(std::string query)
	{
		sql::ResultSet* result = NULL;

		try
		{
			auto command = connection->createStatement();

			result = command->executeQuery(query);
		}
		catch (std::exception ex)
		{
			std::cout << ex.what();
		}

		if (!Configuration.UseCache) return result;

		auto cacheIterator = std::find_if(_cache.begin(), _cache.end(), [&query](const Cache& obj) {return obj.Query == query; });

		if (cacheIterator == _cache.end())
		{
			_cache.push_back(Cache(query, result));
			return result;
		}

		auto cache = *cacheIterator;

		auto updated = Cache(cache.Query, result);

		_cache.erase(std::remove(_cache.begin(), _cache.end(), cache), _cache.end());
		_cache.push_back(updated);
		return result;
	}

private:
	void PreExecuteNonQuery(std::string query)
	{
		if (!Configuration.UseSeparateThread) ExecuteNonQuery(query);

		std::thread t1(&DatabaseConnector::ExecuteNonQuery, this, query);
		ExecuteNonQuery(query);
	}

	sql::ResultSet* PreExecuteReader(std::string query)
	{
		if (!Configuration.UseSeparateThread) ExecuteReader(query);

		std::thread t1(&DatabaseConnector::ExecuteReader, this, query);
		return NULL;
	}
};