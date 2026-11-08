#include "Audio/player.hpp"
#include "main.hpp"
#include "logging.hpp"
#include "c.hpp"
#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/delegate.hpp"
#include "System/Threading/Tasks/Tasks.hpp"
#include "GlobalNamespace/MediaAsyncLoader.hpp"
#include "UnityEngine/AudioSource.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/Time.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include <functional>
#include <mutex>
#include <queue>
#include <cstdlib>
#include <string>

DECLARE_CLASS_CODEGEN(Thanksgiving, GobbleMenuTicker, UnityEngine::MonoBehaviour)
{
    DECLARE_INSTANCE_METHOD(void, Update);
    DECLARE_INSTANCE_METHOD(void, OnDisable);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);
};
DEFINE_TYPE(Thanksgiving, GobbleMenuTicker);

namespace {
constexpr const char* kPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/Thanksgiving/Thanksgiving-Gobble-Quiet-Phrase.ogg";
std::mutex mutex;
std::queue<std::function<void()>> completions;
UnityW<UnityEngine::AudioSource> source;
UnityW<UnityEngine::AudioClip> clip;
UnityW<Thanksgiving::GobbleMenuTicker> ticker;
bool active = false;
bool loading = false;
float nextTime = 0.0f;

void QueueLoad() {
    if (loading || clip) return;
    if (!fileexists(kPath)) {
        PaperLogger.error("v1.0.5 gobble file missing: {}", kPath);
        return;
    }
    loading = true;
    auto loader = GlobalNamespace::MediaAsyncLoader::New_ctor();
    auto task = loader->LoadAudioClipFromFilePathAsync(std::string(kPath));
    if (!task) { loading = false; return; }
    task->GetAwaiter().OnCompleted(custom_types::MakeDelegate<System::Action*>(
        std::function([task]() {
            std::lock_guard<std::mutex> guard(mutex);
            completions.emplace([task]() {
                auto result = task->get_Result();
                if (result) { clip = result; PaperLogger.info("v1.0.5 gobble preloaded"); }
                else PaperLogger.error("v1.0.5 gobble load returned null");
                loading = false;
            });
        })
    ));
}
void Stop() {
    active = false;
    if (source && source->get_isPlaying()) source->Stop();
    nextTime = 0.0f;
}
}

void Thanksgiving::Audio::EnterMenu(GlobalNamespace::MainMenuViewController* menu) {
    if (!menu) return;

    // The scheduler and audio source must not live on the menu GameObject:
    // Beat Saber can disable that object during gameplay.
    if (!ticker || !ticker->get_gameObject()) {
        auto controller = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("ThanksgivingGobbleController"));
        if (!controller) {
            PaperLogger.error("Gobble controller creation failed");
            return;
        }
        UnityEngine::Object::DontDestroyOnLoad(controller);
        ticker = controller->AddComponent<Thanksgiving::GobbleMenuTicker*>();
        source = controller->AddComponent<UnityEngine::AudioSource*>();
        if (!ticker || !source) {
            PaperLogger.error("Gobble components creation failed");
            return;
        }
        source->set_playOnAwake(false);
        source->set_loop(false);
        source->set_spatialBlend(0.0f);
        source->set_volume(0.22f);
    }

    // DidActivate is the authoritative rearm event, including returns from songs.
    active = true;
    nextTime = UnityEngine::Time::get_unscaledTime() + 5.0f;
    QueueLoad();
}

void Thanksgiving::Audio::EnterSoloSelection() {
    // Controller was initialized by the main menu. Do not create one on a
    // transient level-selection GameObject.
    if (!ticker || !source) return;
    if (!getModConfig().EnableTurkeyGobbles.GetValue()) return;
    // Avoid restarting the timer when moving between menu screens while
    // gobbles are already active. Returning from GameCore sets active=false.
    if (active) return;
    active = true;
    nextTime = UnityEngine::Time::get_unscaledTime() + 5.0f;
    QueueLoad();
}

void Thanksgiving::Audio::ExitMenu() { Stop(); }

void Thanksgiving::Audio::Tick() {
    std::queue<std::function<void()>> ready;
    {
        std::lock_guard<std::mutex> guard(mutex);
        std::swap(ready, completions);
    }
    while (!ready.empty()) { ready.front()(); ready.pop(); }

    if (!ticker || !source) return;
    if (!getModConfig().EnableTurkeyGobbles.GetValue()) { Stop(); return; }

    // GameCore is never allowed to play turkey audio. The persistent ticker
    // remains alive so asynchronous clip completions can still be drained.
    const auto scene = UnityEngine::SceneManagement::SceneManager::GetActiveScene();
    if (const_cast<UnityEngine::SceneManagement::Scene&>(scene).get_name() == "GameCore") {
        if (active) Stop();
        return;
    }
    if (!active) return;
    if (!clip) { QueueLoad(); return; }

    const float now = UnityEngine::Time::get_unscaledTime();
    if (now < nextTime) return;
    if (!source->get_isPlaying()) {
        source->set_clip(clip.ptr());
        source->Play();
        PaperLogger.info("v1.0.5 persistent gobble played");
    }
    nextTime = now + 15.0f + float(std::rand() % 11);
}

void Thanksgiving::GobbleMenuTicker::Update() { Thanksgiving::Audio::Tick(); }
void Thanksgiving::GobbleMenuTicker::OnDisable() { Thanksgiving::Audio::ExitMenu(); }
void Thanksgiving::GobbleMenuTicker::OnDestroy() {
    Thanksgiving::Audio::ExitMenu();
    ticker = nullptr;
    source = nullptr;
}

// Legacy hooks remain disabled. Their entry points are retained only for linking.
void InitializeTurkeyGobbles(GlobalNamespace::SongPreviewPlayer*) {}
void DisableTurkeyGobbles(GlobalNamespace::SongPreviewPlayer*) {}
void UpdateTurkeyGobbles(GlobalNamespace::SongPreviewPlayer*) {}