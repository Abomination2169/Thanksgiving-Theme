#include "Audio/player.hpp"
#include "main.hpp"
#include "logging.hpp"
#include "c.hpp"
#include "custom-types/shared/delegate.hpp"
#include "System/Threading/Tasks/Tasks.hpp"
#include "GlobalNamespace/MediaAsyncLoader.hpp"





#include <string>

// Thanksgiving turkey gobble scheduler.
// This AudioSource is separate from the menu music source.

#include "UnityEngine/AudioSource.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/Time.hpp"
#include <cstdlib>

static const std::string highGobblePath =
    "/sdcard/ModData/com.beatgames.beatsaber/Mods/Thanksgiving/Thanksgiving-Gobble-High-Note-Fade.ogg";

static const std::string quietGobblePath =
    "/sdcard/ModData/com.beatgames.beatsaber/Mods/Thanksgiving/Thanksgiving-Gobble-Quiet-Phrase.ogg";

static UnityW<UnityEngine::AudioClip> highGobbleClip;
static UnityW<UnityEngine::AudioClip> quietGobbleClip;

static UnityEngine::AudioSource* gobbleSource = nullptr;
static bool gobbleLoadingStarted = false;
static float nextGobbleTime = 0.0f;

static void LoadGobbleClip(
    const std::string& path,
    UnityW<UnityEngine::AudioClip>* destination
) {
    auto loader = GlobalNamespace::MediaAsyncLoader::New_ctor();
    auto task = loader->LoadAudioClipFromFilePathAsync(path);

    if (!task) {
        PaperLogger.error("Turkey gobble audio task creation failed.");
        return;
    }

    task->GetAwaiter().OnCompleted(
        custom_types::MakeDelegate<System::Action*>(
            std::function([task, destination]() {
                auto clip = task->get_Result();

                if (clip) {
                    *destination = clip;
                    PaperLogger.info("Turkey gobble audio loaded.");
                } else {
                    PaperLogger.error("Turkey gobble audio returned empty.");
                }
            })
        )
    );
}

void InitializeTurkeyGobbles(
    GlobalNamespace::SongPreviewPlayer* player
) {
    if (!player) return;

    if (!gobbleSource) {
        auto gameObject = player->get_gameObject();
        if (!gameObject) return;

        gobbleSource =
            gameObject->AddComponent<UnityEngine::AudioSource*>();

        if (!gobbleSource) {
            PaperLogger.error("Could not create turkey AudioSource.");
            return;
        }

        gobbleSource->set_playOnAwake(false);
        gobbleSource->set_loop(false);
        gobbleSource->set_spatialBlend(0.0f);
        gobbleSource->set_volume(1.0f);

        PaperLogger.info("Turkey gobble AudioSource initialized.");
    }

    if (gobbleLoadingStarted) return;

    gobbleLoadingStarted = true;

    if (fileexists(highGobblePath)) {
        LoadGobbleClip(highGobblePath, &highGobbleClip);
    } else {
        PaperLogger.error("High-note turkey gobble file missing.");
    }

    if (fileexists(quietGobblePath)) {
        LoadGobbleClip(quietGobblePath, &quietGobbleClip);
    } else {
        PaperLogger.error("Quiet turkey gobble file missing.");
    }
}

void UpdateTurkeyGobbles() {
    if (!getModConfig().EnableTurkeyGobbles.GetValue()) return;

    if (!gobbleSource || !highGobbleClip || !quietGobbleClip) return;

    const float now = UnityEngine::Time::get_unscaledTime();

    if (nextGobbleTime <= 0.0f) {
        nextGobbleTime =
            now + 5.0f + static_cast<float>(std::rand() % 4);
        return;
    }

    if (now < nextGobbleTime) return;

    UnityEngine::AudioClip* selectedClip =
        (std::rand() % 2 == 0)
        ? highGobbleClip.ptr()
        : quietGobbleClip.ptr();

    if (selectedClip) {
        gobbleSource->PlayOneShot(selectedClip, 0.22f);
        PaperLogger.info("Turkey gobble played.");
    }

    nextGobbleTime =
        now + 15.0f + static_cast<float>(std::rand() % 11);
}