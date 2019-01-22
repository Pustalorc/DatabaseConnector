#include <ctime>
#include <cppconn/statement.h>

class ReaderCache
{
public:
	std::string Query;
	std::time_t LastCacheUpdate;
	sql::ResultSet* Output;

	ReaderCache(std::string query, sql::ResultSet* output)
	{
		Query = query;
		Output = output;
		LastCacheUpdate = time(0);
	}
};