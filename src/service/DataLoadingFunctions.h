#ifndef DATA_LOADING_FUNCTIONS_H
#define DATA_LOADING_FUNCTIONS_H

#include "HttpClient.h"
#include "../entity/GW2API.h"
#include "../Globals.h"

gw2::account::Account LoadAccountData(std::string apiKey);
std::vector<std::string> LoadDailyCrafting(std::string apiKey);
std::vector<std::string> LoadDailyDungeons(std::string apiKey);
std::vector<std::string> LoadDailyMetas(std::string apiKey);
std::vector<std::string> LoadWorldBosses(std::string apiKey);
gw2::wizardsvault::MetaProgress LoadDailyWizardsVault(std::string apiKey);
gw2::wizardsvault::MetaProgress LoadWeeklyWizardsVault(std::string apiKey);
std::vector<std::string> LoadRaids(std::string apiKey);

#endif