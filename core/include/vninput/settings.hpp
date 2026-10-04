#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace vninput {

enum class InputMethod { Telex, Vni };
enum class TonePlacement { Modern, Classic };

// Shared data only. Persistence and OS hotkey registration belong to the host.
struct Settings {
    InputMethod inputMethod = InputMethod::Telex;
    TonePlacement tonePlacement = TonePlacement::Modern;
    bool enabled = true;
    bool freeTone = true;
    bool restoreOnEscape = true;
    std::string toggleShortcut = "Alt+Z";

    bool operator==(const Settings&) const = default;
};

std::string_view toString(InputMethod value);
std::string_view toString(TonePlacement value);
std::optional<InputMethod> inputMethodFromString(std::string_view value);
std::optional<TonePlacement> tonePlacementFromString(std::string_view value);
// Returns a stable error key; nullopt means the data is valid.
std::optional<std::string_view> validate(const Settings& settings);

} // namespace vninput
