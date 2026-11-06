#include "AutumnLeaves/autumnLeaves.hpp"
#include "main.hpp"
#include "assets.hpp"
#include "UnityEngine/Shader.hpp"
#include "logging.hpp"


#include "AutumnLeaves/particleController.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/ParticleSystemRenderer.hpp"
#include "UnityEngine/Material.hpp"

#include "bsml/shared/Helpers/utilities.hpp"

#include "UnityEngine/Shader.hpp"
#include "UnityEngine/Color.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace Flair;

namespace Thanksgiving::autumnLeaves {

    SafePtrUnity<GameObject> dustGo;
    SafePtrUnity<GameObject> autumnLeavesGo;

    void CreateAutumnLeaves() {
        // Don't create if already created
        if(autumnLeavesGo) return;

        // Find the dust GameObject for a template ParticleSystem
        if(!dustGo) for (auto particle : Resources::FindObjectsOfTypeAll<ParticleSystem*>())
            if (particle->name == "DustPS") {
                dustGo = particle->gameObject;
                break;
            }

        // If the template dust wasn't found, don't create the leaves
        if(!dustGo) return;


        // Create the autumnLeaves GameObject from the dust
        dustGo->active = false; // Dust and leaves at the same time looks busy, so
        autumnLeavesGo = GameObject::Instantiate(dustGo->gameObject);
        autumnLeavesGo->name = "AutumnLeavesPS";
        autumnLeavesGo->active = true;
        // GameObject::DontDestroyOnLoad(autumnLeavesGo->gameObject); // This line will enable autumnLeaves in game

        // Get components from the autumnLeaves
        UnityEngine::ParticleSystem* autumnLeaves = autumnLeavesGo->GetComponent<ParticleSystem*>();
        UnityEngine::ParticleSystemRenderer* particleRenderer = autumnLeavesGo->GetComponent<UnityEngine::ParticleSystemRenderer*>();

        // Set up autumnLeavesGo where it needs to be
        autumnLeavesGo->transform->parent = nullptr;
        autumnLeavesGo->transform->position = UnityEngine::Vector3(0.0f, 7.0f, 0.0f);
        autumnLeavesGo->transform->rotation = UnityEngine::Quaternion::Euler(UnityEngine::Vector3(0.0f, 0.0f, 0.0f));

        // Use Flair's ParticleController to modify ParticleSystem properties to warp the dust into autumnLeaves
        ParticleController::MainModule::set_startLifetime(autumnLeaves, 14.0f);
        ParticleController::MainModule::set_startSize(autumnLeaves, 0.18);
        ParticleController::MainModule::set_maxParticles(autumnLeaves, 150);
        ParticleController::MainModule::set_gravityModifier(autumnLeaves, 0.025f);
// Preserve the original RGB colors of the autumn leaves.
ParticleController::MinMaxGradient leafColor{};

leafColor.mode = ParticleController::GradientMode::Color;
leafColor.colorMin = UnityEngine::Color(1.0f, 1.0f, 1.0f, 1.0f);
leafColor.colorMax = UnityEngine::Color(1.0f, 1.0f, 1.0f, 1.0f);

ParticleController::MainModule::set_startColor(
    autumnLeaves,
    leafColor
);

// Disable inherited particle color animation.
ParticleController::ColorOverLifetimeModule::set_enabled(
    autumnLeaves,
    false
);

ParticleController::ColorBySpeedModule::set_enabled(
    autumnLeaves,
    false
);
        ParticleController::EmissionModule::set_enabled(autumnLeaves, true); // ParticleTuner compatibility
        ParticleController::EmissionModule::set_rateOverTime(autumnLeaves, 5);
        ParticleController::ShapeModule::set_shapeType(autumnLeaves, ParticleController::ShapeType::Box);
        ParticleController::ShapeModule::set_position(autumnLeaves, UnityEngine::Vector3(0.0f, 0.0f, 0.0f));
        ParticleController::ShapeModule::set_rotation(autumnLeaves, UnityEngine::Vector3(0.0f, 0.0f, 0.0f));
        ParticleController::ShapeModule::set_scale(autumnLeaves, UnityEngine::Vector3(20.0f, 1.0f, 20.0f));
        ParticleController::CollisionModule::set_enabled(autumnLeaves, true);
        ParticleController::CollisionModule::set_type(autumnLeaves, ParticleController::CollisionType::World);
        ParticleController::CollisionModule::set_collidesWith(autumnLeaves, UnityEngine::LayerMask(0xffffffff));
        ParticleController::CollisionModule::set_dampen(autumnLeaves, 0.08f);
        ParticleController::CollisionModule::set_bounce(autumnLeaves, 0.0f);
        ParticleController::CollisionModule::set_lifetimeLoss(autumnLeaves, 0.02);
        const int numLeafSprites = 5;
        particleRenderer->material->mainTexture = BSML::Utilities::LoadTextureRaw(static_cast<ArrayW<uint8_t>>(IncludedAssets::autumn_leaves_png));
// Verified Quest fix: Sprites/Default samples the RGBA leaf spritesheet in full color.
// Do not use Particles/Standard Unlit here: it rendered the leaves white.
auto leafShader = UnityEngine::Shader::Find("Sprites/Default");
if (leafShader && particleRenderer) {
    auto leafMaterial = particleRenderer->get_material();
    if (leafMaterial) {
        leafMaterial->set_shader(leafShader);
        leafMaterial->SetTexture(
            StringW("_MainTex"),
            BSML::Utilities::LoadTextureRaw(static_cast<ArrayW<uint8_t>>(IncludedAssets::autumn_leaves_png))
        );
        leafMaterial->SetColor(StringW("_Color"), UnityEngine::Color(1, 1, 1, 1));
    }
} else {
    WARNING("Autumn leaves: Sprites/Default shader not found; using original material");
}
        ParticleController::TextureSheetAnimationModule::set_enabled(autumnLeaves, true);
        ParticleController::TextureSheetAnimationModule::set_mode(autumnLeaves, ParticleController::AnimationMode::Grid);
        ParticleController::TextureSheetAnimationModule::set_numTilesX(autumnLeaves, numLeafSprites);
        ParticleController::TextureSheetAnimationModule::set_numTilesY(autumnLeaves, 1);
        ParticleController::TextureSheetAnimationModule::set_frameOverTime(autumnLeaves, ParticleController::MinMaxCurve(0.0f, numLeafSprites));
        ParticleController::TextureSheetAnimationModule::set_startFrame(autumnLeaves, ParticleController::MinMaxCurve(0.0f, numLeafSprites));
        ParticleController::TextureSheetAnimationModule::set_cycleCount(autumnLeaves, 0);
    }

    void DestroyAutumnLeaves() {
        if(autumnLeavesGo) GameObject::Destroy(autumnLeavesGo->gameObject);
        if(dustGo) dustGo->active = true;
    }

}
