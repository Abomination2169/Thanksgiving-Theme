#include "main.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/MaterialPropertyBlock.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Shader.hpp"


#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/GameNoteController.hpp"
#include "GlobalNamespace/FlickeringNeonSign.hpp"
#include "GlobalNamespace/GameServerLobbyFlowCoordinator.hpp"
#include "GlobalNamespace/MultiplayerModeSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LightWithIdManager.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/MediaAsyncLoader.hpp"

#include "logging.hpp"

#include "Menu/ObjectInstances.hpp"

#include "Audio/player.hpp"
#include "custom-types/shared/register.hpp"

#include "GlobalNamespace/SongPreviewPlayer.hpp"
#include "UnityEngine/AudioSource.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "custom-types/shared/macros.hpp"

#include "UnityEngine/Time.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/SceneManagement/SceneManagement.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/SpriteRenderer.hpp"

#include "System/Threading/Tasks/Tasks.hpp"

#include "Block/colorchanger.hpp"
#include "AutumnLeaves/autumnLeaves.hpp"
#include "Menu/pumpkinAccents.hpp"




#include "c.hpp"

#include "scotland2/shared/modloader.h"


using namespace GlobalNamespace;
using namespace UnityEngine;

namespace {
    constexpr const char* floorDiagnosticBuild = "menu-floor-restoration-v3";
    SafePtrUnity<MeshRenderer> menuFloor;
    Color menuFloorBaseline{};
    bool hasMenuFloorBaseline = false;
    bool floorRestorationActive = false;
    bool loggedFloorRestore = false;

    MeshRenderer* FindMenuFloor(int colorProperty) {
        for (auto* renderer : Resources::FindObjectsOfTypeAll<MeshRenderer*>()) {
            if (!renderer || !renderer->get_enabled()) continue;
            auto go = renderer->get_gameObject();
            if (!go || !go->get_activeInHierarchy() || go->get_name() != "BasicMenuGround") continue;
            const auto materials = renderer->get_sharedMaterials();
            if (materials.size() != 1) continue;
            auto material = materials[0];
            if (material && material->get_name() == "MenuFloor" && material->HasProperty(colorProperty))
                return renderer;
        }
        return nullptr;
    }

}

// Unity copies this block into the renderer. Restore the caller's value after
// submission in case the same block is reused for other environment meshes.
MAKE_HOOK_MATCH(MenuFloorSetPropertyBlock,
    static_cast<void (Renderer::*)(MaterialPropertyBlock*)>(&Renderer::SetPropertyBlock),
    void, Renderer* self, MaterialPropertyBlock* block) {
    if (block && hasMenuFloorBaseline && floorRestorationActive && menuFloor &&
        self == static_cast<Renderer*>(menuFloor.ptr()) && getModConfig().EnableHauntedEnvColors.GetValue()) {
        static const int colorProperty = Shader::PropertyToID("_Color");
        const auto incoming = block->GetColor(colorProperty);
        // Ignore neutral and unset overrides, including the guard's own writes.
        if ((incoming.r != incoming.g || incoming.g != incoming.b) &&
            (incoming.r != menuFloorBaseline.r || incoming.g != menuFloorBaseline.g ||
             incoming.b != menuFloorBaseline.b || incoming.a != menuFloorBaseline.a)) {
            block->SetColor(colorProperty, menuFloorBaseline);
            MenuFloorSetPropertyBlock(self, block);
            block->SetColor(colorProperty, incoming);
            if (!loggedFloorRestore) {
                loggedFloorRestore = true;
                INFO("[MenuFloorRestoration] intercepted BasicMenuGround _Color before submission RGBA=({},{},{},{})",
                     incoming.r, incoming.g, incoming.b, incoming.a);
            }
            return;
        }
    }
    MenuFloorSetPropertyBlock(self, block);
}

DECLARE_CLASS_CODEGEN(Thanksgiving, FloorRendererSample, UnityEngine::MonoBehaviour) {
    DECLARE_INSTANCE_FIELD(float, nextLookupAt);
    DECLARE_INSTANCE_FIELD(UnityEngine::MaterialPropertyBlock*, propertyBlock);
    DECLARE_INSTANCE_METHOD(void, LateUpdate);
};
DEFINE_TYPE(Thanksgiving, FloorRendererSample);

void Thanksgiving::FloorRendererSample::LateUpdate() {
    if (UnityEngine::SceneManagement::SceneManager::GetActiveScene().name == "GameCore") {
        if (!floorRestorationActive) loggedFloorRestore = false;
        floorRestorationActive = true;
        return;
    }
    if (hasMenuFloorBaseline && (!floorRestorationActive || !getModConfig().EnableHauntedEnvColors.GetValue())) return;

    static const int colorProperty = Shader::PropertyToID("_Color");
    if (!menuFloor) {
        const float now = Time::get_realtimeSinceStartup();
        if (now < nextLookupAt) return;
        nextLookupAt = now + 0.5f;
        menuFloor = FindMenuFloor(colorProperty);
        if (!menuFloor) return;
    }

    auto* renderer = menuFloor.ptr();
    auto go = renderer->get_gameObject();
    if (!go || !go->get_activeInHierarchy() || !renderer->get_enabled()) return;
    renderer->GetPropertyBlock(propertyBlock);
    const auto current = propertyBlock->GetColor(colorProperty);

    if (!hasMenuFloorBaseline) {
        if (floorRestorationActive) return;
        // An unset override is reported as zero by the Quest binding.
        const bool unset = current.r == 0 && current.g == 0 && current.b == 0 && current.a == 0;
        menuFloorBaseline = unset ? renderer->get_sharedMaterials()[0]->GetColor(colorProperty) : current;
        hasMenuFloorBaseline = true;
        INFO("[MenuFloorRestoration] baselineRGBA=({},{},{},{})",
             menuFloorBaseline.r, menuFloorBaseline.g, menuFloorBaseline.b, menuFloorBaseline.a);
        return;
    }

    if (current.r == menuFloorBaseline.r && current.g == menuFloorBaseline.g &&
        current.b == menuFloorBaseline.b && current.a == menuFloorBaseline.a) return;
    // Preserve the other properties written by the menu lighting system.
    propertyBlock->SetColor(colorProperty, menuFloorBaseline);
    renderer->SetPropertyBlock(propertyBlock);
    if (!loggedFloorRestore) {
        loggedFloorRestore = true;
        INFO("[MenuFloorRestoration] LateUpdate corrected BasicMenuGround from RGBA=({},{},{},{})",
             current.r, current.g, current.b, current.a);
    }
}

namespace {
    void StartMenuFloorRestoration() {
        auto* probe = GameObject::New_ctor("ThanksgivingMenuFloorRestoration");
        auto* sampler = probe->AddComponent<Thanksgiving::FloorRendererSample*>();
        if (!sampler) {
            GameObject::Destroy(probe);
            INFO("[MenuFloorRestoration] Failed to create floor controller");
            return;
        }
        sampler->nextLookupAt = Time::get_realtimeSinceStartup() + 0.5f;
        sampler->propertyBlock = MaterialPropertyBlock::New_ctor();
        GameObject::DontDestroyOnLoad(probe);
    }
}



static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
// Stores the ID and version of our mod, and is sent to
// the modloader upon startup

// static std::shared_ptr<cColor::ColorChanger> colorChanger;

//MAKE_HOOK_MATCH(SongPreviewPlayer_OnEnable, &GlobalNamespace::SongPreviewPlayer::OnEnable, void, GlobalNamespace::SongPreviewPlayer* self) {
//
//   MenuMusic::menuMusicLoader.set_OriginalClip(self->_defaultAudioClip);
//
//    if (!MenuMusic::menuMusicLoader.get_OriginalClip()) {
//        PaperLogger.info("Loading custom menu music...");
//        MenuMusic::menuMusicLoader.LoadCustomMusic();  // Keine Coroutine mehr nÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¶tig
//    }
//
//    self->_defaultAudioClip = MenuMusic::menuMusicLoader.get_OriginalClip();
//
//    SongPreviewPlayer_OnEnable(self);
//}

// Called whenever the main menu scene loads
MAKE_HOOK_MATCH(SongPreviewPlayer_OnEnable, &SongPreviewPlayer::OnEnable, void, SongPreviewPlayer* self) {

    SongPreviewPlayer_OnEnable(self);
    InitializeTurkeyGobbles(self);
}

// Called whenever the SongPreviewPlayer transitions from one song to another
MAKE_HOOK_MATCH(SongPreviewPlayer_CrossfadeTo, static_cast<void(SongPreviewPlayer::*)(AudioClip*, float, float, float, bool, System::Action*)>(&SongPreviewPlayer::CrossfadeTo), void, SongPreviewPlayer* self, AudioClip* audioClip, float musicVolume, float startTime, float duration, bool isDefault, System::Action* onFadeOutCallback) {
    // Choose a new random main menu song when transitioning away from the main menu music

    SongPreviewPlayer_CrossfadeTo(self, audioClip, musicVolume, startTime, duration, isDefault, onFadeOutCallback);
}

MAKE_HOOK_MATCH(SongPreviewPlayer_OnDisable, &SongPreviewPlayer::OnDisable, void, SongPreviewPlayer* self) {
    DisableTurkeyGobbles(self);
    SongPreviewPlayer_OnDisable(self);
}

MAKE_HOOK_MATCH(SongPreviewPlayer_Update, &SongPreviewPlayer::Update, void, SongPreviewPlayer* self) {
    SongPreviewPlayer_Update(self);
    UpdateTurkeyGobbles(self);
}
// float BytesToFloat(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3) {
//     float out = 0;
//     uint8_t* tempPtr = reinterpret_cast<uint8_t*>(&out);

//     // Arm processors are usually little endian, so assume this operation is okay
//     tempPtr[0] = byte0;
//     tempPtr[1] = byte1;
//     tempPtr[2] = byte2;
//     tempPtr[3] = byte3;

//     return out;
// }

//MAKE_HOOK_MATCH(GameServerLobbyFlowCoordinator_DidActivate, &GlobalNamespace::GameServerLobbyFlowCoordinator::DidActivate, void, GlobalNamespace::GameServerLobbyFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
//    MenuMusic::menuMusicLoader.set_OriginalClip(self->_ambienceAudioClip);
//
//    self->_ambienceAudioClip = MenuMusic::menuMusicLoader.get_OriginalClip();
//    GameServerLobbyFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
//}

//MAKE_HOOK_MATCH(GameServerLobbyFlowCoordinator_DidDeactivate, &GlobalNamespace::GameServerLobbyFlowCoordinator::DidDeactivate, void, GlobalNamespace::GameServerLobbyFlowCoordinator* self, bool removedFromHierarchy, bool screenSystemDisabling) {
//   self->_ambienceAudioClip = MenuMusic::menuMusicLoader.get_OriginalClip();
//    GameServerLobbyFlowCoordinator_DidDeactivate(self, removedFromHierarchy, screenSystemDisabling);
//}

//MAKE_HOOK_MATCH(MultiplayerModeSelectionFlowCoordinator_DidActivate, &GlobalNamespace::MultiplayerModeSelectionFlowCoordinator::DidActivate, void, GlobalNamespace::MultiplayerModeSelectionFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
//    self->_ambienceAudioClip = MenuMusic::menuMusicLoader.get_OriginalClip();
//    MultiplayerModeSelectionFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
//}

//MAKE_HOOK_MATCH(MultiplayerModeSelectionFlowCoordinator_DidDeactivate, &GlobalNamespace::MultiplayerModeSelectionFlowCoordinator::DidDeactivate, void, GlobalNamespace::MultiplayerModeSelectionFlowCoordinator* self, bool removedFromHierarchy, bool screenSystemDisabling) {
//    self->_ambienceAudioClip = MenuMusic::menuMusicLoader.get_OriginalClip();
//    MultiplayerModeSelectionFlowCoordinator_DidDeactivate(self, removedFromHierarchy, screenSystemDisabling);
//}


//MAKE_HOOK_MATCH(NoteControllerInit, &GlobalNamespace::NoteController::Init, void, GlobalNamespace::NoteController *self, GlobalNamespace::NoteData *noteData, float worldRotation, UnityEngine::Vector3 moveStartPos, UnityEngine::Vector3 moveEndPos, UnityEngine::Vector3 jumpEndPos, float moveDuration, float jumpDuration, float jumpGravity) {
    //NoteControllerInit(self, noteData, worldRotation, moveStartPos, moveEndPos, jumpEndPos, moveDuration, jumpDuration, jumpGravity);

    //if (colorChanger) {
    //    colorChanger->UpdateColor(UnityEngine::Time::get_deltaTime());
    //}

    //auto renderers = self->GetComponentInChildren<UnityEngine::Renderer *>();
    //if (renderers) {
    //    for (auto m : renderers->get_materials()) {
    //        m->SetColor("_SimpleColor", colorChanger->currentColor);
     //       m->SetColor("_Color", colorChanger->currentColor);
        //}
    //}
//}


// MAKE_HOOK_MATCH(NoteControllerInit, &GlobalNamespace::NoteController::Init, void, GlobalNamespace::NoteController* self, GlobalNamespace::NoteData* noteData, float worldRotation, UnityEngine::Vector3 moveStartPos, UnityEngine::Vector3 moveEndPos, UnityEngine::Vector3 jumpEndPos, float moveDuration, float jumpDuration, float jumpGravity, float endRotation, float uniformScale, bool rotatesTowardsPlayer, bool useRandomRotation) {
//     NoteControllerInit(self, noteData, worldRotation, moveStartPos, moveEndPos, jumpEndPos, moveDuration, jumpDuration, jumpGravity, endRotation, uniformScale, rotatesTowardsPlayer, useRandomRotation);
//     if(!getModConfig().EnableBlocks.GetValue())
// 		return;

//     if (colorChanger) colorChanger->UpdateColor(UnityEngine::Time::get_deltaTime());


//     for(auto m : self->GetComponentInChildren<Renderer *>()->get_materials()) {
//         //m->SetColor("_SimpleColor", colorChanger->currentColor);
//         m->SetColor("_Color", colorChanger->currentColor);
//     }
// }

// Original Beat Saber neon sign remains untouched. No replacement-logo hook.

// Very quick and dirty way to override the menu light colors
MAKE_HOOK_MATCH(OverrideEnvironmentColors, &GlobalNamespace::LightWithIdManager::SetColorForId, void, GlobalNamespace::LightWithIdManager* self, int lightId, UnityEngine::Color color) {
    // Leave gameplay colors untouched. In the menu, tint environment lights
    // toward warm Thanksgiving harvest tones instead of forcing every
    // light to white (which erased the intended seasonal color treatment).
    if (!getModConfig().EnableHauntedEnvColors.GetValue() ||
        UnityEngine::SceneManagement::SceneManager::GetActiveScene().name == "GameCore") {
        OverrideEnvironmentColors(self, lightId, color);
        return;
    }

    const UnityEngine::Color harvestOrange(1.0f, 0.34f, 0.055f, color.a);
    const UnityEngine::Color goldenWheat(1.0f, 0.69f, 0.16f, color.a);
    const UnityEngine::Color tint = (lightId % 2 == 0) ? harvestOrange : goldenWheat;

    // Blend the original color with the seasonal tint to retain some variation.
    const float blend = 0.70f;
    const UnityEngine::Color seasonalColor(
        color.r * (1.0f - blend) + tint.r * blend,
        color.g * (1.0f - blend) + tint.g * blend,
        color.b * (1.0f - blend) + tint.b * blend,
        color.a
    );
    OverrideEnvironmentColors(self, lightId, seasonalColor);
}

namespace Thanksgiving::logoColors { void ApplyLogoColors(); }

MAKE_HOOK_MATCH(ThanksgivingSteadyEInit, &GlobalNamespace::FlickeringNeonSign::Start, void, GlobalNamespace::FlickeringNeonSign* self) {
    ThanksgivingSteadyEInit(self);
    INFO("[Thanksgiving E DIAG] FlickeringNeonSign::Start hook executed.");
    Thanksgiving::logoColors::ApplyLogoColors();
}


// Re-arm turkey audio when Solo song selection becomes active.
MAKE_HOOK_MATCH(ThanksgivingLevelSelectionDidActivate, &GlobalNamespace::LevelSelectionFlowCoordinator::DidActivate, void, GlobalNamespace::LevelSelectionFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    ThanksgivingLevelSelectionDidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    Thanksgiving::Audio::EnterSoloSelection();
}

MAKE_HOOK_MATCH(AutumnLeavesInit, &GlobalNamespace::MainMenuViewController::DidActivate, void, GlobalNamespace::MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    AutumnLeavesInit(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    Thanksgiving::Audio::EnterMenu(self);

    if(!firstActivation) return;
    StartMenuFloorRestoration();

    Thanksgiving::logoColors::ApplyLogoColors();


    if(getModConfig().EnableAutumnLeaves.GetValue()) Thanksgiving::autumnLeaves::CreateAutumnLeaves();
    if(getModConfig().EnablePumpkinAccents.GetValue()) Thanksgiving::pumpkinAccents::CreatePumpkinAccents();
}


// Called at the early stages of game loading
MOD_EXTERN_FUNC void setup(CModInfo *info) noexcept {
    *info = modInfo.to_c();

    getModConfig().Init(modInfo);

    // File logging
    Paper::Logger::RegisterFileContextId(PaperLogger.tag);

    INFO("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
MOD_EXTERN_FUNC void late_load() noexcept {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();


    INFO("Installing hooks...");
    //INSTALL_HOOK(PaperLogger, NoteControllerInit);
    // Keep the game's original Beat Saber logo; no logo replacement code is compiled.
    INSTALL_HOOK(PaperLogger, OverrideEnvironmentColors);
    INSTALL_HOOK(PaperLogger, MenuFloorSetPropertyBlock);
    INSTALL_HOOK(PaperLogger, ThanksgivingSteadyEInit);
    INSTALL_HOOK(PaperLogger, AutumnLeavesInit);
    INSTALL_HOOK(PaperLogger, ThanksgivingLevelSelectionDidActivate);

    // v1.0.4 stability test: audio lifecycle hook disabled
    // INSTALL_HOOK(PaperLogger, SongPreviewPlayer_OnEnable);
    // v1.0.4 stability test: audio lifecycle hook disabled
    // INSTALL_HOOK(PaperLogger, SongPreviewPlayer_CrossfadeTo);
    // v1.0.4 stability test: audio lifecycle hook disabled
    // INSTALL_HOOK(PaperLogger, SongPreviewPlayer_Update);
    // v1.0.4 stability test: audio lifecycle hook disabled
    // INSTALL_HOOK(PaperLogger, SongPreviewPlayer_OnDisable);
    //INSTALL_HOOK(PaperLogger, GameServerLobbyFlowCoordinator_DidActivate);
    //INSTALL_HOOK(PaperLogger, GameServerLobbyFlowCoordinator_DidDeactivate);
    //INSTALL_HOOK(PaperLogger, MultiplayerModeSelectionFlowCoordinator_DidActivate);
    //INSTALL_HOOK(PaperLogger, MultiplayerModeSelectionFlowCoordinator_DidDeactivate);



    INFO("Installed all hooks!");
}
