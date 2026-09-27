// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "common/math_util.h"

namespace Settings {

/// Native 3DS screen sizes, in pixels, as seen in landscape orientation.
constexpr float BottomScreenNativeWidth = 320.0f;
constexpr float BottomScreenNativeHeight = 240.0f;
constexpr float TopScreenNativeWidth = 400.0f;
constexpr float TopScreenNativeHeight = 240.0f;

/**
 * A rectangle cropped out of the bottom screen and drawn on top of the top screen.
 * All coordinates are in native 3DS pixels with the origin at the top-left corner, so they stay
 * valid regardless of internal resolution, window size or screen layout.
 */
struct BottomScreenOverlay {
    // Source rectangle on the bottom screen (0..320 x 0..240)
    float src_x = 0.0f;
    float src_y = 0.0f;
    float src_w = 80.0f;
    float src_h = 60.0f;
    // Destination rectangle on the top screen (0..400 x 0..240). May differ in size from the
    // source rectangle, in which case the cropped image is scaled.
    float dst_x = 0.0f;
    float dst_y = 0.0f;
    float dst_w = 80.0f;
    float dst_h = 60.0f;
    // 0.0 = invisible, 1.0 = fully opaque
    float opacity = 1.0f;

    bool operator==(const BottomScreenOverlay&) const = default;
};

/**
 * Parses the bottom_overlays setting. Format: overlays separated by ';', each one being
 * "src_x,src_y,src_w,src_h,dst_x,dst_y,dst_w,dst_h[,opacity]". Invalid entries are skipped and
 * values are clamped to the screen bounds.
 */
std::vector<BottomScreenOverlay> ParseBottomScreenOverlays(std::string_view text);

/// Inverse of ParseBottomScreenOverlays.
std::string SerializeBottomScreenOverlays(const std::vector<BottomScreenOverlay>& overlays);

/// Clamps an overlay so both rectangles lie inside their screens and have a non-zero size.
BottomScreenOverlay ClampBottomScreenOverlay(BottomScreenOverlay overlay);

/**
 * Crops a screen's texture coordinates to the overlay's source rectangle. `texcoords` are the
 * full-screen texture coordinates the renderer would normally use for the bottom screen, laid out
 * the way the renderers' DrawSingleScreen functions expect (left/right follow the native X axis,
 * bottom/top follow the native Y axis).
 */
Common::Rectangle<float> CropBottomScreenTexcoords(const Common::Rectangle<float>& texcoords,
                                                   const BottomScreenOverlay& overlay);

/// Window-space rectangle, as x/y/width/height.
struct OverlayDrawRect {
    float x;
    float y;
    float w;
    float h;
};

/**
 * Maps the overlay's destination rectangle into window space, given where the top screen is drawn
 * (top_x, top_y, top_w, top_h) and whether the layout draws the screens in landscape
 * (is_rotated == true) or upright/portrait orientation.
 */
OverlayDrawRect GetBottomScreenOverlayDrawRect(const BottomScreenOverlay& overlay, float top_x,
                                               float top_y, float top_w, float top_h,
                                               bool is_rotated);

} // namespace Settings
