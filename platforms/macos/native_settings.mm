#include "native_settings.hpp"
#import <Carbon/Carbon.h>
#include <array>

namespace vninput::macos {
std::string settingsPath()
{
    return [[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/VNInput/VNInput/settings.json"] UTF8String];
}
void notifySettingsChanged()
{
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:configNotification object:nil
        userInfo:nil deliverImmediately:YES];
}
SourceStatus inputSourceStatus()
{
    NSDictionary* filter = @{(__bridge NSString*)kTISPropertyBundleID: @"vn.vninput.inputmethod.VNInput"};
    CFArrayRef sources = TISCreateInputSourceList((__bridge CFDictionaryRef)filter, true);
    SourceStatus result = SourceStatus::Missing;
    for (CFIndex i = 0; sources && i < CFArrayGetCount(sources); ++i) {
        auto source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, i);
        if ([(__bridge NSNumber*)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsSelected) boolValue]) { result = SourceStatus::Selected; break; }
        if ([(__bridge NSNumber*)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsEnabled) boolValue]) result = SourceStatus::Enabled;
        else if (result == SourceStatus::Missing) result = SourceStatus::Available;
    }
    if (sources) CFRelease(sources);
    return result;
}
std::optional<Settings> readSettingsFile(const std::string& path)
{
    return readSettings([NSString stringWithUTF8String:path.c_str()]);
}
std::u32string unicodeText(NSString* text)
{
    std::u32string result;
    for (NSUInteger i = 0; i < text.length; ++i) {
        char32_t c = [text characterAtIndex:i];
        if (c >= 0xd800 && c <= 0xdbff && i + 1 < text.length) {
            const char32_t low = [text characterAtIndex:i + 1];
            if (low >= 0xdc00 && low <= 0xdfff) { c = 0x10000 + ((c - 0xd800) << 10) + low - 0xdc00; ++i; }
        }
        if (c >= 0xd800 && c <= 0xdfff) c = 0xfffd;
        result += c;
    }
    return result;
}
NSString* nativeText(std::u32string_view text)
{
    std::u16string utf16;
    for (auto c : text) {
        if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) c = 0xfffd;
        if (c <= 0xffff) utf16 += static_cast<char16_t>(c);
        else { c -= 0x10000; utf16 += static_cast<char16_t>(0xd800 + (c >> 10)); utf16 += static_cast<char16_t>(0xdc00 + (c & 0x3ff)); }
    }
    return [[NSString alloc] initWithCharacters:reinterpret_cast<const unichar*>(utf16.data()) length:utf16.size()];
}
std::optional<Shortcut> parseShortcut(std::string_view text)
{
    NSEventModifierFlags flags = 0;
    for (;;) {
        NSEventModifierFlags flag = 0;
        // Qt on macOS: PortableText Ctrl is Command; Meta is physical Control.
        if (text.starts_with("Ctrl+")) { flag = NSEventModifierFlagCommand; text.remove_prefix(5); }
        else if (text.starts_with("Meta+")) { flag = NSEventModifierFlagControl; text.remove_prefix(5); }
        else if (text.starts_with("Alt+")) { flag = NSEventModifierFlagOption; text.remove_prefix(4); }
        else if (text.starts_with("Shift+")) { flag = NSEventModifierFlagShift; text.remove_prefix(6); }
        else break;
        if (flags & flag) return std::nullopt;
        flags |= flag;
    }
    if (!(flags & (NSEventModifierFlagCommand | NSEventModifierFlagControl | NSEventModifierFlagOption))) return std::nullopt;
    struct KeyCode { std::string_view name; unsigned short code; };
    static constexpr std::array keys{
        KeyCode{"A", kVK_ANSI_A}, KeyCode{"B", kVK_ANSI_B}, KeyCode{"C", kVK_ANSI_C}, KeyCode{"D", kVK_ANSI_D},
        KeyCode{"E", kVK_ANSI_E}, KeyCode{"F", kVK_ANSI_F}, KeyCode{"G", kVK_ANSI_G}, KeyCode{"H", kVK_ANSI_H},
        KeyCode{"I", kVK_ANSI_I}, KeyCode{"J", kVK_ANSI_J}, KeyCode{"K", kVK_ANSI_K}, KeyCode{"L", kVK_ANSI_L},
        KeyCode{"M", kVK_ANSI_M}, KeyCode{"N", kVK_ANSI_N}, KeyCode{"O", kVK_ANSI_O}, KeyCode{"P", kVK_ANSI_P},
        KeyCode{"Q", kVK_ANSI_Q}, KeyCode{"R", kVK_ANSI_R}, KeyCode{"S", kVK_ANSI_S}, KeyCode{"T", kVK_ANSI_T},
        KeyCode{"U", kVK_ANSI_U}, KeyCode{"V", kVK_ANSI_V}, KeyCode{"W", kVK_ANSI_W}, KeyCode{"X", kVK_ANSI_X},
        KeyCode{"Y", kVK_ANSI_Y}, KeyCode{"Z", kVK_ANSI_Z}, KeyCode{"0", kVK_ANSI_0}, KeyCode{"1", kVK_ANSI_1},
        KeyCode{"2", kVK_ANSI_2}, KeyCode{"3", kVK_ANSI_3}, KeyCode{"4", kVK_ANSI_4}, KeyCode{"5", kVK_ANSI_5},
        KeyCode{"6", kVK_ANSI_6}, KeyCode{"7", kVK_ANSI_7}, KeyCode{"8", kVK_ANSI_8}, KeyCode{"9", kVK_ANSI_9},
        KeyCode{"Space", kVK_Space}, KeyCode{"Tab", kVK_Tab}, KeyCode{"Return", kVK_Return}, KeyCode{"Enter", kVK_ANSI_KeypadEnter},
        KeyCode{"Esc", kVK_Escape}, KeyCode{"Backspace", kVK_Delete}, KeyCode{"Del", kVK_ForwardDelete},
        KeyCode{"Left", kVK_LeftArrow}, KeyCode{"Right", kVK_RightArrow}, KeyCode{"Up", kVK_UpArrow}, KeyCode{"Down", kVK_DownArrow},
        KeyCode{"Home", kVK_Home}, KeyCode{"End", kVK_End}, KeyCode{"PgUp", kVK_PageUp}, KeyCode{"PgDown", kVK_PageDown},
        KeyCode{"-", kVK_ANSI_Minus}, KeyCode{"=", kVK_ANSI_Equal}, KeyCode{"[", kVK_ANSI_LeftBracket}, KeyCode{"]", kVK_ANSI_RightBracket},
        KeyCode{";", kVK_ANSI_Semicolon}, KeyCode{"'", kVK_ANSI_Quote}, KeyCode{",", kVK_ANSI_Comma}, KeyCode{".", kVK_ANSI_Period},
        KeyCode{"/", kVK_ANSI_Slash}, KeyCode{"\\", kVK_ANSI_Backslash}, KeyCode{"`", kVK_ANSI_Grave},
        KeyCode{"F1", kVK_F1}, KeyCode{"F2", kVK_F2}, KeyCode{"F3", kVK_F3}, KeyCode{"F4", kVK_F4}, KeyCode{"F5", kVK_F5},
        KeyCode{"F6", kVK_F6}, KeyCode{"F7", kVK_F7}, KeyCode{"F8", kVK_F8}, KeyCode{"F9", kVK_F9}, KeyCode{"F10", kVK_F10},
        KeyCode{"F11", kVK_F11}, KeyCode{"F12", kVK_F12}, KeyCode{"F13", kVK_F13}, KeyCode{"F14", kVK_F14}, KeyCode{"F15", kVK_F15},
        KeyCode{"F16", kVK_F16}, KeyCode{"F17", kVK_F17}, KeyCode{"F18", kVK_F18}, KeyCode{"F19", kVK_F19}, KeyCode{"F20", kVK_F20}
    };
    for (const auto& key : keys) if (text == key.name) return Shortcut{key.code, flags};
    if (text == "+") return Shortcut{kVK_ANSI_Equal, flags | NSEventModifierFlagShift};
    return std::nullopt;
}
bool matchesShortcut(NSEvent* event, const Shortcut& shortcut)
{
    constexpr auto mask = NSEventModifierFlagShift | NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagCommand;
    return event.type == NSEventTypeKeyDown && event.keyCode == shortcut.keyCode
        && (event.modifierFlags & mask) == shortcut.modifiers;
}
bool supportsShortcut(std::string_view text) { return parseShortcut(text).has_value(); }
std::optional<Settings> readSettings(NSString* path, NSString** error)
{
    const auto fail = [&](NSString* reason) -> std::optional<Settings> { if (error) *error = reason; return std::nullopt; };
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) return Settings{};
    // Bound reads even if the file changes between stat and open.
    NSFileHandle* file = [NSFileHandle fileHandleForReadingAtPath:path];
    if (!file) return fail(@"Cannot read VNInput configuration");
    NSError* readError = nil;
    NSData* bytes = [file readDataUpToLength:65537 error:&readError];
    [file closeFile];
    if (!bytes || readError || bytes.length > 65536) return fail(@"Invalid configuration size/read");
    id object = [NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil];
    if (![object isKindOfClass:[NSDictionary class]]) return fail(@"Invalid configuration JSON");
    const auto isBoolean = [](id value) { return value && CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID(); };
    id version = object[@"schemaVersion"];
    if (![version isKindOfClass:[NSNumber class]] || isBoolean(version) || [version doubleValue] != 1.0)
        return fail(@"Unsupported configuration version");
    for (NSString* key in @[@"inputMethod", @"tonePlacement", @"toggleShortcut"])
        if (![object[key] isKindOfClass:[NSString class]]) return fail(@"Invalid configuration string");
    for (NSString* key in @[@"enabled", @"freeTone", @"restoreOnEscape"])
        if (!isBoolean(object[key])) return fail(@"Invalid configuration boolean");
    const auto method = inputMethodFromString([object[@"inputMethod"] UTF8String]);
    const auto tone = tonePlacementFromString([object[@"tonePlacement"] UTF8String]);
    if (!method || !tone) return fail(@"Invalid input method/tone placement");
    Settings settings;
    settings.inputMethod = *method; settings.tonePlacement = *tone;
    settings.enabled = [object[@"enabled"] boolValue]; settings.freeTone = [object[@"freeTone"] boolValue];
    settings.restoreOnEscape = [object[@"restoreOnEscape"] boolValue];
    settings.toggleShortcut = [object[@"toggleShortcut"] UTF8String];
    if (validate(settings) || !parseShortcut(settings.toggleShortcut)) return fail(@"Unsupported shortcut");
    return settings;
}
}

@implementation VNInputConfiguration {
    vninput::Settings _saved;
    std::optional<bool> _enabledOverride;
}
+ (instancetype)shared
{
    static VNInputConfiguration* instance;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ instance = [VNInputConfiguration new]; });
    return instance;
}
- (instancetype)init
{
    if ((self = [super init])) {
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(configurationChanged:)
            name:vninput::macos::configNotification object:nil suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
        [self reload];
    }
    return self;
}
- (vninput::Settings)settings
{
    auto settings = _saved;
    if (_enabledOverride) settings.enabled = *_enabledOverride;
    return settings;
}
- (void)reload
{
    [self reloadResetOverride:NO];
}
- (void)reloadResetOverride:(BOOL)resetOverride
{
    const auto settings = vninput::macos::readSettings([NSString stringWithUTF8String:vninput::macos::settingsPath().c_str()]);
    // A partial/corrupt file cannot turn off a working input session.
    if (settings && (*settings != _saved || (resetOverride && _enabledOverride))) {
        _saved = *settings; _enabledOverride.reset(); [self changed];
    }
}
- (void)configurationChanged:(NSNotification*)notification
{
    (void)notification;
    // Distributed notifications can arrive off-main-thread; clients are main-thread only.
    dispatch_async(dispatch_get_main_queue(), ^{ [self reloadResetOverride:YES]; });
}
- (void)changed
{
    [[NSNotificationCenter defaultCenter] postNotificationName:vninput::macos::runtimeNotification object:self];
}
- (void)toggleEnabled { _enabledOverride = ![self settings].enabled; [self changed]; }
@end
