#ifndef GW2API_H
#define GW2API_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace gw2 {
	namespace account {
		struct Account {
			std::string id;
			std::string name;
			int age;
			int world;
			std::vector<std::string> guilds;
			std::vector<std::string> guild_leader;
			std::string created;
			std::vector<std::string> access;
			bool commander;
			int fractal_level;
			int daily_ap;
			int monthly_ap;
			int wvw_rank;
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Account, id, name, age, world, guilds, guild_leader, created, access, commander, fractal_level, daily_ap, monthly_ap, wvw_rank)
	}

	namespace token {
		struct ApiToken {
			std::string id;
			std::string name;
			std::vector<std::string> permissions;
		};
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ApiToken, id, name, permissions);
	}

	namespace crafting {
		struct DailyCrafting {
			std::vector<std::string> recipes;
		};
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DailyCrafting, recipes);
	}

	namespace dungeon {
		struct Path {
			std::string id;
			std::string type;
			
		};
		struct Dungeon {
			std::string id;
			std::vector<Path> paths;
			
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Path, id, type);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Dungeon, id, paths);
	}

	namespace world {
		struct MapChests {
			std::vector<std::string> chests;
		};
		struct WorldBoss {
			std::vector<std::string> worldbosses;
		};
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MapChests, chests);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WorldBoss, worldbosses);
	}

	namespace raid {
		struct Event {
			std::string id;
			std::string type;
		};

		struct Wing {
			std::string id;
			std::vector<Event> events;
		};

		struct Raid {
			std::string id;
			std::vector<Wing> wings;
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Event, id, type)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Wing, id, events)
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Raid, id, wings)
	}

	namespace wizardsvault {
		struct Objective {
			int id;
			std::string title;
			std::string track;
			int acclaim;
			int progress_current;
			int progress_complete;
			bool claimed;
			
		};

		struct MetaProgress {
			int meta_progress_current;
			int meta_progress_complete;
			int meta_reward_item_id;
			int meta_reward_astral;
			bool meta_reward_claimed;
			std::vector<Objective> objectives;
			
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Objective, id, title, track, acclaim, progress_current, progress_complete, claimed);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MetaProgress, meta_progress_current, meta_progress_complete, meta_reward_item_id, meta_reward_astral, meta_reward_claimed, objectives);
	}

	namespace achievements {
		struct AchievementCategory {
			int id;
			std::string name;
			std::string description;
			int order;
			std::string icon;
			std::vector<int> achievements;

			bool const operator==(const AchievementCategory& o) const {
				return id == o.id;
			}
			bool const operator<(const AchievementCategory& o) const {
				return id < o.id;
			}
			bool const operator>(const AchievementCategory& o) const {
				return id > o.id;
			}

		};
		struct Tier {
			int count;
			int points;
		};

		struct Reward {
			std::string type;
			int id;
			int count;
		};
		struct Achievement {
			int id;
			std::string icon;
			std::string name;
			std::string description;
			std::string requirement;
			std::string locked_text;
			std::string type;
			std::vector<std::string> flags;
			std::vector<Tier> tiers;
			std::vector<Reward> rewards;
			std::vector<int> prerequisites;
		};

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AchievementCategory, id, name, description, order, icon, achievements);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Reward, type, id, count);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Tier, count, points);
		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Achievement, id, icon, name, description, requirement, locked_text, type, flags, tiers, rewards, prerequisites);
	}
}
#endif