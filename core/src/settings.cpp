#include <vninput/settings.hpp>

namespace vninput {

std::string_view toString(InputMethod value)
{
    switch (value) {
    case InputMethod::Telex: return "telex";
    case InputMethod::Vni: return "vni";
    }
    return {};
}

std::string_view toString(TonePlacement value)
{
    switch (value) {
    case TonePlacement::Modern: return "modern";
    case TonePlacement::Classic: return "classic";
    }
    return {};
}

std::optional<InputMethod> inputMethodFromString(std::string_view value)
{
    if (value == "telex") return InputMethod::Telex;
    if (value == "vni") return InputMethod::Vni;
    return std::nullopt;
}

std::optional<TonePlacement> tonePlacementFromString(std::string_view value)
{
    if (value == "modern") return TonePlacement::Modern;
    if (value == "classic") return TonePlacement::Classic;
    return std::nullopt;
}

std::optional<std::string_view> validate(const Settings& settings)
{
    if (toString(settings.inputMethod).empty()) return "invalid-input-method";
    if (toString(settings.tonePlacement).empty()) return "invalid-tone-placement";
    if (settings.toggleShortcut.empty() || settings.toggleShortcut.size() > 128)
        return "invalid-shortcut";
    return std::nullopt;
}

} // namespace vninput
