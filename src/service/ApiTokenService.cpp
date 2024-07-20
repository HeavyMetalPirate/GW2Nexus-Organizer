#include "ApiTokenService.h"

void ApiTokenService::startService() {
	running = true;
	worker = std::thread([&] {
		while (running) {
			for (auto key : settings.apiKeys) {
				if (!running) break;

				if (storedTokens.count(key.identifier)) {
					// key info already present, skip
					continue;
				}

				try {
					APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Fetching token info for key " + key.identifier).c_str());
					std::string url = baseUrl + "/v2/tokeninfo?access_token=" + key.apiKey;
					std::string response;
					int retry = 0;
					while (response == "" && retry < 10) {
						response = HTTPClient::GetRequest(url);
						retry++;
					}
					if (response == "") {
						APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Daily Crafting API. Certain functionality might not be fully available.");
						return;
					}
					json j = json::parse(response);
					gw2::token::ApiToken apiToken = j;

					storedTokens.emplace(key.identifier, new gw2::token::ApiToken(apiToken));
				}
				catch (const std::exception& e) {
					APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("ApiTokenService::workerThread(): " + std::string(e.what())).c_str());
				}
				catch (...) {
					APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load token info data. Certain functionality might not be fully available.");
				}
			}
			Sleep(100);
		}
	});

	worker.detach();
}

void ApiTokenService::stopService() {
	running = false;
	if (worker.joinable()) worker.join();
}

gw2::token::ApiToken* ApiTokenService::getToken(std::string identifier) {
	if (storedTokens.count(identifier)) return storedTokens.at(identifier);
	return nullptr;
}