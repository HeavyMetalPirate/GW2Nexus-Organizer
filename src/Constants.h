#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <map>
#include <vector>

static const char* ADDON_NAME = "Organizer";
static const std::string baseUrl = "https://api.guildwars2.com";

static const char* EV_NAME_DAILY_RESET = "EV_ORANIZER_DAILY_RESET_REQUEST";
static const char* EV_NAME_WEEKLY_RESET = "EV_ORANIZER_WEEKLY_RESET_REQUEST";

static const std::map<int, int> calender = {
	{1, 31},
	{2, 0}, // leap year calculation 
	{3, 31},
	{4, 30},
	{5, 31},
	{6, 30},
	{7, 31},
	{8, 31},
	{9, 30},
	{10,31},
	{11,30},
	{12,31}
};

static const char* monthsComboBoxItems[12] = {
	"Jan","Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char* daysOfWeek[] = { 
	"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" 
};

// String mappers
static const std::map<std::string, std::string> dailyCraftablesTranslator = {
	{"charged_quartz_crystal", "Charged Quartz Crystal"},
	{"glob_of_elder_spirit_residue", "Glob of Elder Spirit Residue"},
	{"lump_of_mithrilium","Lump of Mithrilium"},
	{"spool_of_silk_weaving_thread","Spool of Silk Weaving Thread"},
	{"spool_of_thick_elonian_cord","Spool of Thick Elonian Cord"}
};
static const std::map<std::string, std::string> mapChestsTranslator = {
	{"auric_basin_heros_choice_chest", "Auric Basin Hero's Choice Chest"},
	{"crystal_oasis_heros_choice_chest", "Crystal Oasis Hero's Choice Chest"},
	{"domain_of_vabbi_heros_choice_chest","Domain of Vabbi Hero's Choice Chest"},
	{"dragons_stand_heros_choice_chest","Dragon's Stand Hero's Choice Chest"},
	{"elon_riverlands_heros_choice_chest","Elon Riverlands Hero's Choice Chest"},
	{"tangled_depths_heros_choice_chest","Tangled Depths Hero's Choice Chest"},
	{"the_desolation_heros_choice_chest","The Desolation Hero's Choice Chest"},
	{"verdant_brink_heros_choice_chest","Verdant Brink Hero's Choice Chest"}
};
static const std::map<std::string, std::string> worldbossesTranslator = {
	{"admiral_taidha_covington", "Admiral Taidha Covington"},
	{"claw_of_jormag", "Claw of Jormag"},
	{"drakkar","Drakkar"},
	{"fire_elemental","Fire Elemental"},
	{"great_jungle_wurm","Great Jungle Worm"},
	{"inquest_golem_mark_ii","Inquest Golem Mark II"},
	{"karka_queen","Karka Queen"},
	{"megadestroyer","Megadestroyer"},	
	{"modniir_ulgoth", "Modniir Ulgoth"},
	{"shadow_behemoth", "Shadow Behemoth"},
	{"svanir_shaman_chief","Svanir Shaman Chief"},
	{"tequatl_the_sunless","Tequatl the Sunless"},
	{"the_shatterer","The Shatterer"},
	{"triple_trouble_wurm","Triple Trouble"}
};
static const std::map<std::string, std::string> dungeonTranslator = {
	{"ascalonian_catacombs", "Ascalonian Catacombs"},
	{"caudecus_manor", "Caudecus Manor"},
	{"twilight_arbor","Twilight Arbor"},
	{"sorrows_embrace","Sorrows Embrace"},
	{"citadel_of_flame","Citadel of Flame"},
	{"honor_of_the_waves","Honor of the Waves"},
	{"crucible_of_eternity","Crucible of Eternity"},
	{"ruined_city_of_arah","Ruined city of Arah"}
};
static const std::map<std::string, std::string> dungeonPathsTranslator = {
	{"ac_story", "Story"},
	{"hodgins", "Hodgins' Plan"},
	{"detha","Detha's Plan"},
	{"tzark","Tzark's Plan"},
	{"cm_story","Story"},
	{"asura","Helping the visiting Asura"},
	{"seraph","Looking for the Seraph"},
	{"butler","Finding the missing Butler"},
	{"ta_story", "Story"},
	{"leurent", "Leurent's Path (up)"},
	{"vevina","Vevina's Path (forward)"},
	{"aetherpath","Aetherpath"},
	{"se_story","Story"},
	{"fergg","Fergg's Path"},
	{"rasalov","Rasolov's Path"},
	{"koptev","Koptev's Path"},
	{"cof_story", "Ascalonian Catacombs"},
	{"ferrah", "Ferrah's Path"},
	{"magg","Magg's Path"},
	{"rhiannon","Rhiannon's Path"},
	{"hotw_story","Story"},
	{"butcher","Butcher Path"},
	{"plunderer","Plunderer Path"},
	{"zealot","Zealot Path"},
	{"coe_story", "Story"},
	{"submarine", "Escape using the Submarine"},
	{"teleporter","Escape using the Experimental Teleporter"},
	{"front_door","Escape through the Front Door"},
	{"arah_story","Story"},
	{"jotun","Jotun"},
	{"mursaat","Mursaat"},
	{"forgotten","Forgotten"},
	{"seer","Seer"}
};
static const std::map<std::string, std::string> raidTranslator = {
	{"spirit_vale", "Spirit Vale"},
	{"salvation_pass", "Salvation Pass"},
	{"stronghold_of_the_faithful","Stronghold of the Faithful"},
	{"bastion_of_the_penitent","Bastion of the Penitent"},
	{"hall_of_chains","Hall of Chains"},
	{"mythwright_gambit","Mythwright Gambit"},
	{"the_key_of_ahdashim","The Key of Adashim"}
};
static const std::map<std::string, std::string> raidBossesTranslator = {
	{"vale_guardian", "Vale Guardian"},
	{"spirit_woods", "Spirit Woods"},
	{"gorseval","Gorseval the Multifarious"},
	{"sabetha","Sabetha the Saboteur"},
	{"slothasor","Slothasor"},
	{"bandit_trio","Bandit Trio"},
	{"matthias","Matthias Gabrel"},
	{"escort","Escort"},
	{"keep_construct", "Keep Construct"},
	{"twisted_castle", "Twisted Castle"},
	{"xera","Xera"},
	{"cairn","Cairn the Indomitable"},
	{"mursaat_overseer","Mursaat Overseer"},
	{"samarog","Samarog"},
	{"deimos","Deimos"},
	{"soulless_horror","Soulless Horror"},
	{"river_of_souls", "River of Souls"},
	{"statues_of_grenth", "Statues of Grenth"},
	{"voice_in_the_void","Dhuum"},
	{"conjured_amalgamate","Conjured Amalgamate"},
	{"twin_largos","Twin Largos"},
	{"qadim","Qadim"},
	{"gate","Gate of Adashim"},
	{"adina","Cardinal Adina"},
	{"sabir", "Cardinal Sabir"},
	{"qadim_the_peerless", "Qadim the Peerless"}
};

#endif