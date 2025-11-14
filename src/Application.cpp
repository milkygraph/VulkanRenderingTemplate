#include <vks/Application.hpp>
#include <vks/Buffer.hpp>   // Need to include
#include <vks/Material.hpp> // Need to include
#include <vks/Model.hpp>    // Need to include
#include <stdexcept>
#include <iostream>
#include <array>

// Add these for matrix math and time
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

using namespace vks;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

vks::Application::Application()
    : instance("Hello Triangle", "No Engine", true),
      debugMessenger(instance),
      window({WIDTH, HEIGHT}, "Vulkan", instance),
      device(instance, window, Instance::DeviceExtensions),
      swapChain(device, window),
      renderPass(device, swapChain),
      // --- THIS IS THE FIX FROM THE PREVIOUS ERROR ---
      // Pass the "reset" flag to the CommandPool constructor
      commandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
      // --- END FIX ---
      graphicsPipeline(device, swapChain, renderPass),
      // We must pass 'graphicsPipeline' to the base CommandBuffers
      commandBuffers(device, renderPass, swapChain, graphicsPipeline, commandPool, *this),
      syncObjects(device, swapChain.numImages(), MAX_FRAMES_IN_FLIGHT),
      interface(instance, window, device, swapChain, graphicsPipeline)
{
    m_app = this;

    // Now that all core systems are up, load assets
    loadAssets();
    buildScene();
}

/**
 * @brief Creates all asset registries (pools, models, materials).
 */
void Application::loadAssets() {
    // 1. Create Global Descriptor Pool
    m_globalDescriptorPool = vks::DescriptorPool::Builder(device)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100) // For camera + materials
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100) // For textures
        .setMaxSets(200)
        .build();

    // 2. Create the Camera UBO Buffer
    m_cameraUboBuffer = std::make_unique<vks::Buffer>(
        device,
        sizeof(CameraUBO),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    // Map it persistently. We can write to it at any time.
    m_cameraUboBuffer->map();

    // 3. Create the Camera Descriptor Set (Set 0)
    // (This was the other fix: we create the set that points to the buffer)
    {
        auto globalSetLayout = graphicsPipeline.getDescriptorSetLayout("global");
        auto bufferInfo = m_cameraUboBuffer->descriptorInfo();
        vks::DescriptorWriter(globalSetLayout, m_globalDescriptorPool)
            .writeBuffer(0, &bufferInfo)
            .build(m_cameraDescriptorSet); // m_cameraDescriptorSet is now valid!
    }

    // 4. Create Models
    // This calls Model::createSphere, which uses your sphere generation code
    // and uploads it to the GPU.
    m_models["sphere"].createSphere(device, commandPool.handle(), 1.0f, 32, 16);

    // 5. Create Materials
    m_materials.emplace("red_sphere",
        vks::Material{
            device,
            graphicsPipeline,
            m_globalDescriptorPool,
            "sphere", // The name of the pipeline to use
            {1.0f, 0.0f, 0.0f, 1.0f} // Red
        }
    );

    m_materials.emplace("blue_sphere",
        vks::Material{
            device,
            graphicsPipeline,
            m_globalDescriptorPool,
            "sphere", // Same pipeline
            {0.0f, 0.2f, 0.8f, 1.0f} // Blue
        }
    );
}

/**
 * @brief Populates the m_renderObjects list.
 */
void Application::buildScene() {
    // Create a red sphere at (0, 0, 0)
    RenderObject redSphere;
    redSphere.model = &m_models.at("sphere"); // Use .at() to avoid default constructor
    redSphere.material = &m_materials.at("red_sphere");
    redSphere.transform = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 0.0f});
    m_renderObjects.push_back(redSphere);

    // Create a blue sphere at (2, 0, 0)
    RenderObject blueSphere;
    blueSphere.model = &m_models.at("sphere");
    blueSphere.material = &m_materials.at("blue_sphere");
    blueSphere.transform = glm::translate(glm::mat4(1.0f), {2.0f, 0.0f, 0.0f});
    m_renderObjects.push_back(blueSphere);
}

void Application::updateUBOs(uint32_t currentImage) {
    // --- Update Camera UBO ---
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    CameraUBO ubo{};

    // --- CAMERA FIX ---
    // Let's pull the camera back to (5, 5, 5) to get a wider view
    // and keep the Up vector as (0, 0, 1) (Z-up)
    ubo.view = glm::lookAt(
        glm::vec3(5.0f, 5.0f, 5.0f), // <-- Pulled camera back
        glm::vec3(0.0f, 0.0f, 0.0f), // <-- Looking at the origin
        glm::vec3(0.0f, 0.0f, 1.0f)  // <-- Z-up
    );

    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        swapChain.extent().width / (float) swapChain.extent().height,
        0.1f,
        100.0f
    );

    // Flip Y for Vulkan
    ubo.proj[1][1] *= -1;

    // Write to the mapped buffer
    m_cameraUboBuffer->writeToBuffer(&ubo, sizeof(ubo));

    // Let's make the red sphere orbit
    // get current transform
    m_renderObjects[0].transform = glm::rotate(glm::mat4(1.0f), 1000 * time * glm::radians(45.0f), {0.0f, 0.0f, 1.0f});

    // Keep the blue sphere static
    m_renderObjects[1].transform = glm::translate(glm::mat4(1.0f), {2.0f, 0.0f, 0.0f});
}

void Application::run() {
    window.setDrawFrameFunc([this](bool& framebufferResized)
    {
        drawImGui();
        drawFrame(framebufferResized);
    });

    window.mainLoop();
    vkDeviceWaitIdle(device.logical());
}

// This function is not used
void Application::mainLoop() {}


void Application::drawFrame(bool &framebufferResized) {
  vkWaitForFences(device.logical(), 1, &syncObjects.inFlightFence(currentFrame),
                    VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device.logical(), swapChain.handle(), UINT64_MAX,
      syncObjects.imageAvailable(currentFrame), VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain(framebufferResized);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swapchain image");
  }

  // Update all UBOs with fresh data for this frame
  // *before* we record the command buffer.
  updateUBOs(currentFrame);

  if (syncObjects.imageInFlight(imageIndex) != VK_NULL_HANDLE) {
    vkWaitForFences(device.logical(), 1, &syncObjects.imageInFlight(imageIndex),
                    VK_TRUE, UINT64_MAX);
  }
  syncObjects.imageInFlight(imageIndex) =
      syncObjects.inFlightFence(currentFrame);

  // --- Record the command buffers ---
  // (This will now read the UBO data we just wrote)
  commandBuffers.recordCommands(imageIndex); // Your BasicCommandBuffers
  interface.recordCommandBuffers(imageIndex);  // ImGui

  // --- Submit ---
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {syncObjects.imageAvailable(currentFrame)};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  // Submit BOTH command buffers (scene and ImGui)
  std::array<VkCommandBuffer, 2> cmdBuffers = {
      commandBuffers.command(imageIndex),
      interface.command(imageIndex)
  };
  submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
  submitInfo.pCommandBuffers = cmdBuffers.data();

  VkSemaphore signalSemaphores[] = {syncObjects.renderFinished(imageIndex)};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(device.logical(), 1, &syncObjects.inFlightFence(currentFrame));

  if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo,
                    syncObjects.inFlightFence(currentFrame)) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  // --- Present ---
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain.handle()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    recreateSwapChain(framebufferResized);
    syncObjects.recreate(swapChain.numImages());
    framebufferResized = false;
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::drawImGui() {
  // Start the Dear ImGui frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  static float f = 0.0f;
  static int counter = 0;

  ImGui::Begin("Renderer Options");
  ImGui::Text("This is some useful text.");
  ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
  if (ImGui::Button("Button")) {
    counter++;
  }
  ImGui::SameLine();
  ImGui::Text("counter = %d", counter);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();

  ImGui::Render();
}

// for resize window
void Application::recreateSwapChain(bool &framebufferResized) {
  framebufferResized = true;

  glm::ivec2 size;
  window.framebufferSize(size);
  while (size[0] == 0 || size[1] == 0) {
    window.framebufferSize(size);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device.logical());

  swapChain.recreate();
  renderPass.recreate();
  graphicsPipeline.recreate(); // Recreates all pipeline "recipes"
  commandBuffers.recreate();   // Re-allocates the command buffers
  interface.recreate();

  renderPass.cleanupOld();
  swapChain.cleanupOld();
}
