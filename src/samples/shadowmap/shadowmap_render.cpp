#include "shadowmap_render.h"

#include <geom/vk_mesh.h>
#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <iostream>

#include <etna/GlobalContext.hpp>
#include <etna/Etna.hpp>
#include <etna/RenderTargetStates.hpp>
#include <vulkan/vulkan_core.h>


void SimpleShadowmapRender::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer cmdBuf = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandPool, 1).front();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuf, &beginInfo));

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(cmdBuf, srcBuffer, dstBuffer, 1, &copyRegion);

  VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuf;

  VK_CHECK_RESULT(vkQueueSubmit(m_context->getQueue(), 1, &submitInfo, VK_NULL_HANDLE));
  VK_CHECK_RESULT(vkQueueWaitIdle(m_context->getQueue()));
}

/// RESOURCE ALLOCATION

void SimpleShadowmapRender::AllocateResources()
{
  defaultSampler = etna::Sampler(etna::Sampler::CreateInfo{.name = "default_sampler"});

  particles = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(Particle) * PARTICLE_COUNT,
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "particles"
  });
  particlesToSpawn = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(float),
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "particles_to_spawn"
  });
  staging = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(Particle) * PARTICLE_COUNT,
    .bufferUsage = vk::BufferUsageFlagBits::eTransferSrc,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "staging"
  });
  constants = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(UniformParams),
    .bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "constants"
  });

  m_uboMappedMem = constants.map();

  m_stagingMappedMem = staging.map();
  std::vector<Particle> particlesData;
  particlesData.resize(PARTICLE_COUNT);
  memcpy(m_stagingMappedMem, particlesData.data(), sizeof(Particle) * PARTICLE_COUNT);
  staging.unmap();

  // void *particlesToSpawnMapped = particlesToSpawn.map();
  // float zero = 0;
  // memcpy(particlesToSpawnMapped, &zero, sizeof(float));
  // particlesToSpawn.unmap();

  CopyBuffer(staging.get(), particles.get(), sizeof(Particle) * PARTICLE_COUNT);
}

void SimpleShadowmapRender::LoadScene(const char* path, bool transpose_inst_matrices)
{
  // TODO: Make a separate stage
  loadShaders();
  PreparePipelines();
}

void SimpleShadowmapRender::DeallocateResources()
{
  m_swapchain.Cleanup();
  vkDestroySurfaceKHR(GetVkInstance(), m_surface, nullptr);  

  constants = etna::Buffer();
}





/// PIPELINES CREATION

void SimpleShadowmapRender::PreparePipelines()
{
  SetupPipelines();
}

void SimpleShadowmapRender::loadShaders()
{
  etna::create_program("particles_graphics", 
  {
    VK_GRAPHICS_BASIC_ROOT"/resources/shaders/particles.vert.spv",
    VK_GRAPHICS_BASIC_ROOT"/resources/shaders/particles.frag.spv"
  });
  etna::create_program("particles_create",
  {
    VK_GRAPHICS_BASIC_ROOT"/resources/shaders/particles_create.comp.spv"
  });
  etna::create_program("particles_update",
  {
    VK_GRAPHICS_BASIC_ROOT"/resources/shaders/particles_update.comp.spv"
  });
}

void SimpleShadowmapRender::SetupPipelines()
{
  auto& pipelineManager = etna::get_context().getPipelineManager();

  std::vector<etna::VertexByteStreamFormatDescription::Attribute> attributes = {
    { .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Particle, pos) },
    { .format = vk::Format::eR32Sfloat, .offset = offsetof(Particle, remainingLifetime) },
  };

  etna::VertexShaderInputDescription pointInputDesc
  {
    .bindings = { etna::VertexShaderInputDescription::Binding {
      .byteStreamDescription = { .stride = sizeof(Particle), .attributes = attributes }
    }}
  };

  m_graphicsPipeline = pipelineManager.createGraphicsPipeline("particles_graphics", 
  {
    .vertexShaderInput = pointInputDesc,
    .inputAssemblyConfig = { .topology = vk::PrimitiveTopology::ePointList },
  });

  m_computeCreatePipeline = pipelineManager.createComputePipeline("particles_create", { });
  m_computeUpdatePipeline = pipelineManager.createComputePipeline("particles_update", { });
}

/// COMMAND BUFFER FILLING

void SimpleShadowmapRender::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  // create particles
  {
    auto particlesComputeInfo = etna::get_shader_program("particles_create");
    auto set = etna::create_descriptor_set(particlesComputeInfo.getDescriptorLayoutId(0), a_cmdBuff, 
    {
      etna::Binding {0, particles.genBinding()},
      etna::Binding {1, constants.genBinding()},
      etna::Binding {2, particlesToSpawn.genBinding()},
    });
    VkDescriptorSet vkSet = set.getVkSet();

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_computeCreatePipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_computeCreatePipeline.getVkPipelineLayout(),
      0, 1, &vkSet, 0, VK_NULL_HANDLE);

    vkCmdDispatch(a_cmdBuff, 1, 1, 1);
  }

  // update particles
  {
    auto particlesComputeInfo = etna::get_shader_program("particles_update");
    auto set = etna::create_descriptor_set(particlesComputeInfo.getDescriptorLayoutId(0), a_cmdBuff, 
    {
      etna::Binding {0, particles.genBinding()},
      etna::Binding {1, constants.genBinding()}
    });
    VkDescriptorSet vkSet = set.getVkSet();

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_computeUpdatePipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_computeUpdatePipeline.getVkPipelineLayout(),
      0, 1, &vkSet, 0, VK_NULL_HANDLE);

    vkCmdDispatch(a_cmdBuff, PARTICLE_COUNT / 256, 1, 1);
  }

  // render particles
  { 
    // set render target
    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, m_width, m_height}, 
      { {.image = a_targetImage, .view = a_targetImageView} }, // color attachment
      { }                                                      // depth attachment
    );

    // bind pipeline
    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.getVkPipeline());

    // bind vertex buffers
    VkDeviceSize offsets[1] = { 0 };
    VkBuffer vertexBuffer = particles.get();
    vkCmdBindVertexBuffers(a_cmdBuff, 0, 1, &vertexBuffer, offsets);

    // draw
    vkCmdDraw(a_cmdBuff, PARTICLE_COUNT, 1, 0, 0);
  }

  etna::set_state(a_cmdBuff, a_targetImage, vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::AccessFlags2(), vk::ImageLayout::ePresentSrcKHR,
    vk::ImageAspectFlagBits::eColor);

  etna::finish_frame(a_cmdBuff);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}
