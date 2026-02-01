#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <string>

class NameSpoofer : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
public:
    void onLoad() override;
    void onUnload() override;

    void RenderSettings() override;
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;

private:
    void ApplySpoofedName();
    void ApplySpoofedNameInReplay();
    void HookEvents();

    std::string originalName;
    std::shared_ptr<std::string> spoofedName;
    std::shared_ptr<bool> isEnabled;
};
