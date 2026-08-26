#pragma once

namespace vve {

    // Wendet die Vienna Vulkan Post Processing Library auf das gerenderte Bild an.
    class PostProcess : public System {
        public:
            PostProcess(std::string systemName, Engine& engine);
            ~PostProcess();

            std::unique_ptr<vvppl::PostProcessing> m_pp;
            std::vector<VkCommandPool> m_commandPools;
            
        private:
            bool OnInit(Message message);
            bool OnPrepareNextFrame(Message message);
            bool OnRecordNextFrame(Message message);
            bool OnQuit(Message message);
        };

}; // namespace vve
