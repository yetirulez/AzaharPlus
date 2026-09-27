// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include <QDialog>
#include <QImage>
#include <QTransform>
#include <QWidget>
#include "common/bottom_overlay.h"

class QCheckBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
class QSpinBox;

/**
 * Visual editor surface: shows the top screen above the bottom screen and lets the user drag and
 * resize each overlay's source rectangle (on the bottom screen) and destination rectangle (on the
 * top screen).
 */
class BottomOverlayCanvas : public QWidget {
    Q_OBJECT

public:
    explicit BottomOverlayCanvas(std::vector<Settings::BottomScreenOverlay>& overlays,
                                 QWidget* parent = nullptr);

    void SetSelected(int index);
    int Selected() const {
        return selected;
    }
    void SetScreenImages(const QImage& top, const QImage& bottom);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void SelectionChanged(int index);
    void OverlayEdited(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum class Target { None, Source, Destination };
    enum class DragMode {
        None,
        Move,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight,
    };

    struct Hit {
        int index = -1;
        Target target = Target::None;
        DragMode mode = DragMode::None;
    };

    // Scene coordinates are native 3DS pixels: the top screen occupies (0,0)-(400,240) and the
    // bottom screen sits centered beneath it, separated by a small gap.
    QRectF TopScreenScene() const;
    QRectF BottomScreenScene() const;
    QRectF SourceScene(const Settings::BottomScreenOverlay& overlay) const;
    QRectF DestinationScene(const Settings::BottomScreenOverlay& overlay) const;
    QTransform SceneToWidget() const;
    QPointF WidgetToScene(const QPointF& point) const;
    Hit HitTest(const QPointF& widget_pos) const;
    Hit HitTestOverlay(int index, const QPointF& widget_pos) const;
    void UpdateCursor(const QPointF& widget_pos);

    std::vector<Settings::BottomScreenOverlay>& overlays;
    int selected = -1;
    QImage top_image;
    QImage bottom_image;

    Hit drag;
    QPointF drag_start_scene;
    Settings::BottomScreenOverlay drag_start_overlay;
};

/// Dialog for editing the list of bottom screen overlays.
class ConfigureBottomOverlays : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureBottomOverlays(std::vector<Settings::BottomScreenOverlay> overlays,
                                     bool per_game, QWidget* parent = nullptr);
    ~ConfigureBottomOverlays() override;

    const std::vector<Settings::BottomScreenOverlay>& GetOverlays() const {
        return overlays;
    }

    /// True when, in per-game mode, the user chose to go back to the global overlays
    bool ResetToGlobalRequested() const {
        return reset_to_global;
    }

private:
    void BuildUi(bool per_game);
    void RefreshList();
    void RefreshFields();
    void OnFieldEdited();
    void AddOverlay();
    void DuplicateOverlay();
    void RemoveOverlay();
    void MatchSourceSize();
    void CaptureFrame();

    std::vector<Settings::BottomScreenOverlay> overlays;
    bool reset_to_global = false;
    bool updating_fields = false;

    BottomOverlayCanvas* canvas = nullptr;
    QListWidget* overlay_list = nullptr;
    QPushButton* duplicate_button = nullptr;
    QPushButton* remove_button = nullptr;
    QPushButton* match_size_button = nullptr;
    QPushButton* capture_button = nullptr;
    QWidget* fields_widget = nullptr;
    QSpinBox* src_x = nullptr;
    QSpinBox* src_y = nullptr;
    QSpinBox* src_w = nullptr;
    QSpinBox* src_h = nullptr;
    QSpinBox* dst_x = nullptr;
    QSpinBox* dst_y = nullptr;
    QSpinBox* dst_w = nullptr;
    QSpinBox* dst_h = nullptr;
    QSlider* opacity_slider = nullptr;
    QLabel* opacity_label = nullptr;
    QLabel* capture_status = nullptr;
};
