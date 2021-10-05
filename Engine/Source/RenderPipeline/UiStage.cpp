#include "RenderPipeline.h"
#include "Resource/GpuBuffer.h"
#include "Resource/Texture.h"
#include "Resource/Material.h"
#include "Resource/RenderTarget.h"
#include "GearEngine.h"
#include "Renderer/Renderer.h"

namespace gear {
    void RenderPipeline::ExecUiStage() {
        Renderer* renderer = gEngine.GetRenderer();
        uint32_t ui_dc_head = 0;
        uint32_t num_ui_dc = 0;
        for (uint32_t i = 0; i < _num_ui_renderables; ++i) {
            Renderable* rb = &_renderables[_ui_renderables[i]];
            uint32_t dc_idx = ui_dc_head + num_ui_dc;

            uint32_t material_id = rb->mi->GetMaterial()->GetMaterialID();
            uint32_t material_instance_id = rb->mi->GetMaterialInstanceID();
            uint32_t material_variant = 0;

            _dc_list[dc_idx] = {};
            _dc_list[dc_idx].renderable_id = _ui_renderables[i];
            _dc_list[dc_idx].material_variant = material_variant;
            _dc_list[dc_idx].key |= DrawCall::GenMaterialKey(material_id, material_variant, material_instance_id);

            num_ui_dc++;
        }

        // 排序
        std::sort(&_dc_list[ui_dc_head], &_dc_list[ui_dc_head] + num_ui_dc);

        renderer->BindFramebuffer(_display_fb);
        for (uint32_t i = ui_dc_head; i < ui_dc_head + num_ui_dc; ++i) {
            uint32_t material_variant = _dc_list[i].material_variant;
            uint32_t randerable_id = _dc_list[i].renderable_id;
            Renderable& renderable = _renderables[randerable_id];

            renderer->ResetUniformBufferSlot();
            renderer->BindFrameUniformBuffer(_view_ub->GetHandle(), _view_ub->GetSize(), 0);
            renderer->BindRenderableUniformBuffer(renderable.renderable_ub->GetHandle(), renderable.renderable_ub_size, renderable.renderable_ub_offset);
            blast::GfxBuffer* material_ub = renderable.mi->GetUniformBuffer()->GetHandle();
            if (material_ub != nullptr) {
                renderer->BindMaterialUniformBuffer(material_ub, material_ub->GetSize(), 0);
            }

            renderer->ResetSamplerSlot();

            // 材质贴图
            for (uint32_t k = 0; k < renderable.mi->GetGfxSamplerGroup().size(); ++k) {
                SamplerInfo material_sampler_info = {};
                material_sampler_info.slot = 4 + k;
                material_sampler_info.layer = 0;
                material_sampler_info.num_layers = SHADOW_CASCADE_COUNT;
                material_sampler_info.texture = renderable.mi->GetGfxSamplerGroup().at(k).first->GetTexture();
                material_sampler_info.sampler_desc = renderable.mi->GetGfxSamplerGroup().at(k).second;

                renderer->BindSampler(material_sampler_info);
            }

            renderer->BindVertexShader(renderable.mi->GetMaterial()->GetVertShader(material_variant));
            renderer->BindFragmentShader(renderable.mi->GetMaterial()->GetFragShader(material_variant));

            const RenderState& render_state = renderable.mi->GetMaterial()->GetRenderState();
            renderer->SetDepthState(true);
            renderer->SetBlendingMode(render_state.blending_mode);
            renderer->SetFrontFace(blast::FRONT_FACE_CCW);
            renderer->SetCullMode(blast::CULL_MODE_BACK);
            renderer->SetPrimitiveTopo(renderable.topo);

            renderer->BindVertexBuffer(renderable.vb->GetHandle(), renderable.vb->GetVertexLayout(), renderable.vb->GetSize(), 0);
            renderer->BindIndexBuffer(renderable.ib->GetHandle(), renderable.ib->GetIndexType(), renderable.ib->GetSize(), 0);

            renderer->DrawIndexed(renderable.count, renderable.offset);
        }
        renderer->UnbindFramebuffer();
    }
}
