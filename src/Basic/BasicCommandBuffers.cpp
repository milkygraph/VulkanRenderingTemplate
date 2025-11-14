#include <vks/Basic/BasicCommandBuffers.hpp>
#include <vks/Application.hpp>
#include <stdexcept>
#include <array>
#include <algorithm>

using namespace vks;

BasicCommandBuffers::BasicCommandBuffers(
    const Device &device, const RenderPass &renderPass,
    const SwapChain &swapChain, const GraphicsPipeline &graphicsPipeline,
    const CommandPool &commandPool,
    Application& application // <-- ADD THIS
)
    : CommandBuffers(device, renderPass, swapChain, graphicsPipeline, commandPool),
      m_app(application) // <-- STORE THIS
{
    BasicCommandBuffers::createCommandBuffers();
}

void BasicCommandBuffers::recreate() {
    destroyCommandBuffers();
    createCommandBuffers();
}

void BasicCommandBuffers::createCommandBuffers()
{
    m_commandBuffers.resize(m_renderPass.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool.handle();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device.logical(), &allocInfo,
                                 m_commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}


/**
 * @brief This is the new "cooking" function that renders your scene.
 * It is called every frame from Application::drawFrame.
 */
void BasicCommandBuffers::recordCommands(uint32_t imageIndex) {
    VkCommandBuffer cmdBuffer = m_commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass.handle();
    renderPassInfo.framebuffer = m_renderPass.frameBuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain.extent();

    // Set clear color AND depth
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.01f, 0.01f, 0.01f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0}; // For depth buffer
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 1. Get the scene data from the application
    auto renderObjects = m_app.getRenderObjects(); // Gets a copy
    VkDescriptorSet cameraSet = m_app.getCameraDescriptorSet();

    // 2. Sort the render objects for efficient binding
    std::sort(renderObjects.begin(), renderObjects.end(),
        [](const RenderObject& a, const RenderObject& b) {
            return a.getSortKey() < b.getSortKey();
    });

    // 3. Bind the "global" camera descriptor set (Set 0) ONCE
    if (cameraSet != VK_NULL_HANDLE && !renderObjects.empty()) {
        // We can safely get the layout from the first renderable object
        // (This assumes all scene objects use a compatible layout for Set 0)
        auto layoutName = renderObjects[0].material->getPipelineName();
        auto layout = m_graphicsPipeline.getLayout(layoutName);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            layout, 0, 1, &cameraSet, 0, nullptr);
    }

    // 4. Loop through the sorted objects and render them
    VkPipeline lastPipeline = VK_NULL_HANDLE;
    VkPipelineLayout lastLayout = VK_NULL_HANDLE;
    VkDescriptorSet lastMaterialSet = VK_NULL_HANDLE;

    for (const auto& obj : renderObjects) {
        auto pipelineName = obj.material->getPipelineName();
        VkPipeline pipeline = m_graphicsPipeline.getPipeline(pipelineName);
        VkPipelineLayout layout = m_graphicsPipeline.getLayout(pipelineName);

        // --- Bind Pipeline (if different) ---
        if (pipeline != lastPipeline) {
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            lastPipeline = pipeline;
            lastLayout = layout;

            // Re-bind global set if layout changed
             if (cameraSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    layout, 0, 1, &cameraSet, 0, nullptr);
            }
        }

        // --- Bind Material (Set 1) (if different) ---
        VkDescriptorSet materialSet = obj.material->getDescriptorSet();
        if (materialSet != lastMaterialSet && materialSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                layout, 1, 1, &materialSet, 0, nullptr);
            lastMaterialSet = materialSet;
        }

        // --- Bind Instance Data (Push Constants) ---
        // (Only if the layout has push constants)
        if(obj.model != nullptr) { // Simple check
            vkCmdPushConstants(cmdBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(glm::mat4), &obj.transform);
        }

        // --- Bind Geometry & Draw ---
        if (obj.model != nullptr) {
            VkBuffer vertexBuffers[] = { obj.model->getVertexBuffer() };
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(cmdBuffer, obj.model->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmdBuffer, obj.model->getIndexCount(), 1, 0, 0, 0);

        } else {
            // This is for pipelines with no vertex input, like "base"
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        }
    }
    // --- End of new loop ---

    vkCmdEndRenderPass(cmdBuffer);

    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}