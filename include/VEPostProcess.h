#pragma once

namespace vve {

    // Wendet die Vienna Vulkan Post Processing Library auf das gerenderte Bild an.
    class PostProcess : public System {
        public:
            PostProcess(std::string systemName, Engine& engine);
            ~PostProcess();

            std::unique_ptr<vvppl::PostProcessing> m_pp;
            std::vector<VkCommandPool> m_commandPools;

            vvppl::GreyscaleSettings*  m_greyscale{nullptr};
            vvppl::VignetteSettings*   m_vignette{nullptr};
            vvppl::FilmGrainSettings*  m_filmGrain{nullptr};
            vvppl::ChromaticSettings*  m_chromatic{nullptr};
            vvppl::TonemapSettings*    m_tonemap{nullptr};
            
        private:
            bool OnInit(Message message);
            bool OnPrepareNextFrame(Message message);
            bool OnRecordNextFrame(Message message);
            bool OnQuit(Message message);
        };

}; // namespace vve
