#include <iostream>

class AbstractDatabaseConfiguration {
public:
	std::string DatabaseAddress, DatabaseName, DatabasePassword, DatabaseUsername;
	unsigned short DatabasePort;
	unsigned long CacheRefreshMilliseconds;
	bool UseCache, UseSeparateThread;

	AbstractDatabaseConfiguration() {
		DatabaseAddress = "localhost";
		DatabaseUsername = "root";
		DatabasePassword = "password";
		DatabaseName = "database";
		DatabasePort = 3306;
		UseCache = true;
		UseSeparateThread = true;
		CacheRefreshMilliseconds = 60000;
	}
};