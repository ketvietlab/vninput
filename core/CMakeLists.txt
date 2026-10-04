add_library(vninput_core STATIC src/settings.cpp src/engine.cpp)
add_library(VNInput::Core ALIAS vninput_core)
target_include_directories(vninput_core PUBLIC include)
target_link_libraries(vninput_core PRIVATE vninput_warnings)
