find_library(VNINPUT_COCOA Cocoa REQUIRED)
find_library(VNINPUT_IMK InputMethodKit REQUIRED)
find_library(VNINPUT_CARBON Carbon REQUIRED)

add_library(vninput_macos_support STATIC native_settings.mm native_session.mm)
target_include_directories(vninput_macos_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(vninput_macos_support PRIVATE -fobjc-arc)
target_link_libraries(vninput_macos_support PUBLIC VNInput::Core "${VNINPUT_COCOA}" "${VNINPUT_CARBON}" PRIVATE vninput_warnings)

add_executable(vninput-input-method MACOSX_BUNDLE main.mm input_controller.mm)
target_compile_options(vninput-input-method PRIVATE -fobjc-arc)
target_link_libraries(vninput-input-method PRIVATE vninput_macos_support "${VNINPUT_IMK}" vninput_warnings)
set_target_properties(vninput-input-method PROPERTIES
    OUTPUT_NAME VNInput
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")
target_sources(vninput-input-method PRIVATE
    "${PROJECT_SOURCE_DIR}/resources/branding/vninput-logo.png"
    "${PROJECT_SOURCE_DIR}/resources/branding/vninput-input-source.pdf")
set_source_files_properties(
    "${PROJECT_SOURCE_DIR}/resources/branding/vninput-logo.png"
    "${PROJECT_SOURCE_DIR}/resources/branding/vninput-input-source.pdf"
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
install(TARGETS vninput-input-method BUNDLE DESTINATION "Input Methods" COMPONENT macos-ime)

add_executable(vninput-input-source input_source.mm)
target_compile_options(vninput-input-source PRIVATE -fobjc-arc)
target_link_libraries(vninput-input-source PRIVATE "${VNINPUT_COCOA}" "${VNINPUT_CARBON}" vninput_warnings)
install(TARGETS vninput-input-source RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT macos-ime)
