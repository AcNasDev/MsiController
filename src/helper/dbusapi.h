#pragma once

namespace MsiDbusApi {
inline constexpr int currentApiVersion = 1;

inline constexpr auto serviceName = "com.msi.ec";
inline constexpr auto parametersInterface = "com.msi.ec.Parameters";
inline constexpr auto memoryInterface = "com.msi.ec.Memory";
inline constexpr auto profilesInterface = "com.msi.ec.Profiles";
inline constexpr auto healthInterface = "com.msi.ec.Health";

inline constexpr auto parametersPath = "/Parameters";
inline constexpr auto memoryPath = "/Memory";
inline constexpr auto profilesPath = "/Profiles";
inline constexpr auto healthPath = "/Health";
} // namespace MsiDbusApi
