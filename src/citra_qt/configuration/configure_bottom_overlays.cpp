// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTransform>
#include <QVBoxLayout>
#include "citra_qt/configuration/configure_bottom_overlays.h"
#include "citra_qt/util/util.h"
#include "core/3ds.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"

using Settings::BottomScreenOverlay;

namespace {

constexpr float SceneGap = 20.0f;
constexpr float SceneWidth = Settings::TopScreenNativeWidth;
constexpr float SceneHeight =
    Settings::TopScreenNativeHeight + SceneGap + Settings::BottomScreenNativeHeight;
constexpr float WidgetMargin = 10.0f;
constexpr float HandleRadius = 7.0f;
constexpr float MinRectSize = 4.0f;

QColor OverlayColor(int index) {
    static const std::array<QColor, 6> colors{
        QColor(0xff, 0x5a, 0x5a), QColor(0x4f, 0xc3, 0xf7), QColor(0x81, 0xc7, 0x84),
        QColor(0xff, 0xb7, 0x4d), QColor(0xba, 0x68, 0xc8), QColor(0xff, 0xf1, 0x76),
    };
    return colors[static_cast<std::size_t>(index) % colors.size()];
}

BottomScreenOverlay MakeDefaultOverlay() {
    // A small box from the top-left of the bottom screen, placed in the top-right of the top
    // screen, which is a common spot for maps and status panels.
    BottomScreenOverlay overlay;
    overlay.src_x = 0;
    overlay.src_y = 0;
    overlay.src_w = 100;
    overlay.src_h = 75;
    overlay.dst_w = 100;
    overlay.dst_h = 75;
    overlay.dst_x = Settings::TopScreenNativeWidth - overlay.dst_w - 4;
    overlay.dst_y = 4;
    overlay.opacity = 0.85f;
    return overlay;
}

QSpinBox* MakeSpinBox(int min, int max) {
    auto* spin = new QSpinBox;
    spin->setRange(min, max);
    spin->setAccelerated(true);
    return spin;
}

} // Anonymous namespace

BottomOverlayCanvas::BottomOverlayCanvas(std::vector<BottomScreenOverlay>& overlays_,
                                         QWidget* parent)
    : QWidget(parent), overlays{overlays_} {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize BottomOverlayCanvas::sizeHint() const {
    return QSize(static_cast<int>(SceneWidth * 1.5f + WidgetMargin * 2),
                 static_cast<int>(SceneHeight * 1.5f + WidgetMargin * 2));
}

QSize BottomOverlayCanvas::minimumSizeHint() const {
    return QSize(static_cast<int>(SceneWidth + WidgetMargin * 2),
                 static_cast<int>(SceneHeight + WidgetMargin * 2));
}

void BottomOverlayCanvas::SetSelected(int index) {
    if (index == selected) {
        return;
    }
    selected = index;
    update();
}

void BottomOverlayCanvas::SetScreenImages(const QImage& top, const QImage& bottom) {
    top_image = top;
    bottom_image = bottom;
    update();
}

QRectF BottomOverlayCanvas::TopScreenScene() const {
    return QRectF(0, 0, Settings::TopScreenNativeWidth, Settings::TopScreenNativeHeight);
}

QRectF BottomOverlayCanvas::BottomScreenScene() const {
    return QRectF((Settings::TopScreenNativeWidth - Settings::BottomScreenNativeWidth) / 2.0f,
                  Settings::TopScreenNativeHeight + SceneGap, Settings::BottomScreenNativeWidth,
                  Settings::BottomScreenNativeHeight);
}

QRectF BottomOverlayCanvas::SourceScene(const BottomScreenOverlay& overlay) const {
    const QPointF origin = BottomScreenScene().topLeft();
    return QRectF(origin.x() + overlay.src_x, origin.y() + overlay.src_y, overlay.src_w,
                  overlay.src_h);
}

QRectF BottomOverlayCanvas::DestinationScene(const BottomScreenOverlay& overlay) const {
    return QRectF(overlay.dst_x, overlay.dst_y, overlay.dst_w, overlay.dst_h);
}

QTransform BottomOverlayCanvas::SceneToWidget() const {
    const float avail_w = std::max(1.0f, static_cast<float>(width()) - WidgetMargin * 2);
    const float avail_h = std::max(1.0f, static_cast<float>(height()) - WidgetMargin * 2);
    const float scale = std::min(avail_w / SceneWidth, avail_h / SceneHeight);
    const float offset_x = (static_cast<float>(width()) - SceneWidth * scale) / 2.0f;
    const float offset_y = (static_cast<float>(height()) - SceneHeight * scale) / 2.0f;
    QTransform transform;
    transform.translate(offset_x, offset_y);
    transform.scale(scale, scale);
    return transform;
}

QPointF BottomOverlayCanvas::WidgetToScene(const QPointF& point) const {
    return SceneToWidget().inverted().map(point);
}

void BottomOverlayCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), QColor(0x1e, 0x1e, 0x1e));

    const QTransform transform = SceneToWidget();

    const auto draw_screen = [&](const QRectF& scene_rect, const QImage& image,
                                 const QString& label) {
        painter.setTransform(transform);
        if (!image.isNull()) {
            painter.drawImage(scene_rect, image);
        } else {
            painter.fillRect(scene_rect, QColor(0x3a, 0x3a, 0x3a));
            painter.resetTransform();
            painter.setPen(QColor(0x90, 0x90, 0x90));
            painter.drawText(transform.mapRect(scene_rect), Qt::AlignCenter, label);
        }
    };
    draw_screen(TopScreenScene(), top_image, tr("Top screen"));
    draw_screen(BottomScreenScene(), bottom_image, tr("Bottom screen"));

    for (int i = 0; i < static_cast<int>(overlays.size()); i++) {
        const auto& overlay = overlays[i];
        const QColor color = OverlayColor(i);
        const bool is_selected = i == selected;
        const QRectF src = SourceScene(overlay);
        const QRectF dst = DestinationScene(overlay);

        painter.setTransform(transform);

        // Destination: preview of the cropped image at the chosen opacity
        if (!bottom_image.isNull()) {
            const float sx =
                static_cast<float>(bottom_image.width()) / Settings::BottomScreenNativeWidth;
            const float sy =
                static_cast<float>(bottom_image.height()) / Settings::BottomScreenNativeHeight;
            const QRectF crop(overlay.src_x * sx, overlay.src_y * sy, overlay.src_w * sx,
                              overlay.src_h * sy);
            painter.setOpacity(overlay.opacity);
            painter.drawImage(dst, bottom_image, crop);
            painter.setOpacity(1.0);
        } else {
            QColor fill = color;
            fill.setAlphaF(0.35f * overlay.opacity + 0.05f);
            painter.fillRect(dst, fill);
        }

        QColor src_fill = color;
        src_fill.setAlphaF(0.15f);
        painter.fillRect(src, src_fill);

        QPen solid(color, is_selected ? 2.5 : 1.25);
        solid.setCosmetic(true);
        painter.setPen(solid);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(dst);

        QPen dashed = solid;
        dashed.setStyle(Qt::DashLine);
        painter.setPen(dashed);
        painter.drawRect(src);

        // Labels and handles are drawn in widget space so they stay crisp and a constant size
        painter.resetTransform();
        const QRectF src_w = transform.mapRect(src);
        const QRectF dst_w = transform.mapRect(dst);
        QFont font = painter.font();
        font.setBold(true);
        painter.setFont(font);
        const QString number = QString::number(i + 1);
        for (const QRectF& r : {src_w, dst_w}) {
            const QRectF tag(r.left(), r.top(), 16, 16);
            painter.fillRect(tag, color);
            painter.setPen(Qt::black);
            painter.drawText(tag, Qt::AlignCenter, number);
        }

        if (is_selected) {
            painter.setPen(QPen(Qt::black, 1));
            painter.setBrush(Qt::white);
            for (const QRectF& r : {src_w, dst_w}) {
                for (const QPointF& corner :
                     {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()}) {
                    painter.drawRect(QRectF(corner.x() - 4, corner.y() - 4, 8, 8));
                }
            }
            painter.setBrush(Qt::NoBrush);
        }
    }
}

BottomOverlayCanvas::Hit BottomOverlayCanvas::HitTestOverlay(int index,
                                                             const QPointF& widget_pos) const {
    const QTransform transform = SceneToWidget();
    const auto& overlay = overlays[index];
    const auto test_rect = [&](const QRectF& scene_rect, Target target) -> Hit {
        const QRectF r = transform.mapRect(scene_rect);
        const std::array<std::pair<QPointF, DragMode>, 4> corners{{
            {r.topLeft(), DragMode::ResizeTopLeft},
            {r.topRight(), DragMode::ResizeTopRight},
            {r.bottomLeft(), DragMode::ResizeBottomLeft},
            {r.bottomRight(), DragMode::ResizeBottomRight},
        }};
        for (const auto& [corner, mode] : corners) {
            if (std::abs(corner.x() - widget_pos.x()) <= HandleRadius &&
                std::abs(corner.y() - widget_pos.y()) <= HandleRadius) {
                return {index, target, mode};
            }
        }
        if (r.contains(widget_pos)) {
            return {index, target, DragMode::Move};
        }
        return {};
    };
    if (const Hit hit = test_rect(DestinationScene(overlay), Target::Destination); hit.index >= 0) {
        return hit;
    }
    return test_rect(SourceScene(overlay), Target::Source);
}

BottomOverlayCanvas::Hit BottomOverlayCanvas::HitTest(const QPointF& widget_pos) const {
    // The selected overlay wins, then the most recently added (drawn on top)
    if (selected >= 0 && selected < static_cast<int>(overlays.size())) {
        if (const Hit hit = HitTestOverlay(selected, widget_pos); hit.index >= 0) {
            return hit;
        }
    }
    for (int i = static_cast<int>(overlays.size()) - 1; i >= 0; i--) {
        if (const Hit hit = HitTestOverlay(i, widget_pos); hit.index >= 0) {
            return hit;
        }
    }
    return {};
}

void BottomOverlayCanvas::UpdateCursor(const QPointF& widget_pos) {
    const DragMode mode = drag.index >= 0 ? drag.mode : HitTest(widget_pos).mode;
    switch (mode) {
    case DragMode::Move:
        setCursor(drag.index >= 0 ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
        break;
    case DragMode::ResizeTopLeft:
    case DragMode::ResizeBottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case DragMode::ResizeTopRight:
    case DragMode::ResizeBottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        unsetCursor();
        break;
    }
}

void BottomOverlayCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QPointF pos = event->position();
    const Hit hit = HitTest(pos);
    if (hit.index < 0) {
        return;
    }
    if (hit.index != selected) {
        selected = hit.index;
        emit SelectionChanged(selected);
    }
    drag = hit;
    drag_start_scene = WidgetToScene(pos);
    drag_start_overlay = overlays[hit.index];
    UpdateCursor(pos);
    update();
}

void BottomOverlayCanvas::mouseMoveEvent(QMouseEvent* event) {
    const QPointF pos = event->position();
    if (drag.index < 0 || drag.index >= static_cast<int>(overlays.size())) {
        UpdateCursor(pos);
        return;
    }

    const QPointF delta = WidgetToScene(pos) - drag_start_scene;
    const bool is_dst = drag.target == Target::Destination;
    const auto& start = drag_start_overlay;
    auto& overlay = overlays[drag.index];

    const float bound_w =
        is_dst ? Settings::TopScreenNativeWidth : Settings::BottomScreenNativeWidth;
    const float bound_h =
        is_dst ? Settings::TopScreenNativeHeight : Settings::BottomScreenNativeHeight;
    float x = is_dst ? start.dst_x : start.src_x;
    float y = is_dst ? start.dst_y : start.src_y;
    float w = is_dst ? start.dst_w : start.src_w;
    float h = is_dst ? start.dst_h : start.src_h;
    const float dx = static_cast<float>(delta.x());
    const float dy = static_cast<float>(delta.y());

    if (drag.mode == DragMode::Move) {
        x = std::clamp(std::round(x + dx), 0.0f, bound_w - w);
        y = std::clamp(std::round(y + dy), 0.0f, bound_h - h);
    } else {
        // Resize by moving one corner while the opposite corner stays anchored
        const bool moving_left =
            drag.mode == DragMode::ResizeTopLeft || drag.mode == DragMode::ResizeBottomLeft;
        const bool moving_top =
            drag.mode == DragMode::ResizeTopLeft || drag.mode == DragMode::ResizeTopRight;
        const float anchor_x = moving_left ? x + w : x;
        const float anchor_y = moving_top ? y + h : y;
        float corner_x = (moving_left ? x : x + w) + dx;
        float corner_y = (moving_top ? y : y + h) + dy;
        corner_x = moving_left ? std::clamp(corner_x, 0.0f, anchor_x - MinRectSize)
                               : std::clamp(corner_x, anchor_x + MinRectSize, bound_w);
        corner_y = moving_top ? std::clamp(corner_y, 0.0f, anchor_y - MinRectSize)
                              : std::clamp(corner_y, anchor_y + MinRectSize, bound_h);
        w = std::abs(corner_x - anchor_x);
        h = std::abs(corner_y - anchor_y);

        // Shift keeps the destination at the source's aspect ratio
        if (is_dst && (event->modifiers() & Qt::ShiftModifier) && start.src_h > 0) {
            const float aspect = start.src_w / start.src_h;
            if (w / h > aspect) {
                w = std::max(MinRectSize, h * aspect);
            } else {
                h = std::max(MinRectSize, w / aspect);
            }
        }
        w = std::round(w);
        h = std::round(h);
        x = moving_left ? anchor_x - w : anchor_x;
        y = moving_top ? anchor_y - h : anchor_y;
    }

    if (is_dst) {
        overlay.dst_x = x;
        overlay.dst_y = y;
        overlay.dst_w = w;
        overlay.dst_h = h;
    } else {
        overlay.src_x = x;
        overlay.src_y = y;
        overlay.src_w = w;
        overlay.src_h = h;
    }
    overlay = Settings::ClampBottomScreenOverlay(overlay);
    emit OverlayEdited(drag.index);
    update();
}

void BottomOverlayCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        drag = {};
        UpdateCursor(event->position());
    }
}

ConfigureBottomOverlays::ConfigureBottomOverlays(std::vector<BottomScreenOverlay> overlays_,
                                                 bool per_game, QWidget* parent)
    : QDialog(parent), overlays{std::move(overlays_)} {
    setWindowTitle(tr("Bottom Screen Overlays"));
    BuildUi(per_game);
    RefreshList();
    if (!overlays.empty()) {
        overlay_list->setCurrentRow(0);
    }
    RefreshFields();
}

ConfigureBottomOverlays::~ConfigureBottomOverlays() = default;

void ConfigureBottomOverlays::BuildUi(bool per_game) {
    canvas = new BottomOverlayCanvas(overlays, this);

    overlay_list = new QListWidget;
    overlay_list->setMaximumHeight(110);
    auto* add_button = new QPushButton(tr("Add"));
    duplicate_button = new QPushButton(tr("Duplicate"));
    remove_button = new QPushButton(tr("Remove"));
    auto* list_buttons = new QHBoxLayout;
    list_buttons->addWidget(add_button);
    list_buttons->addWidget(duplicate_button);
    list_buttons->addWidget(remove_button);

    const int bottom_w = static_cast<int>(Settings::BottomScreenNativeWidth);
    const int bottom_h = static_cast<int>(Settings::BottomScreenNativeHeight);
    const int top_w = static_cast<int>(Settings::TopScreenNativeWidth);
    const int top_h = static_cast<int>(Settings::TopScreenNativeHeight);
    src_x = MakeSpinBox(0, bottom_w - 1);
    src_y = MakeSpinBox(0, bottom_h - 1);
    src_w = MakeSpinBox(1, bottom_w);
    src_h = MakeSpinBox(1, bottom_h);
    dst_x = MakeSpinBox(0, top_w - 1);
    dst_y = MakeSpinBox(0, top_h - 1);
    dst_w = MakeSpinBox(1, top_w);
    dst_h = MakeSpinBox(1, top_h);

    const auto make_rect_group = [](const QString& title, QSpinBox* x, QSpinBox* y, QSpinBox* w,
                                    QSpinBox* h) {
        auto* group = new QGroupBox(title);
        auto* grid = new QGridLayout(group);
        grid->addWidget(new QLabel(tr("X")), 0, 0);
        grid->addWidget(x, 0, 1);
        grid->addWidget(new QLabel(tr("Y")), 0, 2);
        grid->addWidget(y, 0, 3);
        grid->addWidget(new QLabel(tr("Width")), 1, 0);
        grid->addWidget(w, 1, 1);
        grid->addWidget(new QLabel(tr("Height")), 1, 2);
        grid->addWidget(h, 1, 3);
        return group;
    };

    match_size_button = new QPushButton(tr("Match Source Size"));
    match_size_button->setToolTip(tr("Show the cropped area at its original size (1:1)."));

    opacity_slider = new QSlider(Qt::Horizontal);
    opacity_slider->setRange(0, 100);
    opacity_label = new QLabel;
    opacity_label->setMinimumWidth(40);
    auto* opacity_row = new QHBoxLayout;
    opacity_row->addWidget(new QLabel(tr("Opacity")));
    opacity_row->addWidget(opacity_slider);
    opacity_row->addWidget(opacity_label);

    fields_widget = new QWidget;
    auto* fields_layout = new QVBoxLayout(fields_widget);
    fields_layout->setContentsMargins(0, 0, 0, 0);
    fields_layout->addWidget(
        make_rect_group(tr("Source (bottom screen, 320x240)"), src_x, src_y, src_w, src_h));
    fields_layout->addWidget(
        make_rect_group(tr("Destination (top screen, 400x240)"), dst_x, dst_y, dst_w, dst_h));
    fields_layout->addWidget(match_size_button);
    fields_layout->addLayout(opacity_row);

    capture_button = new QPushButton(tr("Capture Current Frame"));
    capture_button->setToolTip(
        tr("Grab the game's current screens so you can see what you are cropping."));
    capture_status = new QLabel;
    capture_status->setWordWrap(true);
    const bool running = Core::System::GetInstance().IsPoweredOn();
    capture_button->setEnabled(running);
    if (!running) {
        capture_status->setText(tr("Start a game to capture its screens as a guide."));
    }

    auto* side = new QVBoxLayout;
    side->addWidget(new QLabel(tr("Overlays")));
    side->addWidget(overlay_list);
    side->addLayout(list_buttons);
    side->addWidget(fields_widget);
    side->addWidget(capture_button);
    side->addWidget(capture_status);
    side->addStretch();

    auto* side_widget = new QWidget;
    side_widget->setLayout(side);
    side_widget->setFixedWidth(300);

    auto* main_row = new QHBoxLayout;
    main_row->addWidget(canvas, 1);
    main_row->addWidget(side_widget);

    auto* help = new QLabel(
        tr("Drag the dashed box on the bottom screen to choose what to copy, and the solid box on "
           "the top screen to choose where it appears. Drag a corner to resize; hold Shift while "
           "resizing a destination to keep the source's proportions. Coordinates are in native "
           "3DS pixels, so overlays keep their place at any resolution or window size."));
    help->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    if (per_game) {
        auto* global_button =
            buttons->addButton(tr("Use Global Overlays"), QDialogButtonBox::ResetRole);
        global_button->setToolTip(
            tr("Discard this game's overlays and use the ones from the global configuration."));
        connect(global_button, &QPushButton::clicked, this, [this] {
            reset_to_global = true;
            accept();
        });
    }

    auto* root = new QVBoxLayout(this);
    root->addLayout(main_row, 1);
    root->addWidget(help);
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(add_button, &QPushButton::clicked, this, &ConfigureBottomOverlays::AddOverlay);
    connect(duplicate_button, &QPushButton::clicked, this,
            &ConfigureBottomOverlays::DuplicateOverlay);
    connect(remove_button, &QPushButton::clicked, this, &ConfigureBottomOverlays::RemoveOverlay);
    connect(match_size_button, &QPushButton::clicked, this,
            &ConfigureBottomOverlays::MatchSourceSize);
    connect(capture_button, &QPushButton::clicked, this, &ConfigureBottomOverlays::CaptureFrame);
    connect(overlay_list, &QListWidget::currentRowChanged, this, [this](int row) {
        canvas->SetSelected(row);
        RefreshFields();
    });
    connect(canvas, &BottomOverlayCanvas::SelectionChanged, this,
            [this](int index) { overlay_list->setCurrentRow(index); });
    connect(canvas, &BottomOverlayCanvas::OverlayEdited, this, [this](int) { RefreshFields(); });

    for (QSpinBox* spin : {src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h}) {
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this,
                &ConfigureBottomOverlays::OnFieldEdited);
    }
    connect(opacity_slider, &QSlider::valueChanged, this, &ConfigureBottomOverlays::OnFieldEdited);

    resize(980, 640);
}

void ConfigureBottomOverlays::RefreshList() {
    const int row = overlay_list->currentRow();
    overlay_list->blockSignals(true);
    overlay_list->clear();
    for (std::size_t i = 0; i < overlays.size(); i++) {
        auto* item = new QListWidgetItem(tr("Overlay %1").arg(static_cast<int>(i) + 1));
        QPixmap swatch(12, 12);
        swatch.fill(OverlayColor(static_cast<int>(i)));
        item->setIcon(QIcon(swatch));
        overlay_list->addItem(item);
    }
    overlay_list->setCurrentRow(std::min(row, static_cast<int>(overlays.size()) - 1));
    overlay_list->blockSignals(false);
    canvas->SetSelected(overlay_list->currentRow());
}

void ConfigureBottomOverlays::RefreshFields() {
    const int index = overlay_list->currentRow();
    const bool has_selection = index >= 0 && index < static_cast<int>(overlays.size());
    fields_widget->setEnabled(has_selection);
    duplicate_button->setEnabled(has_selection);
    remove_button->setEnabled(has_selection);
    if (!has_selection) {
        opacity_label->clear();
        return;
    }

    const auto& o = overlays[index];
    updating_fields = true;
    src_x->setValue(static_cast<int>(std::lround(o.src_x)));
    src_y->setValue(static_cast<int>(std::lround(o.src_y)));
    src_w->setValue(static_cast<int>(std::lround(o.src_w)));
    src_h->setValue(static_cast<int>(std::lround(o.src_h)));
    dst_x->setValue(static_cast<int>(std::lround(o.dst_x)));
    dst_y->setValue(static_cast<int>(std::lround(o.dst_y)));
    dst_w->setValue(static_cast<int>(std::lround(o.dst_w)));
    dst_h->setValue(static_cast<int>(std::lround(o.dst_h)));
    opacity_slider->setValue(static_cast<int>(std::lround(o.opacity * 100.0f)));
    opacity_label->setText(QStringLiteral("%1%").arg(opacity_slider->value()));
    updating_fields = false;
}

void ConfigureBottomOverlays::OnFieldEdited() {
    const int index = overlay_list->currentRow();
    if (updating_fields || index < 0 || index >= static_cast<int>(overlays.size())) {
        return;
    }
    auto& o = overlays[index];
    o.src_x = static_cast<float>(src_x->value());
    o.src_y = static_cast<float>(src_y->value());
    o.src_w = static_cast<float>(src_w->value());
    o.src_h = static_cast<float>(src_h->value());
    o.dst_x = static_cast<float>(dst_x->value());
    o.dst_y = static_cast<float>(dst_y->value());
    o.dst_w = static_cast<float>(dst_w->value());
    o.dst_h = static_cast<float>(dst_h->value());
    o.opacity = static_cast<float>(opacity_slider->value()) / 100.0f;
    o = Settings::ClampBottomScreenOverlay(o);
    RefreshFields(); // Shows any clamping that happened
    canvas->update();
}

void ConfigureBottomOverlays::AddOverlay() {
    overlays.push_back(MakeDefaultOverlay());
    RefreshList();
    overlay_list->setCurrentRow(static_cast<int>(overlays.size()) - 1);
}

void ConfigureBottomOverlays::DuplicateOverlay() {
    const int index = overlay_list->currentRow();
    if (index < 0 || index >= static_cast<int>(overlays.size())) {
        return;
    }
    BottomScreenOverlay copy = overlays[index];
    copy.dst_x += 10;
    copy.dst_y += 10;
    overlays.push_back(Settings::ClampBottomScreenOverlay(copy));
    RefreshList();
    overlay_list->setCurrentRow(static_cast<int>(overlays.size()) - 1);
}

void ConfigureBottomOverlays::RemoveOverlay() {
    const int index = overlay_list->currentRow();
    if (index < 0 || index >= static_cast<int>(overlays.size())) {
        return;
    }
    overlays.erase(overlays.begin() + index);
    RefreshList();
    overlay_list->setCurrentRow(std::min(index, static_cast<int>(overlays.size()) - 1));
    RefreshFields();
}

void ConfigureBottomOverlays::MatchSourceSize() {
    const int index = overlay_list->currentRow();
    if (index < 0 || index >= static_cast<int>(overlays.size())) {
        return;
    }
    auto& o = overlays[index];
    o.dst_w = o.src_w;
    o.dst_h = o.src_h;
    o = Settings::ClampBottomScreenOverlay(o);
    RefreshFields();
    canvas->update();
}

void ConfigureBottomOverlays::CaptureFrame() {
    auto& system = Core::System::GetInstance();
    if (!system.IsPoweredOn()) {
        return;
    }
    auto& renderer = system.GPU().Renderer();
    if (renderer.IsScreenshotPending()) {
        capture_status->setText(tr("A screenshot is already in progress, try again."));
        return;
    }

    // Render both screens at native resolution, one above the other, without 3D or overlays
    Layout::FramebufferLayout layout = Layout::DefaultFrameLayout(
        Core::kScreenTopWidth, Core::kScreenTopHeight + Core::kScreenBottomHeight, false, false);
    layout.render_3d_mode = Settings::StereoRenderOption::Off;
    layout.draw_bottom_overlays = false;

    // The buffer is shared with the callback so it outlives this dialog if it closes early
    auto image = std::make_shared<QImage>(
        QSize(static_cast<int>(layout.width), static_cast<int>(layout.height)),
        QImage::Format_RGB32);
    QPointer<ConfigureBottomOverlays> self(this);
    capture_button->setEnabled(false);
    capture_status->setText(tr("Capturing..."));

    renderer.RequestScreenshot(
        image->bits(),
        [self, image, layout](bool invert_y) {
            // Runs on the render thread; hand the result back to the UI thread
            QMetaObject::invokeMethod(
                qApp,
                [self, image, layout, invert_y] {
                    if (!self) {
                        return;
                    }
                    const QImage frame = GetMirroredImage(*image, false, invert_y);
                    const auto to_qrect = [](const Common::Rectangle<u32>& r) {
                        return QRect(static_cast<int>(r.left), static_cast<int>(r.top),
                                     static_cast<int>(r.GetWidth()),
                                     static_cast<int>(r.GetHeight()));
                    };
                    self->canvas->SetScreenImages(frame.copy(to_qrect(layout.top_screen)),
                                                  frame.copy(to_qrect(layout.bottom_screen)));
                    self->capture_button->setEnabled(true);
                    self->capture_status->setText(tr("Captured."));
                },
                Qt::QueuedConnection);
        },
        layout);
}
