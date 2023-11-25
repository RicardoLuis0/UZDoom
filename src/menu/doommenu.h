#pragma once
#include "menu.h"
#include "savegamemanager.h"

struct FNewGameStartup
{
	bool hasPlayerClass;
	FString PlayerClass;
	int Episode;
	int Skill;
};

extern FNewGameStartup NewGameStartupInfo;
void M_StartupEpisodeMenu(FNewGameStartup *gs);
void M_StartupSkillMenu(FNewGameStartup *gs);
void M_CreateGameMenus();
void SetDefaultMenuColors();
void OnMenuOpen(bool makeSound);

class FSavegameManager : public FSavegameManagerBase
{
	time_t lastSaveTime = 0;
	FSaveGameNode * lastSave = nullptr;

	void PerformSaveGame(const char *fn, const char *sgdesc) override;
	void PerformLoadGame(const char *fn, bool) override;
	FString ExtractSaveComment(FSerializer &arc) override;
	FString BuildSaveName(const char* prefix, int slot) override;
	void ReadSaveStrings() override;
	int GetLastSavegame() override;
	void SetLastSavegame(int index) override;
};

extern FSavegameManager savegameManager;

