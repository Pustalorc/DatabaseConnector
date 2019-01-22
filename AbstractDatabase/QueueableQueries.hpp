#include <boost/signals2.hpp>
#include <cppconn/statement.h>

class QueueableQuery
{
public:
	std::string Query;

	virtual void OnProcess()
	{

	}

protected:
	QueueableQuery(std::string query)
	{
		Query = query;
	}
};

class QueueableNonQuery : public QueueableQuery
{
public:
	QueueableNonQuery(std::string query) : QueueableQuery(query)
	{

	}

	void OnProcess() override
	{
		OnProcessNonQuery(Query);
	}

	static boost::signals2::signal<void(std::string query)> OnProcessNonQuery;
};

class QueueableReaderQuery : public QueueableQuery
{
public:
	QueueableReaderQuery(std::string query) : QueueableQuery(query)
	{

	}

	void OnProcess() override
	{
		OnProcessReaderQuery(Query);
	}

	static boost::signals2::signal<sql::ResultSet* (std::string query)> OnProcessReaderQuery;
};