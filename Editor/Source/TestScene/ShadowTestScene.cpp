#include "ShadowTestScene.h"
#include <GearEngine.h>
#include <Renderer/Renderer.h>
#include <imgui.h>

ShadowTestScene::ShadowTestScene() {
}

ShadowTestScene::~ShadowTestScene() {
}

void ShadowTestScene::Load() {
    scene = gear::Scene::Create("MaterialTestScene");
    main_camera = gear::Entity::Create("MainCamera");
    main_camera->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
    main_camera->GetComponent<gear::CTransform>()->SetPosition(glm::vec3(0.0f, 0.0f, 4.0f));
    //camera->GetComponent<gear::CTransform>()->SetEuler(glm::vec3(-0.358f, 0.46f, 0.0f));
    main_camera->AddComponent<gear::CCamera>()->SetProjection(gear::ProjectionType::PERSPECTIVE, 0.0, 800.0, 600.0, 0.0, 0.1, 100.0);
    //camera->AddComponent<gear::CCamera>()->SetProjection(gear::ProjectionType::ORTHO, -1.0, 1.0, 1.0, -1.0, 0.0, 1.0);
    scene->AddEntity(main_camera);

    debug_camera = gear::Entity::Create("DebugCamera");
    debug_camera->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
    debug_camera->GetComponent<gear::CTransform>()->SetPosition(glm::vec3(0.0f, 0.0f, 4.0f));
    debug_camera->AddComponent<gear::CCamera>()->SetProjection(gear::ProjectionType::PERSPECTIVE, 0.0, 800.0, 600.0, 0.0, 0.1, 100.0);
    debug_camera->GetComponent<gear::CCamera>()->SetMain(false);
    debug_camera->GetComponent<gear::CCamera>()->SetDisplay(false);
    scene->AddEntity(debug_camera);

    sun = gear::Entity::Create("Sun");
    sun->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
    sun->GetComponent<gear::CTransform>()->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    sun->GetComponent<gear::CTransform>()->SetEuler(glm::vec3(glm::radians(120.0f), 0.0f, 0.0f));
    sun->AddComponent<gear::CLight>();
    sun->GetComponent<gear::CLight>()->SetColor(glm::vec3(0.8f, 0.8f, 0.9f));
    sun->GetComponent<gear::CLight>()->SetIntensity(120000.0f);
    scene->AddEntity(sun);

    camera_controller = new CameraController();
    camera_controller->SetCamera(main_camera.get());

    gltf_asset = ImportGltfAsset("../BuiltinResources/GltfFiles/test.gltf");
    for (auto iter : gltf_asset->entities) {
        auto entity = iter.second;
        if (entity->HasComponent<gear::CMesh>()) {
            entity->GetComponent<gear::CMesh>()->SetCastShadow(true);
            entity->GetComponent<gear::CMesh>()->SetReceiveShadow(true);
            scene->AddEntity(entity);
        }
    }

    // Skybox and ibl
    std::shared_ptr<blast::GfxTexture> equirectangular_map = ImportTexture2DWithFloat("../BuiltinResources/Textures/Ridgecrest_Road_Ref.hdr");
    skybox_map = gear::gEngine.GetRenderer()->EquirectangularMapToCubemap(equirectangular_map, 512);
    irradiance_map = gear::gEngine.GetRenderer()->ComputeIrradianceMap(skybox_map);
    prefiltered_map = gear::gEngine.GetRenderer()->ComputePrefilteredMap(skybox_map);
    brdf_lut = gear::gEngine.GetRenderer()->ComputeBRDFLut();

    sun->AddComponent<gear::CSkybox>()->SetCubeMap(skybox_map);
    ibl = gear::Entity::Create("IBL");
    ibl->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
    ibl->AddComponent<gear::CLight>();
    ibl->GetComponent<gear::CLight>()->SetLightType(gear::CLight::LightType::IBL);
    ibl->GetComponent<gear::CLight>()->SetIrradianceMap(irradiance_map);
    ibl->GetComponent<gear::CLight>()->SetPrefilteredMap(prefiltered_map);
    ibl->GetComponent<gear::CLight>()->SetBRDFLut(brdf_lut);
    scene->AddEntity(ibl);
}

void ShadowTestScene::Clear() {
    DestroyGltfAsset(gltf_asset);
    SAFE_DELETE(camera_controller);
}

void ShadowTestScene::DrawUI() {
    ImGui::Begin("Change Camera");
    if (ImGui::Button("Main Camera")) {
        main_camera->GetComponent<gear::CCamera>()->SetDisplay(true);
        debug_camera->GetComponent<gear::CCamera>()->SetDisplay(false);
        camera_controller->SetCamera(main_camera.get());
    }
    if (ImGui::Button("Debug Camera")) {
        main_camera->GetComponent<gear::CCamera>()->SetDisplay(false);
        debug_camera->GetComponent<gear::CCamera>()->SetDisplay(true);
        camera_controller->SetCamera(debug_camera.get());
    }
    ImGui::End();
}

std::shared_ptr<gear::Scene> ShadowTestScene::GetScene() {
    return scene;
}

