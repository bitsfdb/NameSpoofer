#include "pch.h"
#include "NameSpoofer.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/bakkesmodsdk.h"
#include <string>

BAKKESMOD_PLUGIN(NameSpoofer, "NameSpoofer", "0.1", PLUGINTYPE_FREEPLAY | PLUGINTYPE_CUSTOM_TRAINING | PLUGINTYPE_SPECTATOR | PLUGINTYPE_REPLAY)

void NameSpoofer::onLoad()
{
    _globalCvarManager = cvarManager;
    cvarManager->log("[NameSpoofer] Loaded");

    spoofedName = std::make_shared<std::string>("");
    isEnabled = std::make_shared<bool>(false);

    cvarManager->registerNotifier("sm_name", [this](std::vector<std::string> params) {
        if (params.size() < 2) {
            cvarManager->log("[NameSpoofer] Usage: sm_name <newName>");
            return;
        }
        *spoofedName = params[1];
        *isEnabled = true;
        cvarManager->log("[NameSpoofer] Name set to: " + *spoofedName);
    }, "Spoof your local display name", PERMISSION_ALL);

    cvarManager->registerNotifier("sm_name_off", [this](std::vector<std::string> params) {
        *isEnabled = false;
        spoofedName->clear();
        originalName.clear();
        cvarManager->log("[NameSpoofer] Disabled");
    }, "Disable name spoofing", PERMISSION_ALL);

    HookEvents();
}

void NameSpoofer::HookEvents()
{
    // Hook scoreboard rendering
    gameWrapper->HookEventPost("Function TAGame.GFxData_GameEvent_TA.UpdateScoreboard",
        [this](std::string) { ApplySpoofedName(); });

    // Hook when entering any game
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.InitGame",
        [this](std::string) { 
            originalName.clear(); // Reset for new match
        });

    // Hook the HUD stat ticker
    gameWrapper->HookEventPost("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
        [this](std::string) { ApplySpoofedName(); });

    // Hook goal scored
    gameWrapper->HookEventPost("Function TAGame.PRI_TA.EventScoredGoal",
        [this](std::string) { ApplySpoofedName(); });

    // Hook assist
    gameWrapper->HookEventPost("Function TAGame.PRI_TA.EventAssist",
        [this](std::string) { ApplySpoofedName(); });

    // Hook for replay viewing
    gameWrapper->HookEventPost("Function TAGame.Replay_TA.EventSpawned",
        [this](std::string) { ApplySpoofedNameInReplay(); });

    gameWrapper->HookEventPost("Function TAGame.ReplayDirector_TA.OnSoccarGameSet",
        [this](std::string) { ApplySpoofedNameInReplay(); });
}

void NameSpoofer::ApplySpoofedNameInReplay()
{
    if (!isEnabled || !spoofedName || !*isEnabled || spoofedName->empty() || !gameWrapper)
        return;

    if (!gameWrapper->IsInReplay())
        return;

    ServerWrapper server = gameWrapper->GetGameEventAsReplay();
    if (!server) return;

    auto localController = gameWrapper->GetPlayerController();
    if (!localController) return;

    auto localPRI = localController.GetPRI();
    if (!localPRI) return;

    if (originalName.empty()) {
        originalName = localPRI.GetPlayerName().ToString();
        cvarManager->log("[NameSpoofer] Original name stored (Replay): " + originalName);
    }

    auto localUID = localPRI.GetUniqueIdWrapper().GetIdString();

    auto PRIs = server.GetPRIs();
    for (auto pri : PRIs) {
        if (!pri) continue;
        
        auto priUID = pri.GetUniqueIdWrapper().GetIdString();
        if (priUID == localUID) {
            bool success = pri.ChangeNameForScoreboardAndNameplateInReplay(*spoofedName);
            if (success) {
                cvarManager->log("[NameSpoofer] Applied name in replay: " + *spoofedName);
            }
        }
    }
}

void NameSpoofer::ApplySpoofedName()
{
    if (!isEnabled || !spoofedName || !*isEnabled || spoofedName->empty() || !gameWrapper)
        return;

    // If in replay, use the replay-specific method
    if (gameWrapper->IsInReplay()) {
        ApplySpoofedNameInReplay();
        return;
    }

    // Skip online games entirely
    if (gameWrapper->IsInOnlineGame())
        return;

    if (!gameWrapper->IsInGame())
        return;

    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) return;

    auto localController = gameWrapper->GetPlayerController();
    if (!localController) return;

    auto localPRI = localController.GetPRI();
    if (!localPRI) return;

    if (originalName.empty()) {
        originalName = localPRI.GetPlayerName().ToString();
        cvarManager->log("[NameSpoofer] Original name stored: " + originalName);
    }

    auto localUID = localPRI.GetUniqueIdWrapper().GetIdString();

    auto PRIs = server.GetPRIs();
    for (auto pri : PRIs) {
        if (!pri) continue;
        
        auto priUID = pri.GetUniqueIdWrapper().GetIdString();
        if (priUID == localUID) {
            pri.eventSetPlayerName(*spoofedName);
        }
    }
}

void NameSpoofer::onUnload()
{
    cvarManager->log("[NameSpoofer] Unloaded");
}

// PluginSettingsWindow interface implementations
void NameSpoofer::RenderSettings()
{
    static char nameBuffer[128] = "";
    
    ImGui::TextUnformatted("Name Spoofer Settings | by bitsfdb");
    ImGui::Separator();
    
    if (!isEnabled || !spoofedName) {
        ImGui::Text("Plugin not fully initialized");
        return;
    }
    
    ImGui::Text("Status: %s", *isEnabled ? "Enabled" : "Disabled");
    
    if (*isEnabled && !spoofedName->empty()) {
        ImGui::Text("Current spoofed name: %s", spoofedName->c_str());
    }

    ImGui::Spacing();
    
    // Show current mode
    if (gameWrapper->IsInReplay()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), 
            "Mode: Replay (Scoreboard & nameplate)");
    } else if (gameWrapper->IsInOnlineGame()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
            "Mode: Online (Not supported)");
    } else if (gameWrapper->IsInGame()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), 
            "Mode: Offline (Full name spoof active)");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
            "Mode: Not in game");
    }
    
    ImGui::Spacing();
    ImGui::InputText("New Name", nameBuffer, sizeof(nameBuffer));
    
    if (ImGui::Button("Apply Name")) {
        if (strlen(nameBuffer) > 0) {
            *spoofedName = nameBuffer;
            *isEnabled = true;
            cvarManager->log("[NameSpoofer] Name set to: " + *spoofedName);
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Disable")) {
        *isEnabled = false;
        spoofedName->clear();
        cvarManager->log("[NameSpoofer] Disabled");
    }
}

std::string NameSpoofer::GetPluginName()
{
    return "NameSpoofer";
}

void NameSpoofer::SetImGuiContext(uintptr_t ctx)
{
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}
