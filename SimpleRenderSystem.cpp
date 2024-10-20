//
// Created by antoi on 20/10/2024.
//

#include "SimpleRenderSystem.h"

#include <array>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Liara
{
    struct SimplePushConstantData
    {
        glm::mat2 transform{1.0f};
        glm::vec2 offset;
        alignas(16) glm::vec3 color;
    };

    SimpleRenderSystem::SimpleRenderSystem(Liara_Device& device, VkRenderPass render_pass) : m_Device(device)
    {
        CreatePipelineLayout();
        CreatePipeline(render_pass);
    }

    SimpleRenderSystem::~SimpleRenderSystem()
    {
        vkDestroyPipelineLayout(m_Device.GetDevice(), m_PipelineLayout, nullptr);
    }

    void SimpleRenderSystem::RenderGameObjects(VkCommandBuffer commandBuffer, std::vector<Liara_GameObject> &game_objects) const
    {
        m_Pipeline->Bind(commandBuffer);

        for (auto& obj : game_objects)
        {
            SimplePushConstantData push{};
            push.offset = obj.m_Transform.position;
            push.color = obj.m_color;
            push.transform = obj.m_Transform.GetMat2();
            vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
            obj.m_Model->Bind(commandBuffer);
            obj.m_Model->Draw(commandBuffer);
        }
    }

    void SimpleRenderSystem::CreatePipelineLayout()
    {
        VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData)};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(m_Device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
    }

    void SimpleRenderSystem::CreatePipeline(VkRenderPass render_pass)
    {
        assert(m_PipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Liara_Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.m_RenderPass = render_pass;
        pipelineConfig.m_PipelineLayout = m_PipelineLayout;
        m_Pipeline = std::make_unique<Liara_Pipeline>(m_Device, "shaders/SimpleShader.vert.spv", "shaders/SimpleShader.frag.spv", pipelineConfig);
    }
}