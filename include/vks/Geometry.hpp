#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <vector>
#include <glm/glm.hpp>

#include "Descriptors.hpp"

namespace vks {
namespace geometry {

struct Vertex
{
    float pos[3]; // layout(location = 0)
    float normal[3]; // layout(location = 1)
    float uv[2]; // layout(location = 2)

    // Helper function to tell Vulkan about this struct's memory layout
    static VkVertexInputBindingDescription getBindingDescription();

    // Helper function to tell Vulkan about each member
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

    void createSphere(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, float radius, uint32_t sectors,
                      uint32_t stacks);
} // namespace geometry
} // namespace vks

#endif // GEOMETRY_HPP
