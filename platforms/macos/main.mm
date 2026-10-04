#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>
#include "native_settings.hpp"

int main(int argc, const char* argv[])
{
    @autoreleasepool {
        if (argc == 2 && std::string_view(argv[1]) == "--config-path") {
            puts(vninput::macos::settingsPath().c_str()); return 0;
        }
        [NSApplication sharedApplication];
        NSBundle* bundle = [NSBundle mainBundle];
        __attribute__((objc_precise_lifetime)) IMKServer* server = [[IMKServer alloc] initWithName:[bundle objectForInfoDictionaryKey:@"InputMethodConnectionName"]
            bundleIdentifier:bundle.bundleIdentifier];
        if (!server) return 1;
        [VNInputConfiguration shared];
        [NSApp run];
        (void)server; // Keep the server alive for the entire run loop under ARC.
    }
    return 0;
}
