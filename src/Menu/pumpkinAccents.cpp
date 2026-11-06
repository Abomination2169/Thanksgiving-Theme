#include "Menu/pumpkinAccents.hpp"
#include "bsml/shared/BSML.hpp"
#include "main.hpp"
#include "logging.hpp"
#include "assets.hpp"
#include "bsml/shared/Helpers/utilities.hpp"
#include "UnityEngine/SpriteRenderer.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/PrimitiveType.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Renderer.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Bounds.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Shader.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/AssetBundle.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <span>

using namespace UnityEngine;

// Keep the original namespace and entry points used by main.cpp.
namespace Thanksgiving::pumpkinAccents {
    safe_ptr<GameObject*> pumpkinAccents;

    struct HarvestSpot {
        Vector3 originalPosition;
        float scale;
        float angle;
    };

    static GameObject* MakePart(Transform* parent, PrimitiveType type,
                                const char* name, Vector3 position,
                                Vector3 scale, Material* material) {
        GameObject* part = GameObject::CreatePrimitive(type);
        part->set_name(name);
        part->get_transform()->SetParent(parent, false);
        part->get_transform()->set_localPosition(position);
        part->get_transform()->set_localScale(scale);
        auto* renderer = part->GetComponent<MeshRenderer*>();
        if (renderer && material) renderer->set_sharedMaterial(material);
        auto* collider = part->GetComponent<Collider*>();
        if (collider) Object::Destroy(collider);
        return part;
    }

    // Thanksgiving v1.4: Extract the embedded Android AssetBundle.
    static UnityEngine::AssetBundle* LoadThanksgivingHarvestBundle() {
        // Reuse the AssetBundle throughout this game session.
        static UnityEngine::AssetBundle* cachedBundle = nullptr;

        if (cachedBundle) {
            return cachedBundle;
        }

        namespace fs = std::filesystem;

        const std::string directory =
            "/sdcard/Android/data/com.beatgames.beatsaber/files/Mods/Thanksgiving";

        const std::string path = directory + "/thanksgiving_harvest";

        std::error_code error;
        fs::create_directories(directory, error);

        if (error) {
            ERROR("[Thanksgiving v1.4] Cannot create AssetBundle directory: {}", error.message());
            return nullptr;
        }

        auto bytes = static_cast<std::span<uint8_t>>(
            IncludedAssets::thanksgiving_harvest
        );

        if (bytes.empty()) {
            ERROR("[Thanksgiving v1.4] Embedded AssetBundle is empty.");
            return nullptr;
        }

        // Write to a temporary file before replacing the destination.
        const std::string temporaryPath = path + ".tmp";

        {
            std::ofstream output(
                temporaryPath,
                std::ios::binary | std::ios::trunc
            );

            if (!output) {
                ERROR("[Thanksgiving v1.4] Cannot open temporary AssetBundle file.");
                return nullptr;
            }

            output.write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size())
            );

            output.flush();

            if (!output.good()) {
                ERROR("[Thanksgiving v1.4] Failed writing AssetBundle data.");
                return nullptr;
            }
        }

        fs::rename(temporaryPath, path, error);

        if (error) {
            ERROR("[Thanksgiving v1.4] Cannot finalize AssetBundle: {}", error.message());
            return nullptr;
        }

        INFO("[Thanksgiving v1.4] AssetBundle extracted successfully.");

        auto bundle = UnityEngine::AssetBundle::LoadFromFile(path);

        if (!bundle) {
            ERROR("[Thanksgiving v1.4] Unity failed to load AssetBundle.");
            return nullptr;
        }

        INFO("[Thanksgiving v1.4] Unity AssetBundle loaded successfully.");

        cachedBundle = bundle;
        return cachedBundle;
    }
    void CreatePumpkinAccents() {
        if (pumpkinAccents) return;
        GameObject* environment = GameObject::Find("DefaultMenuEnvironment");
        if (!environment) return;

        GameObject* note = GameObject::Find("Note");
        Material* baseMaterial = nullptr;
        if (note) {
            auto* renderer = note->GetComponent<MeshRenderer*>();
            if (renderer) baseMaterial = renderer->get_sharedMaterial();
        }
        if (!baseMaterial) {
            auto shader = Shader::Find("Standard");
            if (!shader) shader = Shader::Find("Sprites/Default");
            if (!shader) return;
            baseMaterial = Material::New_ctor(shader);
        }

        pumpkinAccents = GameObject::New_ctor("ThanksgivingHarvestAccents");
        pumpkinAccents->get_transform()->SetParent(environment->get_transform(), false);

        // Two independent pumpkin canvases, following the earlier
       // earlier seasonal mod positioning pattern.
       // Preserve the original Beat Saber neon logo and flickering E.
       auto* pumpkinSignPlane = GameObject::New_ctor("ThanksgivingPumpkinSignPlane");
       pumpkinSignPlane->get_transform()->SetParent(pumpkinAccents->get_transform(), false);
       pumpkinSignPlane->get_transform()->set_localPosition(Vector3(0.0f, 16.6f, 26.1f));

       auto makePumpkin = [&](const char* name, ArrayW<uint8_t> png, float side) {
           auto* canvas = BSML::Lite::CreateCanvas();
           canvas->set_name(name);
           canvas->get_transform()->SetParent(pumpkinSignPlane->get_transform(), false);
           canvas->get_transform()->set_localPosition(Vector3(side > 0 ? 9.0f : -10.0f, 0.4f, 0.0f));

           BSML::Lite::CreateImage(
               canvas,
               BSML::Utilities::LoadSpriteRaw(png),
               UnityEngine::Vector2(0.0f, 0.0f),
               UnityEngine::Vector2(291.0f, 291.0f)
           );
       };

       makePumpkin(
           "ThanksgivingPumpkinLeft",
           static_cast<ArrayW<uint8_t>>(IncludedAssets::pumpkin_left_png),
           -1.0f
       );

       makePumpkin(
           "ThanksgivingPumpkinRight",
           static_cast<ArrayW<uint8_t>>(IncludedAssets::pumpkin_right_png),
           1.0f
       );
        // Thanksgiving harvest materials; preserve the existing placement system.
        auto* wood = Object::Instantiate(baseMaterial);
        auto* darkWood = Object::Instantiate(baseMaterial);
        auto* straw = Object::Instantiate(baseMaterial);
        auto* strawHighlight = Object::Instantiate(baseMaterial);
        wood->SetColor("_Color", Color(0.48f, 0.26f, 0.105f, 1.0f));
        darkWood->SetColor("_Color", Color(0.28f, 0.13f, 0.055f, 1.0f));
        straw->SetColor("_Color", Color(0.82f, 0.57f, 0.20f, 1.0f));
        strawHighlight->SetColor("_Color", Color(0.98f, 0.76f, 0.36f, 1.0f));

        // Original twenty locations, including the original division by two.
        const HarvestSpot spots[] = {
            {Vector3(-5.5f, -1.3f, 16.1f), 1.00f, 12.0f},
            {Vector3(16.4f, -1.3f, 0.0f), 0.88f, -25.0f},
            {Vector3(6.5f, -1.3f, 16.4f), 1.12f, 31.0f},
            {Vector3(15.9f, -1.0f, 10.8f), 0.96f, -12.0f},
            {Vector3(-18.3f, -0.8f, -11.4f), 1.18f, 38.0f},
            {Vector3(-9.0f, -1.7f, -7.7f), 0.85f, -36.0f},
            {Vector3(-18.1f, -1.3f, 2.7f), 1.07f, 18.0f},
            {Vector3(-2.5f, -1.3f, -14.2f), 0.93f, -16.0f},
            {Vector3(-15.7f, -1.3f, 12.0f), 1.13f, 27.0f},
            {Vector3(10.0f, -1.3f, -10.5f), 0.91f, -28.0f},
            {Vector3(-15.1f, -1.3f, 26.5f), 1.28f, 42.0f},
            {Vector3(25.4f, -0.9f, -15.5f), 1.36f, -43.0f},
            {Vector3(7.0f, -1.3f, 29.5f), 1.22f, 11.0f},
            {Vector3(29.9f, -1.3f, 19.2f), 1.40f, -32.0f},
            {Vector3(34.4f, -1.3f, 0.5f), 1.25f, 22.0f},
            {Vector3(-34.3f, -0.9f, 16.6f), 1.32f, -19.0f},
            {Vector3(-36.0f, -1.3f, -6.0f), 1.17f, 33.0f},
            {Vector3(-15.2f, -1.3f, -25.3f), 1.38f, -10.0f},
            {Vector3(7.9f, -1.3f, -30.2f), 1.30f, 45.0f},
            {Vector3(-25.8f, -0.7f, 7.0f), 1.04f, -38.0f}
        };

        // Thanksgiving v1.4: load the embedded 3D decorations.
        auto* harvestBundle = LoadThanksgivingHarvestBundle();

        for (int i = 0; i < 20; ++i) {
            const auto& spot = spots[i];
            GameObject* harvest = GameObject::New_ctor("ThanksgivingHarvestDisplay");
            harvest->get_transform()->SetParent(pumpkinAccents->get_transform(), false);
            harvest->get_transform()->set_localPosition(
                Vector3(spot.originalPosition.x / 2.0f,
                        spot.originalPosition.y / 2.0f,
                        spot.originalPosition.z / 2.0f));
            harvest->get_transform()->set_localRotation(
                Quaternion::Euler(Vector3(0.0f, spot.angle, 0.0f)));
                        // Varied harvest display sizes.
            static const float harvestSizeVariation[20] = {
                1.00f, 0.85f, 1.20f, 0.90f, 1.15f,
                1.05f, 0.80f, 1.25f, 0.95f, 1.10f,
                0.85f, 1.20f, 1.00f, 0.90f, 1.25f,
                0.85f, 1.15f, 0.80f, 1.10f, 0.95f
            };

            float variedScale = spot.scale * harvestSizeVariation[i];

            harvest->get_transform()->set_localScale(
                Vector3(variedScale, variedScale, variedScale));
            Transform* root = harvest->get_transform();
            // Thanksgiving v1.4: optimized prefab placement.
            const char* prefabPath = nullptr;

            // Varied harvest arrangement: 0=rectangular, 1=round, 2=fence.
            static const int harvestVariety[20] = {
                0, 2, 1, 0, 1,
                2, 0, 1, 2, 0,
                1, 0, 2, 1, 0,
                2, 1, 0, 2, 1
            };


            switch (harvestVariety[i]) {
                case 0:
                    prefabPath = "Assets/Prefabs/Thanksgiving_Rectangular_Hay_Bale.prefab";
                    break;
                case 1:
                    prefabPath = "Assets/Prefabs/Thanksgiving_Round_Hay_Bale.prefab";
                    break;
                default:
                    prefabPath = "Assets/Prefabs/Thanksgiving_Rustic_Fence.prefab";
                    break;
            }

            // [Thanksgiving DISPLAY10 CORN RESTORE v1]
            // Restore the existing corn stalk prefab as Display 10's main decoration.
            if (i == 9) {
                prefabPath = "Assets/Prefabs/Thanksgiving_Single_Corn_Stalk.prefab";
            }
            // Six additional clustered hay bales, reusing existing prefabs.
            // Each is parented to the current harvest display.
            static const int clusterPositions[6] = {0, 3, 6, 9, 14, 17};

            for (int clusterIndex = 0; clusterIndex < 6; ++clusterIndex) {
                if (i != clusterPositions[clusterIndex] || !harvestBundle) continue;
                if (i == 6 && clusterIndex == 2) continue;
        if (i == 3 && clusterIndex == 1) continue; // Display 4: remove only far additional rectangular bale // [Thanksgiving DISPLAY7 EXTRA REMOVE TEST v1]

                const char* extraPrefabPath =
                    (clusterIndex % 2 == 0)
                        ? (clusterIndex == 0 ? "Assets/Prefabs/Thanksgiving_Wooden_Crate.prefab" : "Assets/Prefabs/Thanksgiving_Round_Hay_Bale.prefab") /* [Thanksgiving CENTER CLUSTER CRATE TEST v1] */
                        : "Assets/Prefabs/Thanksgiving_Rectangular_Hay_Bale.prefab";

                // [Thanksgiving DISPLAY18 SMALL CRATE TEST v1]
                // Display 18 (i=17, clusterIndex=5) has the extra rectangular hay bale.
                // Swap only its prefab, retaining existing position, rotation and scale.
                if (i == 17 && clusterIndex == 5) {
                    extraPrefabPath = "Assets/Prefabs/Thanksgiving_Wooden_Crate.prefab";
                }
                auto extraPrefab = harvestBundle->LoadAsset(extraPrefabPath);

                if (extraPrefab) {
                    auto extraInstance = UnityEngine::Object::Instantiate(
                        extraPrefab.ptr(), root, false);

                    if (extraInstance) {
                        auto* extraObject =
                            il2cpp_utils::try_cast<UnityEngine::GameObject>(
                                extraInstance.ptr()).value_or(nullptr);

                        if (extraObject) {
                            auto extraTransform = extraObject->get_transform();

                            extraTransform->set_localPosition(
                                Vector3(2.85f, 0.0f, 1.65f));

                            extraTransform->set_localRotation(
                                Quaternion::Euler(
                                    Vector3(0.0f, 25.0f + clusterIndex * 13.0f, 0.0f)));

                            extraTransform->set_localScale(
                                Vector3(0.72f, 0.72f, 0.72f));

                            if (baseMaterial) {
                                auto extraRenderers =
                                    extraObject->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);

                                for (auto* renderer : extraRenderers) {
                                    if (!renderer) continue;

                                    auto* material =
                                        UnityEngine::Object::Instantiate(baseMaterial);

                                    material->SetColor(
                                        "_Color",
                                        UnityEngine::Color(0.75f, 0.52f, 0.20f, 1.0f));

                                    renderer->set_sharedMaterial(material);
                                    renderer->set_enabled(true);
                                }
                            }

                            INFO("[Thanksgiving CLUSTER] Added extra hay bale at position {}", i + 1);
                        }
                    }
                }
            }

            // Six additional wooden crates beside existing harvest displays.
            static const int cratePositions[5] = {1, 4, 10, 13, 18}; // [Thanksgiving REAR CRATE REMOVE TEST v1] display 8 excluded

            for (int crateIndex = 0; crateIndex < 5; ++crateIndex) {
                if (i != cratePositions[crateIndex] || !harvestBundle) continue;

                auto cratePrefab = harvestBundle->LoadAsset(
                    "Assets/Prefabs/Thanksgiving_Wooden_Crate.prefab");

                if (!cratePrefab) {
                    WARNING("[Thanksgiving CRATE] Wooden crate prefab unavailable at position {}", i + 1);
                    continue;
                }

                auto crateInstance = UnityEngine::Object::Instantiate(
                    cratePrefab.ptr(), root, false);

                if (!crateInstance) {
                    WARNING("[Thanksgiving CRATE] Instantiation failed at position {}", i + 1);
                    continue;
                }

                auto* crateObject =
                    il2cpp_utils::try_cast<UnityEngine::GameObject>(
                        crateInstance.ptr()).value_or(nullptr);

                if (!crateObject) continue;

                auto crateTransform = crateObject->get_transform();

                crateTransform->set_localPosition(
                    Vector3(-1.35f, 0.0f, 0.85f));

                crateTransform->set_localRotation(
                    Quaternion::Euler(
                        Vector3(0.0f, 20.0f + crateIndex * 23.0f, 0.0f)));

                crateTransform->set_localScale(
                    Vector3(0.75f, 0.75f, 0.75f));

                if (baseMaterial) {
                    auto crateRenderers =
                        crateObject->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);

                    for (auto* renderer : crateRenderers) {
                        if (!renderer) continue;

                        auto rendererObject = renderer->get_gameObject();

                        std::string rendererName = rendererObject
                            ? static_cast<std::string>(rendererObject->get_name())
                            : "";

                        UnityEngine::Color crateColor(0.48f, 0.30f, 0.17f, 1.0f);

                        if (rendererName.find("Sun_faded") != std::string::npos) {
                            crateColor = UnityEngine::Color(0.65f, 0.45f, 0.27f, 1.0f);
                        } else if (rendererName.find("Dark_oak") != std::string::npos) {
                            crateColor = UnityEngine::Color(0.30f, 0.18f, 0.10f, 1.0f);
                        }

                        auto* material =
                            UnityEngine::Object::Instantiate(baseMaterial);

                        material->SetColor("_Color", crateColor);

                        renderer->set_sharedMaterial(material);
                        renderer->set_enabled(true);
                    }
                }

                INFO("[Thanksgiving CRATE] Added wooden crate at position {}", i + 1);
            }
            // Thanksgiving: exactly four individual corn stalks.
            static const int cornPositions[4] = { 2, 5, 11, 16 }; // [Thanksgiving SPACING v1] 1-based: 3, 6, 12, 17

            for (int cornIndex = 0; cornIndex < 4; ++cornIndex) {
                if (i != cornPositions[cornIndex] || !harvestBundle) continue;

                auto cornPrefab = harvestBundle->LoadAsset(
                    "Assets/Prefabs/Thanksgiving_Single_Corn_Stalk.prefab");

                if (!cornPrefab) {
                    WARNING("[Thanksgiving CORN] Single-stalk prefab unavailable at position {}", i + 1);
                    continue;
                }

                auto cornInstance = UnityEngine::Object::Instantiate(
                    cornPrefab.ptr(), root, false);

                if (!cornInstance) {
                    WARNING("[Thanksgiving CORN] Single-stalk instantiation failed at position {}", i + 1);
                    continue;
                }

                auto* cornObject =
                    il2cpp_utils::try_cast<UnityEngine::GameObject>(
                        cornInstance.ptr()).value_or(nullptr);

                if (!cornObject) continue;

                auto cornTransform = cornObject->get_transform();
                const float cornSide = (cornIndex % 2 == 0) ? 1.0f : -1.0f;
                const float cornWorldScale = 0.80f;
                const float cornLocalScale = cornWorldScale / variedScale;

                cornTransform->set_localPosition(
                    Vector3(cornSide * 2.35f, 0.0f, -1.05f));
                cornTransform->set_localRotation(
                    Quaternion::Euler(
                        Vector3(0.0f, 20.0f + cornIndex * 83.0f, 0.0f)));
                cornTransform->set_localScale(
                    Vector3(cornLocalScale, cornLocalScale, cornLocalScale));

                if (baseMaterial) {
                    auto cornRenderers =
                        cornObject->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);

                    int cornRendererCount = 0;

                    for (auto* renderer : cornRenderers) {
                        if (!renderer) continue;

                        auto rendererObject = renderer->get_gameObject();
                        std::string rendererName = rendererObject
                            ? static_cast<std::string>(rendererObject->get_name())
                            : "";

                        UnityEngine::Color cornColor(0.57f, 0.43f, 0.18f, 1.0f);

                        if (rendererName.find("Olive_Leaves") != std::string::npos) {
                            cornColor = UnityEngine::Color(0.42f, 0.48f, 0.18f, 1.0f);
                        } else if (rendererName.find("Golden_Leaves") != std::string::npos) {
                            cornColor = UnityEngine::Color(0.73f, 0.57f, 0.20f, 1.0f);
                        } else if (rendererName.find("Kernels") != std::string::npos) {
                            cornColor = UnityEngine::Color(0.96f, 0.73f, 0.23f, 1.0f);
                        } else if (rendererName.find("Husks") != std::string::npos) {
                            cornColor = UnityEngine::Color(0.82f, 0.70f, 0.40f, 1.0f);
                        } else if (rendererName.find("Tassels") != std::string::npos) {
                            cornColor = UnityEngine::Color(0.77f, 0.61f, 0.29f, 1.0f);
                        }

                        auto* cornMaterial =
                            UnityEngine::Object::Instantiate(baseMaterial);

                        cornMaterial->SetColor("_Color", cornColor);

                        renderer->set_sharedMaterial(cornMaterial);
                        renderer->set_enabled(true);

                        ++cornRendererCount;
                    }

                    INFO("[Thanksgiving CORN RENDER] Position {} renderer count={}",
                         i + 1, cornRendererCount);
                } else {
                    WARNING("[Thanksgiving CORN RENDER] Base material unavailable at position {}",
                            i + 1);
                }
                INFO("[Thanksgiving CORN] Added individual stalk at position {}", i + 1);
            }            if (prefabPath) {
                if (!harvestBundle) {
                    WARNING("[Thanksgiving v1.4] Bundle unavailable at position {}. Using fallback.", i + 1);
                } else {
                    INFO("[Thanksgiving v1.4] Loading prefab {} at position {}.", prefabPath, i + 1);

                    auto prefab = harvestBundle->LoadAsset(prefabPath);

                    if (!prefab) {
                        WARNING("[Thanksgiving v1.4] Prefab not found at position {}: {}", i + 1, prefabPath);
                    } else {
                        auto instance = UnityEngine::Object::Instantiate(
                            prefab.ptr(), root, false);

                        if (instance) {
                            INFO("[Thanksgiving v1.4] Prefab instantiated successfully at position {}.", i + 1);

                            auto* instanceObject =
                                il2cpp_utils::try_cast<UnityEngine::GameObject>(instance.ptr()).value_or(nullptr);

                            if (!instanceObject) {
                                WARNING("[Thanksgiving v1.4 DIAG] Position {}: instance is not a GameObject.", i + 1);
                            } else {

                                // Keep every rustic fence the same world-space size.
                                if (std::string(prefabPath).find(
                                    "Thanksgiving_Rustic_Fence.prefab") != std::string::npos) {

                                    auto fenceTransform = instanceObject->get_transform();
                                    auto fenceScale = fenceTransform->get_localScale();

                                    const float inverseParentScale = 1.0f / variedScale;

                                    fenceTransform->set_localScale(
                                        Vector3(
                                            fenceScale.x * inverseParentScale,
                                            fenceScale.y * inverseParentScale,
                                            fenceScale.z * inverseParentScale
                                        )
                                    );
                                    // [Thanksgiving SPACING v1] Keep fence world size uniform;
                                    // move it away from adjacent hay displays.
                                    fenceTransform->set_localPosition(
                                        Vector3(0.0f, 0.0f, -2.25f / variedScale));

                                    INFO("[Thanksgiving FENCE] Applied uniform world-scale correction at position {}", i + 1);
                                }
                                INFO("[Thanksgiving v1.4 CAMERA DIAG] Position {}: prefab layer={}",
                                     i + 1, instanceObject->get_layer());

                                auto cameras = UnityEngine::Camera::get_allCameras();

                                INFO("[Thanksgiving v1.4 CAMERA DIAG] Active camera count={}",
                                     cameras.size());

                                for (auto camera : cameras) {
                                    if (!camera) continue;

                                    auto cameraObject = camera->get_gameObject();

                                    INFO("[Thanksgiving v1.4 CAMERA DIAG] Camera layer={} cullingMask={}",
                                         cameraObject ? cameraObject->get_layer() : -1,
                                         camera->get_cullingMask());
                                }

                                INFO("[Thanksgiving v1.4 DIAG] Position {}: active={}",
                                     i + 1, instanceObject->get_activeInHierarchy());

                                auto renderers =
                                    instanceObject->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);

                                INFO("[Thanksgiving v1.4 DIAG] Position {}: renderer count={}",
                                     i + 1, renderers.size());

                                for (auto* renderer : renderers) {
                                    if (!renderer) continue;

                                    auto rendererObject = renderer->get_gameObject();

                                    INFO("[Thanksgiving v1.4 CAMERA DIAG] Position {}: renderer layer={}",
                                         i + 1,
                                         rendererObject ? rendererObject->get_layer() : -1);



                                    auto bounds = renderer->get_bounds();
                                    auto center = bounds.get_center();
                                    auto size = bounds.get_size();

                                    auto material = renderer->get_sharedMaterial();
                                    bool hasMaterial = static_cast<bool>(material);

                                    bool hasShader = false;
                                    bool shaderSupported = false;

                                    if (material) {
                                        auto shader = material->get_shader();
                                        hasShader = static_cast<bool>(shader);

                                        if (shader) {
                                            shaderSupported = shader->get_isSupported();
                                        }
                                    }

                                    INFO("[Thanksgiving v1.4 DIAG] Position {}: enabled={} center=({},{},{}) size=({},{},{}) material={} shader={} supported={}",
                                         i + 1,
                                         renderer->get_enabled(),
                                         center.x, center.y, center.z,
                                         size.x, size.y, size.z,
                                         hasMaterial, hasShader, shaderSupported);
                                }
                            }

                            if (instanceObject && baseMaterial) {
                                auto meshRenderers =
                                    instanceObject->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);

                                for (auto* meshRenderer : meshRenderers) {
                                    if (!meshRenderer) continue;

                                    auto rendererObject = meshRenderer->get_gameObject();
                                    std::string rendererName = rendererObject
                                        ? static_cast<std::string>(rendererObject->get_name())
                                        : "";

                                    UnityEngine::Color naturalColor(0.82f, 0.57f, 0.20f, 1.0f);

                                    if (rendererName.find("Dry_straw") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.66f, 0.43f, 0.16f, 1.0f);
                                    } else if (rendererName.find("Light_straw") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.96f, 0.79f, 0.43f, 1.0f);
                                    } else if (rendererName.find("Golden_straw") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.88f, 0.62f, 0.22f, 1.0f);
                                    } else if (rendererName.find("Natural_twine") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.43f, 0.30f, 0.17f, 1.0f);
                                    } else if (rendererName.find("Sun_faded_oak") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.64f, 0.43f, 0.26f, 1.0f);
                                    } else if (rendererName.find("Weathered_oak") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.43f, 0.27f, 0.16f, 1.0f);
                                    } else if (rendererName.find("Aged_oak") != std::string::npos) {
                                        naturalColor = UnityEngine::Color(0.29f, 0.17f, 0.10f, 1.0f);
                                    }

                                    INFO("[Thanksgiving COLOR DIAG] Renderer={} Color=({},{},{})",
                                         rendererName,
                                         naturalColor.r, naturalColor.g, naturalColor.b);

                                    auto* replacement = UnityEngine::Object::Instantiate(baseMaterial);

                                    replacement->SetColor("_Color", naturalColor);

                                    auto activeShader = replacement->get_shader();
                                    if (activeShader) {
                                        INFO("[Thanksgiving SHADER NAME] Renderer={} Shader={}",
                                             rendererName,
                                             static_cast<std::string>(activeShader->get_name()));
                                    }

                                    INFO("[Thanksgiving SHADER DIAG] Renderer={} HasColor={} HasBaseColor={}",
                                         rendererName,
                                         replacement->HasProperty("_Color"),
                                         replacement->HasProperty("_BaseColor"));

                                    if (replacement->HasProperty("_EmissionColor")) {
                                        auto emissionColor = replacement->GetColor("_EmissionColor");

                                        INFO("[Thanksgiving EMISSION DIAG] Renderer={} EmissionColor=({},{},{},{})",
                                             rendererName,
                                             emissionColor.r,
                                             emissionColor.g,
                                             emissionColor.b,
                                             emissionColor.a);
                                    }

                                    auto actualColor = replacement->GetColor("_Color");

                                    INFO("[Thanksgiving MATERIAL DIAG] Renderer={} MainTex={} Emission={} BaseMap={} Color=({},{},{},{})",
                                         rendererName,
                                         replacement->HasProperty("_MainTex"),
                                         replacement->HasProperty("_EmissionColor"),
                                         replacement->HasProperty("_BaseMap"),
                                         actualColor.r,
                                         actualColor.g,
                                         actualColor.b,
                                         actualColor.a);

                                    meshRenderer->set_sharedMaterial(replacement);
                                    meshRenderer->set_enabled(true);
                                }
                            }

                            continue;
                        }

                        ERROR("[Thanksgiving v1.4] Prefab instantiation failed at position {}.", i + 1);
                    }
                }
            }

            // Thanksgiving v1.3: rustic farm fence with rounded posts.
            // Each cylinder's long axis is its local Y axis.

            auto* leftPost = MakePart(
                root, PrimitiveType::Cylinder, "RusticFencePostLeft",
                Vector3(-0.75f, 0.85f, 0.0f),
                Vector3(0.12f, 0.85f, 0.12f), wood);

            auto* rightPost = MakePart(
                root, PrimitiveType::Cylinder, "RusticFencePostRight",
                Vector3(0.75f, 0.85f, 0.0f),
                Vector3(0.12f, 0.85f, 0.12f), wood);

            // Horizontal split rails.
            for (float height : {0.62f, 1.22f}) {
                auto* rail = MakePart(
                    root, PrimitiveType::Cylinder, "RusticFenceRail",
                    Vector3(0.0f, height, 0.0f),
                    Vector3(0.075f, 0.88f, 0.075f), darkWood);

                rail->get_transform()->set_localRotation(
                    Quaternion::Euler(Vector3(0.0f, 0.0f, 90.0f)));
            }

            // Rounded straw bale bodies.
            MakePart(root, PrimitiveType::Cube, "StrawBaleLower",
                     Vector3(-0.13f, 0.27f, -0.48f),
                     Vector3(1.20f, 0.48f, 0.63f), straw);

            MakePart(root, PrimitiveType::Cube, "StrawBaleUpper",
                     Vector3(0.27f, 0.75f, -0.48f),
                     Vector3(0.77f, 0.42f, 0.58f), strawHighlight);

            // Straw-colored rounded edges soften the rectangular silhouette.
            for (float x : {-0.69f, 0.43f}) {
                MakePart(root, PrimitiveType::Capsule, "BaleStrawEdge",
                         Vector3(x, 0.27f, -0.48f),
                         Vector3(0.075f, 0.24f, 0.075f), strawHighlight);
            }

            // Two visible binding bands on the lower bale.
            for (float x : {-0.40f, 0.18f}) {
                MakePart(root, PrimitiveType::Cube, "BaleTwineFront",
                         Vector3(x, 0.27f, -0.805f),
                         Vector3(0.035f, 0.50f, 0.028f), darkWood);

                MakePart(root, PrimitiveType::Cube, "BaleTwineTop",
                         Vector3(x, 0.515f, -0.48f),
                         Vector3(0.035f, 0.025f, 0.66f), darkWood);
            }

            // Short straw strands on the front face.
            for (int strand = 0; strand < 5; ++strand) {
                float x = -0.58f + strand * 0.22f;

                auto* strawDetail = MakePart(
                    root, PrimitiveType::Cylinder, "LooseStraw",
                    Vector3(x, 0.30f, -0.807f),
                    Vector3(0.012f, 0.17f, 0.012f), strawHighlight);

                strawDetail->get_transform()->set_localRotation(
                    Quaternion::Euler(Vector3(0.0f, 0.0f, 76.0f)));
            }
        }
    }

    void DestroyPumpkinAccents() {
        if (pumpkinAccents) {
            GameObject::Destroy(pumpkinAccents.ptr());
            pumpkinAccents = nullptr;
        }
    }
}
