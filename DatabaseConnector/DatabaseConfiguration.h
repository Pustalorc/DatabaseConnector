#include <string>

class DatabaseConfiguration {
public:
	std::string DatabaseAddress, DatabaseName, DatabasePassword, DatabaseUsername;
	unsigned short DatabasePort;
	bool UseCache, UseSeparateThread;

	DatabaseConfiguration() {
		DatabaseAddress = "localhost";
		DatabaseUsername = "root";
		DatabasePassword = "asdf";
		DatabaseName = "database";
		DatabasePort = 3306;
		UseCache = true;
		UseSeparateThread = true;
	}
};