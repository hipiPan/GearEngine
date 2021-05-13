#pragma once

#include "Core/GearDefine.h"
#include "Math/Math.h"
#include "Resource/Resource.h"
#include <Blast/Utility/ShaderCompiler.h>
#include <string>
#include <unordered_map>

namespace Blast {
    class GfxShader;
}

namespace gear {
    class Texture;

    // 材质应用的范围
    enum class MaterialDomain {
        SURFACE = 0,
        POST_PROCESS = 1,
    };

    // 材质属性
    static constexpr size_t MATERIAL_PROPERTIES_COUNT = 1;
    enum class MaterialProperty {
        BASE_COLOR,
    };

    // 材质的变体参数
    static constexpr size_t MATERIAL_VARIANT_COUNT = 2;
    struct MaterialVariant {
    public:
        MaterialVariant(uint8_t key) : key(key) { }
        static constexpr uint8_t SKINNING_OR_MORPHING   = 0x01; // 使用GPU蒙皮
        static constexpr uint8_t VERTEX_MASK = SKINNING_OR_MORPHING;
        static constexpr uint8_t FRAGMENT_MASK = 0;

        inline bool hasSkinningOrMorphing() const noexcept { return key & SKINNING_OR_MORPHING; }

        inline void setSkinning(bool v) noexcept { set(v, SKINNING_OR_MORPHING); }

        static constexpr uint8_t filterVariantVertex(uint8_t variantKey) noexcept {
            // 过滤掉不需要的顶点变体
            return variantKey & VERTEX_MASK;
        }

        static constexpr uint8_t filterVariantFragment(uint8_t variantKey) noexcept {
            // 过滤掉不需要的片段变体
            return variantKey & FRAGMENT_MASK;
        }
    private:
        void set(bool v, uint8_t mask) noexcept {
            key = (key & ~mask) | (v ? mask : uint8_t(0));
        }

        uint8_t key = 0;
    };

    struct MaterialDesc {
    };

    class MaterialInstance;

    class Material : public Resource {
    public:
        Material() {}
        ~Material() {}
        MaterialInstance* createInstance();
    protected:
        friend class MaterialCompiler;
        friend class MaterialInstance;
        std::vector<Blast::GfxShaderResource> mResources;
        std::vector<Blast::GfxShaderVariable> mVariables;
        std::unordered_map<uint8_t, Blast::GfxShader*> mVertShaderCache;
        std::unordered_map<uint8_t, Blast::GfxShader*> mFragShaderCache;
    };

    class MaterialInstance {
    public:
        MaterialInstance(Material* material);
        ~MaterialInstance();
    private:
        Material* mMaterial = nullptr;

    };
}