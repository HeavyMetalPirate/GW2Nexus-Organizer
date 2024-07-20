#ifndef API_TOKEN_SERVICE_H
#define API_TOKEN_SERVICE_H

#include "../Globals.h"
#include "HttpClient.h"
#include <map>
#include <string>
#include <thread>

class ApiTokenService {
public:
	void startService();
	void stopService();

	gw2::token::ApiToken* getToken(std::string identifier);

private:
	bool running = false;
	std::thread worker;
	std::map<std::string, gw2::token::ApiToken*> storedTokens;
};

#endif
