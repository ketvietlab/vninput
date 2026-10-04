#pragma once

#include <string_view>

namespace vninput::platform {

struct BackendInfo {
    std::string_view operatingSystem;
    std::string_view inputApi;
    bool implemented;
};

BackendInfo currentBackend();

} // namespace vninput::platform
