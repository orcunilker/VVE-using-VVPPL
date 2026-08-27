#include "VHInclude.h"
#include "VEInclude.h"

namespace vve{

    PostProcess::PostProcess(std::string systenName, Engine& engine)
        : System(systenName, engine) {

            m_engine.RegisterCallbacks({
                {this, 2500, "RECORD_NEXT_FRAME", [this](Message& message){ return OnRecordNextFrame(message); }},
                {this, 4500, "INIT", [this](Message& message){ return OnInit(message); }},
                {this, 2500, "PREPARE_NEXT_FRAME", [this](Message& message){return OnPrepareNextFrame(message); }},
                {this, 1000, "QUIT", [this](Message& message){ return OnQuit(message); }}
            });
    };

    PostProcess::~PostProcess() {};

    bool PostProcess::OnInit(Message message) {
        auto[handle, state] = Renderer::GetState(m_registry);

        // ein Command Pool pro Frame in Flight, wie bei den Renderern
        m_commandPools.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto& pool : m_commandPools) {
            vvh::ComCreateCommandPool({
                .m_surface = state().m_surface,
                .m_physicalDevice = state().m_physicalDevice,
                .m_device = state().m_device,
                .m_queueFamilyIndex = state().m_queueFamilies.graphicsFamily.value(),
                .m_commandPool = pool
            });
        }

        // die Library bekommt nur Device, Physical Device und die Bildgröße
        m_pp = std::make_unique<vvppl::PostProcessing>(
            state().m_device,
            state().m_physicalDevice,
            state().m_swapChain.m_swapChainExtent.width,
            state().m_swapChain.m_swapChainExtent.height,
            MAX_FRAMES_IN_FLIGHT
        );
        
        m_greyscale = &m_pp->addGreyscale();
        m_greyscale->strength = 0.2f;

        m_vignette = &m_pp->addVignette();
        m_vignette->intensity = 0.2f;

        m_filmGrain = &m_pp->addFilmGrain();
        m_filmGrain->intensity = 0.03f;

        m_chromatic = &m_pp->addChromatic();
        m_chromatic->intensity = 0.07f;

        m_tonemap = &m_pp->addTonemap();
        m_tonemap->exposure = 0.9f;

        return false;
    }
    
    bool PostProcess::OnPrepareNextFrame(Message message) {
        auto[handle, state] = Renderer::GetState(m_registry);
        vkResetCommandPool(state().m_device, m_commandPools[state().m_currentFrame], 0);

        // Filmgrain next seed
		m_filmGrain->time += 15;
    }

    bool PostProcess::OnRecordNextFrame(Message message) {
        
        auto [handle, state] = Renderer::GetState(m_registry);
        VkImage image = state().m_swapChain.m_swapChainImages[state().m_imageIndex];

        std::vector<VkCommandBuffer> cmdBuffers(1);
        vvh::ComCreateCommandBuffers({
            .m_device = state().m_device,
            .m_commandPool = m_commandPools[state().m_currentFrame],
            .m_commandBuffers = cmdBuffers
        });
        auto cmd = cmdBuffers[0];

        vvh::ComBeginCommandBuffer({cmd});

        VkImageSubresourceRange range {};
        range.aspectMask    = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel  = 0;
        range.levelCount    = 1;
        range.baseArrayLayer= 0;
        range.layerCount    = 1;

        // die Library erwartet GENERAL, der Render Pass hinterlässt COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier toGeneral{};
        toGeneral.sType             = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toGeneral.oldLayout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toGeneral.newLayout         = VK_IMAGE_LAYOUT_GENERAL;
        toGeneral.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
        toGeneral.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
        toGeneral.image             = image;
        toGeneral.srcAccessMask     = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        toGeneral.dstAccessMask     = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        toGeneral.subresourceRange  = range;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &toGeneral);

        m_pp->apply(cmd, image, image, state().m_currentFrame);

        // zurück, weil ImGui auf Phase 3000 einen renderpass darauf startet
        VkImageMemoryBarrier toAttachment = toGeneral;
        toAttachment.oldLayout      = VK_IMAGE_LAYOUT_GENERAL;
        toAttachment.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        toAttachment.srcAccessMask  = VK_ACCESS_TRANSFER_WRITE_BIT;
        toAttachment.dstAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &toAttachment);

        vvh::ComEndCommandBuffer({.m_commandBuffer = cmd});

        state().m_commandBuffersSubmit.push_back(cmd);

        return false;
    }

    bool PostProcess::OnQuit(Message message) {
        auto[handle, state] = Renderer::GetState(m_registry);
        vkDeviceWaitIdle(state().m_device); // warten bis die objekte nicht mehr benutzt werden
        // interessant: erstes mal wo ich in die VVE aktiv eingreife

        m_pp.reset();

        for (auto& pool : m_commandPools) {
            vkDestroyCommandPool(state().m_device, pool, nullptr);
        }
        return false;
    }

};