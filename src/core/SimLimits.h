#pragma once

// Shared simulation limits for CLI, GUI, and tests.
namespace SimLimits {

constexpr int DEFAULT_MAX_CYCLES = 200000;

// Maximum continuous simulation rate in the GUI (cycles per second).
constexpr int MAX_SIM_SPEED_CPS = 500;

// Fixed GUI refresh rate while running. Cycles per tick are accumulated as
// (cyclesPerSecond * interval / 1000) so low speeds are not rounded up to 1
// cycle per tick (~62 cps at 16 ms).
constexpr int GUI_REFRESH_INTERVAL_MS = 16;  // ~60 Hz

}  // namespace SimLimits
