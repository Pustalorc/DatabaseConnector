/*
	Cache.h
	Purpose: Basic class for the cache.

	@author Vicente Pastor
	@version 1.0 20/03/19
*/

#include <ctime>
#include <jdbc\cppconn\statement.h>

class Cache
{
public:
	std::string Query;
	std::time_t LastCacheUpdate;
	sql::ResultSet* Output;

	Cache(std::string query, sql::ResultSet* output)
	{
		Query = query;
		Output = output;
		LastCacheUpdate = time(0);
	}

	bool operator==(Cache c)
	{
		return c.Query == this->Query;
	}
};