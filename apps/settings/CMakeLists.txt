qt_add_library(vninput_settings_ui STATIC
    engine_text_edit.cpp engine_text_edit.hpp
    settings_store.cpp settings_store.hpp
    settings_window.cpp settings_window.hpp
    theme.cpp theme.hpp
)
target_include_directories(vninput_settings_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vninput_settings_ui
    PUBLIC VNInput::Core VNInput::Platform Qt6::Widgets
    PRIVATE vninput_warnings)
target_compile_definitions(vninput_settings_ui PUBLIC VNINPUT_VERSION="${PROJECT_VERSION}")
if(TARGET vninput_macos_support)
    target_link_libraries(vninput_settings_ui PRIVATE vninput_macos_support)
    target_compile_definitions(vninput_settings_ui PUBLIC VNINPUT_MACOS_IME=1)
endif()
qt_add_resources(vninput_settings_ui branding
    PREFIX "/vninput"
    BASE "${PROJECT_SOURCE_DIR}/resources"
    FILES "${PROJECT_SOURCE_DIR}/resources/branding/vninput-logo.png")

qt_add_executable(vninput-settings WIN32 MACOSX_BUNDLE main.cpp)
target_link_libraries(vninput-settings PRIVATE vninput_settings_ui vninput_warnings)
set_target_properties(vninput-settings PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "VNInput"
    MACOSX_BUNDLE_GUI_IDENTIFIER "vn.vninput.settings"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)
install(TARGETS vninput-settings
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(VNINPUT_DEPLOY_QT)
    if(APPLE)
        # The UI only needs Cocoa and built-in PNG support. Deploying every
        # Homebrew image/input plugin pulls unrelated PDF/QML/SVG frameworks.
        if(Qt6_VERSION VERSION_LESS 6.7)
            message(FATAL_ERROR "macOS Qt deployment requires Qt 6.7+; or use VNINPUT_DEPLOY_QT=OFF")
        endif()
        get_target_property(vninput_cocoa_plugin Qt6::QCocoaIntegrationPlugin LOCATION)
        file(REAL_PATH "${vninput_cocoa_plugin}" vninput_cocoa_plugin)
        install(FILES "${vninput_cocoa_plugin}"
            DESTINATION "vninput-settings.app/Contents/PlugIns/platforms")
        qt_generate_deploy_app_script(TARGET vninput-settings
            OUTPUT_SCRIPT vninput_deploy_script
            DEPLOY_TOOL_OPTIONS -no-plugins
                "-executable=vninput-settings.app/Contents/PlugIns/platforms/libqcocoa.dylib")
    else()
        qt_generate_deploy_app_script(TARGET vninput-settings OUTPUT_SCRIPT vninput_deploy_script)
    endif()
    install(SCRIPT ${vninput_deploy_script})
endif()
