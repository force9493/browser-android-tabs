// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/accessibility/apply_dark_mode.h"

#include "base/metrics/field_trial_params.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/forcedark/forcedark_switches.h"
#include "third_party/blink/renderer/core/css/properties/css_property.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/graphics/dark_mode_color_classifier.h"
#include "third_party/blink/renderer/platform/graphics/dark_mode_settings.h"

namespace blink {
namespace {

const int kAlphaThreshold = 100;
const int kBrightnessThreshold = 50;

// TODO(https://crbug.com/925949): Add detection and classification of
// background image color. Most sites with dark background images also have a
// dark background color set, so this is less of a priority than it would be
// otherwise.
bool HasLightBackground(const LayoutView& root) {
  const ComputedStyle& style = root.StyleRef();

  // If we can't easily determine the background color, default to inverting the
  // page.
  if (!style.HasBackground())
    return true;

  Color color = style.VisitedDependentColor(GetCSSPropertyBackgroundColor());
  if (color.Alpha() < kAlphaThreshold)
    return true;

  return DarkModeColorClassifier::CalculateColorBrightness(color) >
         kBrightnessThreshold;
}

DarkMode GetMode(const Settings& frame_settings) {
  switch (features::kForceDarkInversionMethodParam.Get()) {
    case ForceDarkInversionMethod::kUseBlinkSettings:
      return frame_settings.GetDarkMode();
    case ForceDarkInversionMethod::kCielabBased:
      return DarkMode::kInvertLightnessLAB;
    case ForceDarkInversionMethod::kHslBased:
      return DarkMode::kInvertLightness;
    case ForceDarkInversionMethod::kRgbBased:
      return DarkMode::kInvertBrightness;
  }
}

DarkModeImagePolicy GetImagePolicy(const Settings& frame_settings) {
  switch (features::kForceDarkImageBehaviorParam.Get()) {
    case ForceDarkImageBehavior::kUseBlinkSettings:
      return frame_settings.GetDarkModeImagePolicy();
    case ForceDarkImageBehavior::kInvertNone:
      return DarkModeImagePolicy::kFilterNone;
    case ForceDarkImageBehavior::kInvertSelectively:
      return DarkModeImagePolicy::kFilterSmart;
  }
}

int GetTextBrightnessThreshold(const Settings& frame_settings) {
  const int flag_value = base::GetFieldTrialParamByFeatureAsInt(
      features::kForceWebContentsDarkMode,
      features::kForceDarkTextLightnessThresholdParam.name, -1);
  return flag_value >= 0 ? flag_value
                         : frame_settings.GetDarkModeTextBrightnessThreshold();
}

int GetBackgroundBrightnessThreshold(const Settings& frame_settings) {
  const int flag_value = base::GetFieldTrialParamByFeatureAsInt(
      features::kForceWebContentsDarkMode,
      features::kForceDarkBackgroundLightnessThresholdParam.name, -1);
  return flag_value >= 0
             ? flag_value
             : frame_settings.GetDarkModeBackgroundBrightnessThreshold();
}

}  // namespace

DarkModeSettings BuildDarkModeSettings(const Settings& frame_settings,
                                       const LayoutView& root) {
  DarkModeSettings dark_mode_settings;

  if (!ShouldApplyDarkModeFilterToPage(frame_settings.GetDarkModePagePolicy(),
                                       root)) {
    // In theory it should be sufficient to set mode to
    // kOff (or to just return the default struct) without also setting
    // image_policy. However, this causes images to be inverted unexpectedly in
    // some cases (such as when toggling between the site's light and dark theme
    // on arstechnica.com).
    //
    // TODO(gilmanmh): Investigate unexpected image inversion behavior when
    // image_policy not set to kFilterNone.
    dark_mode_settings.mode = DarkMode::kOff;
    dark_mode_settings.image_policy = DarkModeImagePolicy::kFilterNone;
    return dark_mode_settings;
  }

  dark_mode_settings.mode = GetMode(frame_settings);
  dark_mode_settings.image_policy = GetImagePolicy(frame_settings);
  dark_mode_settings.text_brightness_threshold =
      GetTextBrightnessThreshold(frame_settings);
  dark_mode_settings.background_brightness_threshold =
      GetBackgroundBrightnessThreshold(frame_settings);

  dark_mode_settings.grayscale = frame_settings.GetDarkModeGrayscale();
  dark_mode_settings.contrast = frame_settings.GetDarkModeContrast();
  dark_mode_settings.image_grayscale_percent =
      frame_settings.GetDarkModeImageGrayscale();
  return dark_mode_settings;
}

bool ShouldApplyDarkModeFilterToPage(DarkModePagePolicy policy,
                                     const LayoutView& root) {
  if (root.StyleRef().DarkColorScheme())
    return false;

  switch (policy) {
    case DarkModePagePolicy::kFilterAll:
      return true;
    case DarkModePagePolicy::kFilterByBackground:
      return HasLightBackground(root);
  }
}

}  // namespace blink
