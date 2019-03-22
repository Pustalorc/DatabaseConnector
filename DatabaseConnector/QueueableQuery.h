/*
	QueueableQuery.h
	Purpose: A basic class for queueable queries as well as a small enum for the types of queries.

	@author Vicente Pastor
	@version 1.0 20/03/19
*/

#include <string>

enum EQueryType { Reader, NonQuery };

class QueueableQuery
{
public:
	std::string Query;
	EQueryType Type;

	QueueableQuery(std::string query, EQueryType type)
	{
		Query = query;
		Type = type;
	}
};