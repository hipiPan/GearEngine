#pragma once
#include <Blast/Gfx/GfxDefine.h>
#include <Blast/Gfx/GfxBuffer.h>
#include <Blast/Gfx/GfxContext.h>
#include <Blast/Gfx/GfxPipeline.h>
#include <vector>
namespace gear {
    class GpuBuffer {
    public:
        GpuBuffer(Blast::ResourceType type, uint32_t size);

        virtual ~GpuBuffer();

        void update(void* data, uint32_t offset, uint32_t size);
    protected:
        Blast::GfxBuffer* mBuffer = nullptr;
    };

    // TODO: 支持instance
    class VertexBuffer : public GpuBuffer {
    public:
        class Builder {
        public:
            Builder() = default;

            ~Builder() = default;

            void vertexCount(uint32_t count);

            void attribute(Blast::ShaderSemantic semantic, Blast::Format format);

            VertexBuffer* build();
        private:
            friend class VertexBuffer;
            uint32_t mVertexCount;
            uint32_t mBufferSize;
            std::vector<Blast::GfxVertexAttrib> mAttributes;
        };

        ~VertexBuffer();
    private:
        VertexBuffer(Builder* builder);

    private:
        Blast::GfxVertexLayout mLayout;
    };

    class IndexBuffer : public GpuBuffer {
    public:
        class Builder {
        public:
            Builder() = default;

            ~Builder() = default;

            void indexType(Blast::IndexType type);

            void indexCount(uint32_t count);

            IndexBuffer* build();
        private:
            friend class IndexBuffer;
            uint32_t mIndexCount;
            uint32_t mBufferSize;
            Blast::IndexType mIndexType;
        };

        ~IndexBuffer();
    private:
        IndexBuffer(Builder* builder);
    private:
        Blast::IndexType mIndexType;
    };

    class UniformBuffer : public GpuBuffer {
    public:
        UniformBuffer(uint32_t size);

        ~UniformBuffer();
    };
}