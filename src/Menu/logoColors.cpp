#include "logging.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/SpriteRenderer.hpp"
#include "UnityEngine/Color.hpp"

#include "GlobalNamespace/TubeBloomPrePassLight.hpp"
#include "GlobalNamespace/FlickeringNeonSign.hpp"

namespace Thanksgiving::logoColors {

void ApplyLogoColors() {
    INFO("[Thanksgiving E DIAG] ApplyLogoColors executed.");
    const UnityEngine::Color orange(1.0f, 0.24f, 0.035f, 1.0f);
    const UnityEngine::Color purple(1.0f, 0.68f, 0.12f, 1.0f);

    // BEAT neon lights
    const char* beatLights[] = {
        "BNeon", "ENeon", "ANeon", "TNeon"
    };

    for (const char* name : beatLights) {
        auto obj = UnityEngine::GameObject::Find(name);
        if (!obj) continue;

        auto neon = obj->GetComponent<GlobalNamespace::TubeBloomPrePassLight*>();
        if (neon) neon->set_color(orange);
    }

    // Preserve the original Beat Saber E and its controller.
    auto eFlickering = UnityEngine::GameObject::Find("EFlickering");

    if (eFlickering) {
        eFlickering->active = true;

        auto flicker =
            eFlickering->GetComponent<GlobalNamespace::FlickeringNeonSign*>();

        if (flicker) {
            flicker->__cordl_internal_set__spriteOnColor(orange);
            flicker->__cordl_internal_set__lightOnColor(orange);
        }
    }
    // SABER neon lighting
    auto saberNeon = UnityEngine::GameObject::Find("SaberNeon");
    if (saberNeon) {
        auto neon = saberNeon->GetComponent<GlobalNamespace::TubeBloomPrePassLight*>();
        if (neon) neon->set_color(purple);
    }

    // SABER visible lettering
    auto saberLogo = UnityEngine::GameObject::Find("SaberLogo");
    if (saberLogo) {
        auto sprite = saberLogo->GetComponent<UnityEngine::SpriteRenderer*>();
        if (sprite) sprite->set_color(purple);
    }
}

}
