#include <array>
#include <windows.h>
#include <commdlg.h>

#include <imgui.h>

#include "insight/reporter.h"

#include "toolbar_panel.h"

namespace insight::viewer {

void ToolbarPanel::Render() {
    auto& server  = Server::GetInstance();
    auto& context = GetContext();

    if (ImGui::Button("Save")) {
        auto default_name = Server::GenerateSessionFilename().string();

        std::array<char, MAX_PATH> path_buf{};
        std::copy(default_name.begin(), default_name.end(), path_buf.begin());

        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = "Trace Files (*.trace)\0*.trace\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = path_buf.data();
        ofn.nMaxFile    = static_cast<DWORD>(path_buf.size());
        ofn.lpstrDefExt = "trace";
        ofn.Flags       = OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameA(&ofn)) {
            try {
                server.SaveSession(path_buf.data());
                status_message_ = "Saved: " + std::string(path_buf.data());
            } catch (const std::exception& e) {
                status_message_ = std::string("Save failed: ") + e.what();
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        std::array<char, MAX_PATH> path_buf{};

        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = "Trace Files (*.trace)\0*.trace\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = path_buf.data();
        ofn.nMaxFile    = static_cast<DWORD>(path_buf.size());
        ofn.lpstrDefExt = "trace";
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameA(&ofn)) {
            try {
                server.Stop();
                server.LoadSession(path_buf.data());
                context.server_state   = ServerState::OFFLINE;
                context.needs_reset    = true;
                context.timeline_begin = 0;
                context.timeline_end   = Reporter::GetInstance().Size();
                status_message_ = "Loaded: " + std::string(path_buf.data());
            } catch (const std::exception& e) {
                status_message_ = std::string("Load failed: ") + e.what();
            }
        }
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

    if (!status_message_.empty()) {
        ImGui::TextDisabled("%s", status_message_.c_str());
    }
}

} // namespace insight::viewer