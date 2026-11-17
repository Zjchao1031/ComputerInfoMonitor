#pragma once

namespace monitor::fixed
{

inline constexpr char kDataPath[] = "/data.json";
inline constexpr int kConnectionTimeoutMs = 3000;
inline constexpr int kReadTimeoutMs = 8000;
inline constexpr int kDelayedDataThresholdMs = 3000;
inline constexpr int kOfflineDataThresholdMs = 10000;
inline constexpr int kMaximumHistorySamples = 300;
inline constexpr double kTemperatureWarningCelsius = 60.0;
inline constexpr double kTemperatureHighCelsius = 80.0;
inline constexpr bool kUseDarkTheme = true;

} // namespace monitor::fixed
