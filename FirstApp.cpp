//
// Created by antoi on 15/10/2024.
//

#include "FirstApp.h"

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

    FirstApp::FirstApp()
    {
        LoadGameObjects();
        CreatePipelineLayout();
        CreateSwapChain();
        CreateCommandBuffers();
    }

    FirstApp::~FirstApp()
    {
        vkDestroyPipelineLayout(m_Device.GetDevice(), m_PipelineLayout, nullptr);
    }

    void FirstApp::Run()
    {
        while (!m_Window.ShouldClose())
        {
            glfwPollEvents();
            DrawFrame();
        }

        vkDeviceWaitIdle(m_Device.GetDevice());
    }

    void FirstApp::CreatePipeline()
    {
        assert(m_SwapChain != nullptr && "Cannot create pipeline before swap chain");
        assert(m_PipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        Liara_Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.m_RenderPass = m_SwapChain->GetRenderPass();
        pipelineConfig.m_PipelineLayout = m_PipelineLayout;
        m_Pipeline = std::make_unique<Liara_Pipeline>(m_Device, "shaders/SimpleShader.vert.spv", "shaders/SimpleShader.frag.spv", pipelineConfig);
    }

    void FirstApp::CreatePipelineLayout()
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

    void FirstApp::CreateCommandBuffers()
    {
        m_CommandBuffers.resize(m_SwapChain->ImageCount());

        VkCommandBufferAllocateInfo commandBufferAllocInfo{};
        commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocInfo.commandPool = m_Device.GetCommandPool();
        commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device.GetDevice(), &commandBufferAllocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void FirstApp::FreeCommandBuffers()
    {
        vkFreeCommandBuffers(
            m_Device.GetDevice(),
            m_Device.GetCommandPool(),
            static_cast<uint32_t>(m_CommandBuffers.size()),
            m_CommandBuffers.data());
        m_CommandBuffers.clear();
    }


    void FirstApp::DrawFrame()
    {
        uint32_t imageIndex;
        auto result = m_SwapChain->AcquireNextImage(&imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            CreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire next image!");
        }

        RecordCommandBuffer(imageIndex);
        result = m_SwapChain->SubmitCommandBuffers(&m_CommandBuffers[imageIndex], &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.WasResized())
        {
            m_Window.ResetResizedFlag();
            CreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit command buffer!");
        }
    }

    void FirstApp::CreateSwapChain()
    {
        auto extent = m_Window.GetExtent();
        while (extent.width == 0 || extent.height == 0)
        {
            glfwWaitEvents();
            extent = m_Window.GetExtent();
        }

        vkDeviceWaitIdle(m_Device.GetDevice());
        m_SwapChain = nullptr;

        if (m_SwapChain == nullptr)
        {
            m_SwapChain = std::make_unique<Liara_SwapChain>(m_Device, extent);
        }
        else
        {
            m_SwapChain = std::make_unique<Liara_SwapChain>(m_Device, extent, std::move(m_SwapChain));
            if (m_SwapChain->ImageCount() != m_CommandBuffers.size())
            {
                FreeCommandBuffers();
                CreateCommandBuffers();
            }
        }

        CreatePipeline();
    }


    void FirstApp::RecordCommandBuffer(const uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(m_CommandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_SwapChain->GetRenderPass();
        renderPassInfo.framebuffer = m_SwapChain->GetFrameBuffer(static_cast<int>(imageIndex));
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_CommandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
        viewport.height = static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        const VkRect2D scissor{{0, 0}, m_SwapChain->GetSwapChainExtent()};
        vkCmdSetViewport(m_CommandBuffers[imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(m_CommandBuffers[imageIndex], 0, 1, &scissor);

        RenderGameObjects(m_CommandBuffers[imageIndex]);

        vkCmdEndRenderPass(m_CommandBuffers[imageIndex]);
        if (vkEndCommandBuffer(m_CommandBuffers[imageIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void FirstApp::SierpinskiTriangle(std::vector<Liara_Model::Vertex> &vertices, int depth, Liara_Model::Vertex v0, Liara_Model::Vertex v1, Liara_Model::Vertex v2)
    {
        if (depth <= 0)
        {
            vertices.push_back({v0});
            vertices.push_back({v1});
            vertices.push_back({v2});
        }
        else
        {
            // Calculate midpoints of sides, and interpolate color
            Liara_Model::Vertex v01 = {(v0.position + v1.position) / 2.0f, (v0.color + v1.color) / 2.0f};
            Liara_Model::Vertex v12 = {(v1.position + v2.position) / 2.0f, (v1.color + v2.color) / 2.0f};
            Liara_Model::Vertex v20 = {(v2.position + v0.position) / 2.0f, (v2.color + v0.color) / 2.0f};

            SierpinskiTriangle(vertices, depth - 1, v0, v01, v20);
            SierpinskiTriangle(vertices, depth - 1, v01, v1, v12);
            SierpinskiTriangle(vertices, depth - 1, v20, v12, v2);
        }
    }

    void FirstApp::LoadGameObjects()
    {
        // A basic triangle
        /*std::vector<Liara_Model::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        const auto model = std::make_shared<Liara_Model>(m_Device, vertices);
        auto triangle = Liara_GameObject::CreateGameObject();
        triangle.m_Model = model;
        m_GameObjects.push_back(std::move(triangle));*/

        // A Sierpinski triangle
        /*std::vector<Liara_Model::Vertex> vertices;
        SierpinskiTriangle(vertices, 5, {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}, {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}});
        const auto model = std::make_shared<Liara_Model>(m_Device, vertices);
        auto triangle = Liara_GameObject::CreateGameObject();
        triangle.m_Model = model;
        triangle.m_color = {0.0f, 0.8f, 1.0f};
        m_GameObjects.push_back(std::move(triangle));*/

        // A bunch of triangles
        std::vector<Liara_Model::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        auto model = std::make_shared<Liara_Model>(m_Device, vertices);

        std::vector<glm::vec3> colors{
            {1.f, .7f, .73f},
            {1.f, .87f, .73f},
            {1.f, 1.f, .73f},
            {.73f, 1.f, .8f},
            {.73, .88f, 1.f}
        };

        for (auto& color : colors) { color = glm::pow(color, glm::vec3{2.2f}); }

        for (int i = 0; i < 40; i++)
        {
            auto triangle = Liara_GameObject::CreateGameObject();
            triangle.m_Model = model;
            triangle.m_Transform.scale = glm::vec2(.5f) + i * 0.025f;
            triangle.m_Transform.rotation = i * glm::pi<float>() * .025f;
            triangle.m_color = colors[i % colors.size()];
            m_GameObjects.push_back(std::move(triangle));
        }
    }

    void FirstApp::RenderGameObjects(VkCommandBuffer commandBuffer)
    {
        int i = 0;
        for (auto& obj : m_GameObjects)
        {
            i++;
            obj.m_Transform.rotation = glm::mod<float>(obj.m_Transform.rotation + 0.001f * i, 2.f * glm::pi<float>());
        }

        m_Pipeline->Bind(commandBuffer);

        for (auto& obj : m_GameObjects)
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

}
