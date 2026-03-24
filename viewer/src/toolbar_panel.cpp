#include <imgui.h>

#include "insight/reporter.h"

#include "toolbar_panel.h"

namespace insight::viewer {

void ToolbarPanel::Render() {
    auto& server = Server::GetInstance();
    auto& context = GetContext();

    if (ImGui::Button("Save")) {
        // @todo
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        // @todo
    }
    ImGui::SameLine();
    
    ImGui::TextDisabled("|"); 
    ImGui::SameLine();

    switch (context.server_state) {
        
        case ServerState::OFFLINE:
            if (ImGui::Button("Listen")) {
                server.Listen();
                context.server_state = ServerState::LISTENING; 
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Offline (Local Mode)");
            break;

        case ServerState::LISTENING:
            if (ImGui::Button("Stop Listening")) {
                server.Stop();
                context.server_state = ServerState::OFFLINE;
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Listening for Client...");
            break;

        case ServerState::CONNECTED:
            if (ImGui::Button("Disconnect")) {
                server.Stop();
                context.server_state = ServerState::OFFLINE;
            }
            ImGui::SameLine();
            if (ImGui::Button("Record")) {
                server.StartRecording();
                context.server_state = ServerState::RECORDING;
                context.needs_reset = true;
                context.timeline_begin = 0;
                context.timeline_end = 0;
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected (Idle)");
            break;

        case ServerState::RECORDING:
            if (ImGui::Button("Disconnect")) {
                server.Stop();
                context.server_state = ServerState::OFFLINE;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Recording")) {
                server.StopRecording();
                context.server_state = ServerState::CONNECTED;
                context.timeline_end = insight::Reporter::GetInstance().Size();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Recording...");
            break;
    }

    ImGui::Separator();
}

} // namespace insight::viewer