// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cmath>
#include <locale>
#include <sstream>
#include <fmt/format.h>
#include "common/bottom_overlay.h"

namespace Settings {

namespace {

constexpr float MinOverlaySize = 1.0f;

std::vector<std::string_view> Split(std::string_view text, char delimiter) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(delimiter, start);
        if (end == std::string_view::npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

// Locale-independent float parsing (the Qt frontend may change the C locale).
bool ParseFloat(std::string_view text, float& out) {
    std::istringstream stream{std::string{text}};
    stream.imbue(std::locale::classic());
    float value{};
    stream >> std::ws >> value;
    if (stream.fail()) {
        return false;
    }
    stream >> std::ws;
    if (!stream.eof() || !std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

void ClampRect(float& x, float& y, float& w, float& h, float max_w, float max_h) {
    x = std::clamp(x, 0.0f, max_w - MinOverlaySize);
    y = std::clamp(y, 0.0f, max_h - MinOverlaySize);
    w = std::clamp(w, MinOverlaySize, max_w - x);
    h = std::clamp(h, MinOverlaySize, max_h - y);
}

} // Anonymous namespace

BottomScreenOverlay ClampBottomScreenOverlay(BottomScreenOverlay overlay) {
    ClampRect(overlay.src_x, overlay.src_y, overlay.src_w, overlay.src_h, BottomScreenNativeWidth,
              BottomScreenNativeHeight);
    ClampRect(overlay.dst_x, overlay.dst_y, overlay.dst_w, overlay.dst_h, TopScreenNativeWidth,
              TopScreenNativeHeight);
    overlay.opacity = std::clamp(overlay.opacity, 0.0f, 1.0f);
    return overlay;
}

std::vector<BottomScreenOverlay> ParseBottomScreenOverlays(std::string_view text) {
    std::vector<BottomScreenOverlay> overlays;
    for (const auto entry : Split(text, ';')) {
        if (entry.find_first_not_of(" \t\r\n") == std::string_view::npos) {
            continue;
        }
        const auto fields = Split(entry, ',');
        if (fields.size() != 8 && fields.size() != 9) {
            continue;
        }
        std::array<float, 9> values{0, 0, 0, 0, 0, 0, 0, 0, 1.0f};
        bool valid = true;
        for (std::size_t i = 0; i < fields.size(); i++) {
            if (!ParseFloat(fields[i], values[i])) {
                valid = false;
                break;
            }
        }
        if (!valid || values[2] <= 0.0f || values[3] <= 0.0f || values[6] <= 0.0f ||
            values[7] <= 0.0f) {
            continue;
        }
        overlays.push_back(ClampBottomScreenOverlay({
            .src_x = values[0],
            .src_y = values[1],
            .src_w = values[2],
            .src_h = values[3],
            .dst_x = values[4],
            .dst_y = values[5],
            .dst_w = values[6],
            .dst_h = values[7],
            .opacity = values[8],
        }));
    }
    return overlays;
}

std::string SerializeBottomScreenOverlays(const std::vector<BottomScreenOverlay>& overlays) {
    std::string result;
    for (const auto& o : overlays) {
        if (!result.empty()) {
            result += ';';
        }
        result += fmt::format("{:g},{:g},{:g},{:g},{:g},{:g},{:g},{:g},{:g}", o.src_x, o.src_y,
                              o.src_w, o.src_h, o.dst_x, o.dst_y, o.dst_w, o.dst_h, o.opacity);
    }
    return result;
}

Common::Rectangle<float> CropBottomScreenTexcoords(const Common::Rectangle<float>& texcoords,
                                                   const BottomScreenOverlay& overlay) {
    // In landscape, DrawSingleScreen maps the window's X axis to texcoords.left -> right and the
    // window's Y axis to texcoords.bottom -> top, so interpolate along those axes.
    const float x_span = texcoords.right - texcoords.left;
    const float y_span = texcoords.top - texcoords.bottom;
    const float x0 = overlay.src_x / BottomScreenNativeWidth;
    const float x1 = (overlay.src_x + overlay.src_w) / BottomScreenNativeWidth;
    const float y0 = overlay.src_y / BottomScreenNativeHeight;
    const float y1 = (overlay.src_y + overlay.src_h) / BottomScreenNativeHeight;

    Common::Rectangle<float> cropped;
    cropped.left = texcoords.left + x0 * x_span;
    cropped.right = texcoords.left + x1 * x_span;
    cropped.bottom = texcoords.bottom + y0 * y_span;
    cropped.top = texcoords.bottom + y1 * y_span;
    return cropped;
}

OverlayDrawRect GetBottomScreenOverlayDrawRect(const BottomScreenOverlay& overlay, float top_x,
                                               float top_y, float top_w, float top_h,
                                               bool is_rotated) {
    if (is_rotated) {
        // Landscape: window axes match the native axes.
        const float scale_x = top_w / TopScreenNativeWidth;
        const float scale_y = top_h / TopScreenNativeHeight;
        return {
            .x = top_x + overlay.dst_x * scale_x,
            .y = top_y + overlay.dst_y * scale_y,
            .w = overlay.dst_w * scale_x,
            .h = overlay.dst_h * scale_y,
        };
    }

    // Upright/portrait: the screen is rotated so that native Y runs left -> right across the
    // window and native X runs bottom -> top.
    const float scale_x = top_w / TopScreenNativeHeight;
    const float scale_y = top_h / TopScreenNativeWidth;
    return {
        .x = top_x + overlay.dst_y * scale_x,
        .y = top_y + (TopScreenNativeWidth - overlay.dst_x - overlay.dst_w) * scale_y,
        .w = overlay.dst_h * scale_x,
        .h = overlay.dst_w * scale_y,
    };
}

} // namespace Settings
