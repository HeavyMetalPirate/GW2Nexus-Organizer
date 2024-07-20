#include "DataLoadingFunctions.h"

gw2::account::Account LoadAccountData(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);	
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account API. Certain functionality might not be fully available.");
			return {};
		}
		return json::parse(response);
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadAccountData(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account progression data. Certain functionality might not be fully available.");
	}
	return {};
}

std::vector<std::string> LoadDailyCrafting(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/dailycrafting?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account Daily Crafting API. Certain functionality might not be fully available.");
			return {};
		}
		return json::parse(response).get<std::vector<std::string>>();
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadDailyCrafting(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account Daily Crafting progression data. Certain functionality might not be fully available.");
	}
	return {};
}

std::vector<std::string> LoadDailyDungeons(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/dungeons?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account Dungeons API. Certain functionality might not be fully available.");
			return {};
		}
		return json::parse(response).get<std::vector<std::string>>();
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadDailyDungeons(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account Dungeons progression data. Certain functionality might not be fully available.");
	}
	return {};
}

std::vector<std::string> LoadDailyMetas(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/mapchests?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account MapChests API. Certain functionality might not be fully available.");
			return {};
		}
		return json::parse(response).get<std::vector<std::string>>();
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadDailyMetas(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account MapChests progression data. Certain functionality might not be fully available.");
	}
	return {};
}

std::vector<std::string> LoadWorldBosses(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/worldbosses?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account Worldbosses API. Certain functionality might not be fully available.");
			return {};
		}
		return json::parse(response).get<std::vector<std::string>>();
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadWorldBosses(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account Worldbosses progression data. Certain functionality might not be fully available.");
	}
	return {};
}

std::vector<std::string> LoadRaids(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/raids?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account Raids API. Certain functionality might not be fully available.");
			return {};
		}
		return json::parse(response).get<std::vector<std::string>>();
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadRaids(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account Raids progression data. Certain functionality might not be fully available.");
	}
	return {};
}

gw2::wizardsvault::MetaProgress LoadDailyWizardsVault(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/wizardsvault/daily?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account Wizards Vault Daily API. Certain functionality might not be fully available.");
		}
		else {
			return json::parse(response);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadDailyWizardsVault(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account Wizards Vault Daily progression data. Certain functionality might not be fully available.");
	}
	return {};
}

gw2::wizardsvault::MetaProgress LoadWeeklyWizardsVault(std::string apiKey) {
	try {
		std::string url = baseUrl + "/v2/account/wizardsvault/weekly?access_token=" + apiKey;
		std::string response;
		int retry = 0;
		while (response == "" && retry < 10) {
			response = HTTPClient::GetRequest(url);
			retry++;
		}
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Account Wizards Vault Daily API. Certain functionality might not be fully available.");
		}
		else {
			return json::parse(response);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgess()::LoadWeeklyWizardsVault(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account Wizards Vault Weekly progression data. Certain functionality might not be fully available.");
	}
	return {};
}