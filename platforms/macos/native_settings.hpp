#pragma once

#include <vninput/settings.hpp>
#include <string>

namespace vninput::macos {
std::string settingsPath();
void notifySettingsChanged();
enum class SourceStatus { Missing, Available, Enabled, Selected };
SourceStatus inputSourceStatus();
std::optional<Settings> readSettingsFile(const std::string& path);
bool supportsShortcut(std::string_view text);
}

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#include <optional>

namespace vninput::macos {
inline NSString* const configNotification = @"vn.vninput.settings.changed";
inline NSString* const runtimeNotification = @"vn.vninput.runtime.changed";
std::optional<Settings> readSettings(NSString* path, NSString** error = nullptr);
struct Shortcut {
    unsigned short keyCode;
    NSEventModifierFlags modifiers;
};
std::optional<Shortcut> parseShortcut(std::string_view portableText);
bool matchesShortcut(NSEvent* event, const Shortcut& shortcut);
std::u32string unicodeText(NSString* text);
NSString* nativeText(std::u32string_view text);
}

// The UI owns the JSON file. The server only reads; V/E is a runtime override.
@interface VNInputConfiguration : NSObject
+ (instancetype)shared;
- (vninput::Settings)settings;
- (void)reload;
- (void)toggleEnabled;
@end
#endif
