#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#include <cstdio>
#include <string_view>

namespace {
int selectAsciiLayout()
{
    TISInputSourceRef layout = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
    if (!layout) { fprintf(stderr, "No ASCII keyboard layout is available.\n"); return 1; }
    const OSStatus status = TISSelectInputSource(layout);
    CFRelease(layout);
    if (status != noErr) fprintf(stderr, "TISSelectInputSource: %d\n", static_cast<int>(status));
    return status == noErr ? 0 : 1;
}
}

int main(int argc, const char* argv[])
{
    @autoreleasepool {
        const std::string_view command = argc > 1 ? argv[1] : "--status";
        if (command == "--register" && argc == 3) {
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[2]]];
            if (![[NSBundle bundleWithURL:url].bundleIdentifier isEqualToString:@"vn.vninput.inputmethod.VNInput"]) return 2;
            const OSStatus status = TISRegisterInputSource((__bridge CFURLRef)url);
            if (status != noErr) { fprintf(stderr, "TISRegisterInputSource: %d\n", static_cast<int>(status)); return 1; }
        } else if (command == "--select-ascii") {
            if (selectAsciiLayout()) return 1;
        } else if (command != "--status" && command != "--enable" && command != "--disable" && command != "--select") {
            fprintf(stderr, "Usage: vninput-input-source --status | --register /path/VNInput.app | --enable | --disable | --select | --select-ascii\n"); return 2;
        }
        NSDictionary* filter = @{(__bridge NSString*)kTISPropertyBundleID: @"vn.vninput.inputmethod.VNInput"};
        CFArrayRef sources = TISCreateInputSourceList((__bridge CFDictionaryRef)filter, true);
        for (CFIndex i = 0; sources && i < CFArrayGetCount(sources); ++i) {
            auto source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, i);
            OSStatus status = noErr;
            if (command == "--enable") status = TISEnableInputSource(source);
            if (command == "--select") status = TISSelectInputSource(source);
            if (command == "--disable") {
                if ([(__bridge NSNumber*)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsSelected) boolValue]
                    && selectAsciiLayout()) { CFRelease(sources); return 1; }
                status = TISDisableInputSource(source);
            }
            if (status != noErr) { CFRelease(sources); fprintf(stderr, "%s: %d\n", command.data(), static_cast<int>(status)); return 1; }
        }
        // Query again after mutation; TIS objects can retain their old properties.
        if (sources) CFRelease(sources);
        sources = TISCreateInputSourceList((__bridge CFDictionaryRef)filter, true);
        NSMutableArray* found = [NSMutableArray array];
        for (CFIndex i = 0; sources && i < CFArrayGetCount(sources); ++i) {
            auto source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, i);
            NSString* identifier = (__bridge NSString*)TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
            NSURL* iconURL = (__bridge NSURL*)TISGetInputSourceProperty(source, kTISPropertyIconImageURL);
            NSImage* icon = iconURL ? [[NSImage alloc] initWithContentsOfURL:iconURL] : nil;
            [found addObject:@{@"id": identifier ? identifier : @"", @"enabled": @([(__bridge NSNumber*)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsEnabled) boolValue]),
                @"selected": @([(__bridge NSNumber*)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsSelected) boolValue]),
                @"iconPath": iconURL.path ? iconURL.path : @"", @"iconWidthPoints": @(icon.size.width), @"iconHeightPoints": @(icon.size.height)}];
        }
        if (sources) CFRelease(sources);
        TISInputSourceRef current = TISCopyCurrentKeyboardInputSource();
        NSString* currentID = current ? (__bridge NSString*)TISGetInputSourceProperty(current, kTISPropertyInputSourceID) : @"";
        NSData* json = [NSJSONSerialization dataWithJSONObject:@{@"registered": @(found.count > 0), @"sources": found, @"currentInputSource": currentID ? currentID : @""}
            options:NSJSONWritingPrettyPrinted error:nil];
        if (current) CFRelease(current);
        fwrite(json.bytes, 1, json.length, stdout); puts("");
        return found.count ? 0 : 3;
    }
}
