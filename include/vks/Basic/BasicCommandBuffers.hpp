#pragma once // Use pragma once

#include <vks/CommandBuffers.hpp>
// #include <vks/Model.hpp> // No longer needed here
// #include <memory> // No longer needed here

namespace vks {

// Forward-declare Application to avoid circular include
class Application;

class BasicCommandBuffers : public CommandBuffers {
public:
    BasicCommandBuffers(
        const Device &device,
        const RenderPass &renderpass,
        const SwapChain &swapChain,
        const GraphicsPipeline &graphicsPipeline,
        const CommandPool &commandPool,
        Application& application // <-- ADD THIS
    );

    void recreate();

    /**
     * @brief This is the new "cooking" function.
     * It's called every frame to record all draw calls.
     */
    void recordCommands(uint32_t imageIndex);

    void createCommandBuffers() override;

private:
    // This function is being removed, its logic moves to recordCommands
    // void createCommandBuffers();

    // We need this to get the scene data
    Application& m_app;
};
} // namespace vks
