#pragma once

/// @brief Starts the Wi-Fi NAT router.
///
/// Brings up the ESP's own access point and then connects to the home Wi-Fi
/// as a station. NAPT is switched on automatically once the station gets an
/// IP address, and off again when the upstream connection is lost.
///
/// @pre accessPointSettings holds valid credentials for both networks.
/// @return true if the access point is running. The upstream connection is
///         established in the background and may still be pending.
bool startNatRouter();

/// @brief Reports whether clients currently have internet access.
/// @return true while the home Wi-Fi is connected and NAPT is active.
/// @note Thread-safe: it is updated from the Wi-Fi event task.
bool isNatEnabled();