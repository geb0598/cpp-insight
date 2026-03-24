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

    if (!context.is_connected) {
        if (ImGui::Button("Connect")) {
            server.Listen();
        }
    } else {
        if (ImGui::Button("Disconnect")) {
            server.Stop();
        }
        ImGui::SameLine();

        if (!context.is_recording) {
            if (ImGui::Button("Record")) {
                server.StartSession();
                context.is_recording   = true;
                context.needs_reset    = true;
                context.timeline_begin = 0;
                context.timeline_end   = 0;
            }
        } else {
            if (ImGui::Button("Stop")) {
                server.StopSession();
                context.is_recording = false;
                context.timeline_end = insight::Reporter::GetInstance().Size();
            }
        }
    }

    ImGui::SameLine();

    ImGui::TextColored(
        context.is_connected
            ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
            : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        context.is_connected ? "Connected" : "Disconnected");

    ImGui::Separator();
}

} // namespace insight::viewer