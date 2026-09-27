// Copyright 2019-2024 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <QWidget>
#include "common/common_types.h"

namespace Settings {
enum class StereoRenderOption : u32;
}

namespace ConfigurationShared {
enum class CheckState;
}

namespace Ui {
class ConfigureLayout;
}

class ConfigureLayout : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureLayout(QWidget* parent = nullptr);
    ~ConfigureLayout();

    void ApplyConfiguration();
    void RetranslateUI();
    void SetConfiguration();

    void SetupPerGameUI();

private:
    void updateShaders(Settings::StereoRenderOption stereo_option);
    void updateTextureFilter(int index);

    void UpdateBottomOverlaySummary();

    std::unique_ptr<Ui::ConfigureLayout> ui;
    ConfigurationShared::CheckState swap_screen;
    ConfigurationShared::CheckState upright_screen;
    ConfigurationShared::CheckState bottom_overlay_enabled;
    std::string bottom_overlays;
    // Per-game only: whether this game has its own overlays instead of the global ones
    bool bottom_overlays_per_game = false;
    QColor bg_color;
};
