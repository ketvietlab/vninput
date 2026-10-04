#include <vninput/platform.hpp>

vninput::platform::BackendInfo vninput::platform::currentBackend()
{
#ifdef VNINPUT_HAS_MACOS_IME
    return {"macOS", "InputMethodKit", true};
#else
    return {"macOS", "InputMethodKit", false};
#endif
}
