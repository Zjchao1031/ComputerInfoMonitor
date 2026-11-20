#pragma once

#include "model/monitor_snapshot.h"

#include <QString>

namespace monitor::theme
{

inline constexpr const char* kBackground = "#0C1424";
inline constexpr const char* kTextPrimary = "#F4F7FB";
inline constexpr const char* kTextSecondary = "#A9BED9";
inline constexpr const char* kAccentYellow = "#FFD54F";
inline constexpr const char* kAccentCyan = "#58D9FF";
inline constexpr const char* kOnline = "#37E39B";
inline constexpr const char* kDelayed = "#FFD54F";
inline constexpr const char* kOffline = "#FF5C6C";

[[nodiscard]] QString styleSheet();
[[nodiscard]] QString connectionStateText(ConnectionState state);

} // namespace monitor::theme

