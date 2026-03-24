#include <chrono>
#include <thread>
#include <iostream>

#include "insight/insight.h"

INSIGHT_DECLARE_STATGROUP("Physics",   STATGROUP_PHYSICS);
INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);

INSIGHT_DECLARE_CYCLE_STAT("PhysicsUpdate", STAT_PHYSICS,  STATGROUP_PHYSICS);
INSIGHT_DECLARE_CYCLE_STAT("BVHTraverse",   STAT_BVH,      STATGROUP_PHYSICS);
INSIGHT_DECLARE_CYCLE_STAT("DrawCall",      STAT_DRAW,     STATGROUP_RENDERING);

int main() {
    std::cout << "Connecting to viewer...\n";

    insight::Client::GetInstance().Connect();
    std::cout << "Connect thread started. Running game loop...\n";

    while (true) {
        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_PHYSICS);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            {
                INSIGHT_SCOPE_CYCLE_COUNTER(STAT_BVH);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_DRAW);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        INSIGHT_FRAME_END();

        std::cout << "is_sending: "
            << insight::Client::GetInstance().IsSessionActive()
            << "\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    insight::Client::GetInstance().Disconnect();
    return 0;
}