#pragma once

namespace vve {

    // Wendet die Vienna Vulkan Post Processing Library auf das gerenderte Bild an.
    class PostProcess : public System {
        public:
            PostProcess(std::string systemName, Engine& engine);
            ~PostProcess();
            
        private:
            bool OnRecordNextFrame(Message message);
        };

}; // namespace vve
