#include <vninput/engine.hpp>
#include <vninput/platform.hpp>
#include <vninput/settings.hpp>
#include <iostream>

int main()
{
    int failures = 0;
    const auto check = [&](bool valid, const char* description) {
        if (!valid) { std::cerr << description << '\n'; ++failures; }
    };
    vninput::Settings settings;
    check(!vninput::validate(settings), "Default configuration must be valid");
    check(!vninput::inputMethodFromString("unknown"), "Reject unknown input method");
    check(!vninput::tonePlacementFromString("unknown"), "Reject unknown tone placement");
    for (auto method : {vninput::InputMethod::Telex, vninput::InputMethod::Vni})
        check(vninput::inputMethodFromString(vninput::toString(method)) == method, "Input method round trip");
    settings.toggleShortcut.clear();
    check(vninput::validate(settings).has_value(), "Reject empty shortcut");
    settings = {};
    settings.inputMethod = static_cast<vninput::InputMethod>(42);
    check(vninput::validate(settings).has_value(), "Reject invalid enum");
#ifdef VNINPUT_MACOS_IME
    check(vninput::platform::currentBackend().implemented, "macOS adapter is included in this build");
#else
    check(!vninput::platform::currentBackend().implemented, "Unavailable adapters must not claim implementation");
#endif
    return failures == 0 ? 0 : 1;
}
