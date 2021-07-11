#pragma once
#include "Core/GearDefine.h"
#include "RenderCache.h"
#include "RenderScene.h"
#include <Blast/Gfx/GfxPipeline.h>
#include <functional>

namespace Blast {
    class ShaderCompiler;
    class GfxContext;
    class GfxSurface;
    class GfxSwapchain;
    class GfxRenderPass;
    class GfxFramebuffer;
    class GfxQueue;
    class GfxFence;
    class GfxSemaphore;
    class GfxSemaphore;
    class GfxCommandBufferPool;
    class GfxCommandBuffer;
    class GfxRootSignature;
    class GfxGraphicsPipeline;
    class GfxSampler;
}

namespace gear {
    struct Attachment;
    struct RenderTargetDesc;
    struct RenderTargetParams;
    class GpuBuffer;
    class VertexBuffer;
    class IndexBuffer;
    class UniformBuffer;
    class Texture;
    class MaterialInstance;
    class RenderTarget;
    class CopyEngine;
    class RenderBuiltinResource;
    class RenderView;
    class RenderScene;
    class RenderPassCache;
    class FramebufferCache;
    class GraphicsPipelineCache;

    class Renderer {
    public:
        /**
         * 基础绘制单位
         */
        using DrawCallKey = uint64_t;

        struct DrawCall {
            DrawCallKey key = 0;

            // Material Variant
            uint8_t variant = 0;

            // RenderPrimitive
            uint32_t offset = 0;
            uint32_t count = 0;
            MaterialInstance* materialInstance = nullptr;
            VertexBuffer* vertexBuffer = nullptr;
            IndexBuffer* indexBuffer = nullptr;
            UniformBuffer* renderableUB = nullptr;
            UniformBuffer* boneUB = nullptr;
            Blast::PrimitiveTopology type = Blast::PrimitiveTopology::PRIMITIVE_TOPO_TRI_STRIP;

            // RenderState
            Blast::GfxBlendState blendState;
            Blast::GfxDepthState depthState;
            Blast::GfxRasterizerState rasterizerState;
            bool operator < (DrawCall const& rhs) { return key < rhs.key; }
        };

        Renderer();

        ~Renderer();

        void terminate();

        void initSurface(void* surface);

        void beginFrame(uint32_t width, uint32_t height);

        void endFrame();

        Blast::GfxContext* getContext() { return mContext; }

        Blast::GfxQueue* getQueue() { return mQueue; }

        CopyEngine* getCopyEngine() { return mCopyEngine; }

        RenderScene* getScene() { return mScene; }

        RenderBuiltinResource* getRenderBuiltinResource() { return mRenderBuiltinResource; }

        Blast::ShaderCompiler* getShaderCompiler() { return mShaderCompiler; }

        RenderTarget* getRenderTarget() { return mDefaultRenderTarget; }

        RenderTarget* createRenderTarget(const RenderTargetDesc& desc);

    private:
        Attachment getColor();

        Attachment getDepthStencil();

        void resize(uint32_t width, uint32_t height);

        void prepare();

        void render(RenderView* view);

        // begin shadow
        void renderShadow(RenderView* view);

        void computeSceneCascadeParams(RenderView* view, const CameraInfo& cameraInfo, CascadeParameters& cascadeParameters);
        // end shadow

        void bindRenderTarget(RenderTarget* rt, RenderTargetParams* params);

        void unbindRenderTarget();

        void executeDrawCall(DrawCall* dc);

    private:
        friend CopyEngine;
        friend RenderBuiltinResource;
        friend RenderTarget;
        CopyEngine* mCopyEngine;
        RenderBuiltinResource* mRenderBuiltinResource;
        Blast::ShaderCompiler* mShaderCompiler = nullptr;
        Blast::GfxContext* mContext = nullptr;
        Blast::GfxSurface* mSurface = nullptr;
        Blast::GfxSwapchain* mSwapchain = nullptr;
        Blast::GfxQueue* mQueue = nullptr;
        Blast::GfxFence** mRenderCompleteFences = nullptr;
        Blast::GfxSemaphore** mImageAcquiredSemaphores = nullptr;
        Blast::GfxSemaphore** mRenderCompleteSemaphores = nullptr;
        Blast::GfxCommandBufferPool* mCmdPool = nullptr;
        Blast::GfxCommandBuffer** mCmds = nullptr;
        Attachment* mColors = nullptr;
        Attachment* mDepthStencils = nullptr;
        RenderTarget* mDefaultRenderTarget = nullptr;
        RenderScene* mScene = nullptr;
        SamplerCache* mSamplerCache = nullptr;
        RenderPassCache* mRenderPassCache = nullptr;
        FramebufferCache* mFramebufferCache = nullptr;
        GraphicsPipelineCache* mGraphicsPipelineCache = nullptr;
        DescriptorCache* mDescriptorCache = nullptr;
        uint32_t mFrameIndex = 0;
        uint32_t mImageCount = 0;
        uint32_t mFrameWidth = 0;
        uint32_t mFrameHeight = 0;

        /**
         * 当前图形管线绑定的相关变量
         */
         Blast::GfxRenderPass* mBindRenderPass = nullptr;
         DescriptorKey mDescriptorKey;

        /**
         * DrawCall相关变量
         */
        DrawCall mDrawCalls[10240];
        uint32_t mDrawCallHead = 0;

         /**
          * Render Pass相关变量
          */
        bool mClearColor;
        bool mClearDepth;
        bool mClearStencil;
    };
}