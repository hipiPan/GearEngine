#ifndef RHI_PIPELINE_STATE_H
#define RHI_PIPELINE_STATE_H
#include "RHIDefine.h"
#include "RHIProgram.h"
#include "RHIProgramParam.h"
#include "Utility/Hash.h"

// note:暂不加入计算管线
// note:使用智能指针来传参会好一点?

struct RHIPipelineStateInfo
{
	//programs
	RHIProgram* vertexProgram = nullptr;
	RHIProgram* fragmentProgram = nullptr;

	//todo:render states
};

class RHIDevice;

class RHIGraphicsPipelineState
{
public:
	RHIGraphicsPipelineState(RHIDevice* device, const RHIPipelineStateInfo& info);
	~RHIGraphicsPipelineState();
	VkPipeline getPipeline();
private:
	struct VariantKey
	{
		// note:开发阶段仅使用render pass生成hash key,后续还有绘制方式以及顶点输入布局
		VariantKey(uint32_t inRenderpassID);

		class HashFunction
		{
		public:
			std::size_t operator()(const VariantKey& key) const;
		};

		class EqualFunction
		{
		public:
			bool operator()(const VariantKey& lhs, const VariantKey& rhs) const;
		};

		uint32_t renderpassID;
	};


private:
	RHIDevice* mDevice;
	RHIProgram* mVertexProgram = nullptr;
	RHIProgram* mFragmentProgram = nullptr;

	VkPipeline mPipeline;
};

#endif