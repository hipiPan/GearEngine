#include "MaterialTestScene.h"

#include <GearEngine.h>
#include <Renderer/Renderer.h>
#include <Resource/Material.h>

#include <imgui.h>


MaterialTestScene::MaterialTestScene() {
}

MaterialTestScene::~MaterialTestScene() {

}

void MaterialTestScene::Load() {
    scene = new gear::Scene();
    {
        main_camera = gear::gEngine.GetEntityManager()->CreateEntity();
        main_camera->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
        main_camera->GetComponent<gear::CTransform>()->SetPosition(glm::vec3(0.0f, 0.0f, 4.0f));
        main_camera->AddComponent<gear::CCamera>()->SetProjection(gear::ProjectionType::PERSPECTIVE, 0.0, 800.0, 600.0, 0.0, 0.1, 100.0);
        scene->AddEntity(main_camera);

        sun = gear::gEngine.GetEntityManager()->CreateEntity();
        sun->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
        sun->GetComponent<gear::CTransform>()->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
        sun->GetComponent<gear::CTransform>()->SetEuler(glm::vec3(glm::radians(120.0f), 0.0f, 0.0f));
        sun->AddComponent<gear::CLight>();
        sun->GetComponent<gear::CLight>()->SetColor(glm::vec3(0.8f, 0.8f, 0.9f));
        sun->GetComponent<gear::CLight>()->SetIntensity(120000.0f);
        scene->AddEntity(sun);

        camera_controller = new CameraController();
        camera_controller->SetCamera(main_camera);

        gltf_asset = ImportGltfAsset("./BuiltinResources/GltfFiles/material_sphere/material_sphere.gltf");

        for (uint32_t i = 0; i < gltf_asset->entities.size(); ++i) {
            if (gltf_asset->entities[i]->HasComponent<gear::CMesh>()) {
                gltf_asset->entities[i]->GetComponent<gear::CMesh>()->SetCastShadow(true);
                gltf_asset->entities[i]->GetComponent<gear::CMesh>()->SetReceiveShadow(true);
                scene->AddEntity(gltf_asset->entities[i]);
            }
        }

        // 加载天空盒以及IBL资源
        gear::Texture* equirectangular_map = ImportTexture2DWithFloat("./BuiltinResources/Textures/Ridgecrest_Road_Ref.hdr");
        skybox_map = gear::gEngine.GetRenderer()->EquirectangularMapToCubemap(equirectangular_map, 512);
        irradiance_map = gear::gEngine.GetRenderer()->ComputeIrradianceMap(skybox_map);
        prefiltered_map = gear::gEngine.GetRenderer()->ComputePrefilteredMap(skybox_map);
        brdf_lut = gear::gEngine.GetRenderer()->ComputeBRDFLut();
        SAFE_DELETE(equirectangular_map);

        sun->AddComponent<gear::CSkybox>()->SetCubeMap(skybox_map);
        ibl = gear::gEngine.GetEntityManager()->CreateEntity();
        ibl->AddComponent<gear::CTransform>()->SetTransform(glm::mat4(1.0f));
        ibl->AddComponent<gear::CLight>();
        ibl->GetComponent<gear::CLight>()->SetLightType(gear::CLight::LightType::IBL);
        ibl->GetComponent<gear::CLight>()->SetIrradianceMap(irradiance_map);
        ibl->GetComponent<gear::CLight>()->SetPrefilteredMap(prefiltered_map);
        ibl->GetComponent<gear::CLight>()->SetBRDFLut(brdf_lut);
        scene->AddEntity(ibl);
    }
}

void MaterialTestScene::Clear() {
    DestroyGltfAsset(gltf_asset);
    gear::gEngine.GetEntityManager()->DestroyEntity(sun);
    gear::gEngine.GetEntityManager()->DestroyEntity(ibl);
    gear::gEngine.GetEntityManager()->DestroyEntity(main_camera);
    SAFE_DELETE(skybox_map);
    SAFE_DELETE(irradiance_map);
    SAFE_DELETE(prefiltered_map);
    SAFE_DELETE(brdf_lut);
    SAFE_DELETE(camera_controller);
    SAFE_DELETE(scene);
}

void MaterialTestScene::DrawUI() {
    // Get Material
    gear::Material* ma = nullptr;
    gear::MaterialInstance* mi = nullptr;
    mi = gltf_asset->entities[0]->GetComponent<gear::CMesh>()->GetSubMeshs()[0].mi;
    ma = mi->GetMaterial();
    if (!ma || !mi) {
        return;
    }

    ImGui::Begin("Material Setting");
    auto uniforms = mi->GetUniformGroup();
    uint8_t* uniform_data = mi->GetStorage();
    for (auto u : uniforms) {
        blast::UniformType type = std::get<0>(u.second);
        uint32_t offset = std::get<1>(u.second);
        if (type == blast::UNIFORM_FLOAT4) {
            ImGui::DragFloat4(u.first.c_str(), reinterpret_cast<float*>(uniform_data + offset), 0.1f, 0.0f, 1.0f);
        }
    }
    ImGui::End();
}

gear::Scene* MaterialTestScene::GetScene() {
    return scene;
}

