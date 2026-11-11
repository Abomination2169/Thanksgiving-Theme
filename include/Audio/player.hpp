#pragma once
#include "GlobalNamespace/SongPreviewPlayer.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
namespace Thanksgiving::Audio {
    void EnterMenu(GlobalNamespace::MainMenuViewController* menu);
    void ExitMenu();
    void EnterSoloSelection();
    void Tick();
}
// Compatibility declarations for legacy hook definitions, which remain uninstalled.
void InitializeTurkeyGobbles(GlobalNamespace::SongPreviewPlayer*);
void DisableTurkeyGobbles(GlobalNamespace::SongPreviewPlayer*);
void UpdateTurkeyGobbles(GlobalNamespace::SongPreviewPlayer*);