#include "Texture.h"
#include "../RenderAPI/VulkanContext.h"
#include "../RenderAPI/CommandBuffer.h"
#include <assert.h>
Texture::Texture()
{
}

Texture::~Texture()
{
	mImage->destroy();
}

void Texture::init(VkImageUsageFlags usage, VkFormat format, VkSampleCountFlagBits samples, VkImageAspectFlags imageAspect)
{
	mImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	mImageCI.pNext = nullptr;
	mImageCI.flags = 0;
	mImageCI.imageType = VK_IMAGE_TYPE_2D;
	mImageCI.usage = usage;
	//todo
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

	mImageCI.extent = {mWidth, mHeight, 1};
	mImageCI.mipLevels = mNumMipLevels;
	mImageCI.arrayLayers = mNumFaces;
	mImageCI.samples = samples;
	mImageCI.tiling = tiling;
	mImageCI.initialLayout = layout;
	mImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	mImageCI.queueFamilyIndexCount = 0;
	mImageCI.pQueueFamilyIndices = nullptr;

	mDesc.layout = layout;
	mDesc.format = format;
	mDesc.aspectFlags = imageAspect;
	mDesc.numFaces = mNumFaces;
	mDesc.numMipLevels = mNumMipLevels;
	mDesc.type = TextureType::TEX_TYPE_2D;
	

	createImage();
}

void Texture::setImageLayout(const VkImageLayout & newLayout, const uint32_t &mipLevels, const uint32_t &baseArrayLayer, const uint32_t &layerCount)
{
	VkImageLayout oldLayout = mDesc.layout;
	CommandBuffer commandBuffer = CommandBuffer();
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = mDesc.image;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	imageMemoryBarrier.subresourceRange.layerCount = layerCount;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageMask = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else
	{
		assert(false && "Unsupported image layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(), srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	commandBuffer.end();
	commandBuffer.submit();
}

void Texture::copyBufferToImage(const uint32_t & width, const uint32_t & height, const VkBuffer & buffer, const VkImage & image)
{
	CommandBuffer commandBuffer = CommandBuffer();
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(commandBuffer.getCommandBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	commandBuffer.end();
	commandBuffer.submit();
}

void Texture::writeData(void * pixelData)
{
	Buffer* stagingBuffer = createStaging();
	void *data;
	vkMapMemory(VulkanContext::instance().getDevice(), stagingBuffer->getMemory(), 0, stagingBuffer->getSize(), 0, &data);
	memcpy(data, pixelData, stagingBuffer->getSize());
	vkUnmapMemory(VulkanContext::instance().getDevice(), stagingBuffer->getMemory());
	copyBufferToImage(mWidth,mHeight,stagingBuffer->getBuffer(),mDesc.image);
	stagingBuffer->destroy();
}

void Texture::createImage()
{
	VkImage image;

	if (vkCreateImage(VulkanContext::instance().getDevice(), &mImageCI, nullptr, &image))
	{
		throw std::runtime_error("failed to create image !");
	}
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(VulkanContext::instance().getDevice(), image, &memoryRequirements);

	VkMemoryPropertyFlags flags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);//todo

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = VulkanContext::instance().findMemoryType(memoryRequirements.memoryTypeBits, flags);

	VkDeviceMemory memory;
	if (vkAllocateMemory(VulkanContext::instance().getDevice(), &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(VulkanContext::instance().getDevice(), image, memory, 0);
	mDesc.image = image;

	mImage = VulkanContext::instance().getResourceManager()->create<Image>(mDesc, memory);
}

Buffer * Texture::createStaging()
{
	VkBufferCreateInfo bufferCI;
	bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCI.pNext = nullptr;
	bufferCI.flags = 0;
	bufferCI.size = mWidth * mHeight * 4;//todo
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferCI.queueFamilyIndexCount = 0;
	bufferCI.pQueueFamilyIndices = nullptr;

	VkBuffer buffer;
	if (vkCreateBuffer(VulkanContext::instance().getDevice(), &bufferCI, nullptr, &buffer))
	{
		throw std::runtime_error("failed to create buffer!");
	}

	// Allocates buffer memory.
	VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanContext::instance().getDevice(), buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = VulkanContext::instance().findMemoryType(memoryRequirements.memoryTypeBits, flags);

	VkDeviceMemory memory;
	if (vkAllocateMemory(VulkanContext::instance().getDevice(), &memoryAllocateInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(VulkanContext::instance().getDevice(), buffer, memory, 0);

	return VulkanContext::instance().getResourceManager()->create<Buffer>(buffer, memory, mWidth * mHeight * 4);
}