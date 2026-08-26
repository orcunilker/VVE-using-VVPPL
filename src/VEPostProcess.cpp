#include "VHInclude.h"
#include "VEInclude.h"

namespace vve{

    PostProcess::PostProcess(std::string systenName, Engine& engine)
        : System(systenName, engine) {

            m_engine.RegisterCallbacks({
                {this, 2500, "RECORD_NEXT_FRAME", [this](Message& message){ return OnRecordNextFrame(message); }}
            });
    };

    PostProcess::~PostProcess() {};



    bool PostProcess::OnRecordNextFrame(Message message) {
        std::cout << "post process\n";
        return false;
    }

};