#import <Cocoa/Cocoa.h>
#include <cmath>
#include <cstdio>

int main(int argc, const char* argv[])
{
    @autoreleasepool {
        if (argc != 2) return 2;
        NSBundle* bundle = [NSBundle bundleWithPath:[NSString stringWithUTF8String:argv[1]]];
        NSString* name = bundle.infoDictionary[@"tsInputMethodIconFileKey"];
        if (![name isKindOfClass:NSString.class] || !name.length) return 1;
        NSString* path = [bundle.resourcePath stringByAppendingPathComponent:name];
        NSImage* icon = [[NSImage alloc] initWithContentsOfFile:path];
        const NSSize size = icon.size;
        printf("Input source icon: %s; logical size %.2f x %.2f pt\n", name.UTF8String, size.width, size.height);
        // Menu rows can grow to the image's logical height, regardless of pixel
        // density. Test the packaged resource through AppKit, not just its name.
        if (!icon.valid || !std::isfinite(size.width) || !std::isfinite(size.height)
            || size.width < 1 || size.height < 1 || size.width > 18 || size.height > 18) {
            fprintf(stderr, "Input source icon must fit within 18 x 18 points.\n");
            return 1;
        }
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Input menu layout check"];
        NSMenuItem* item = [menu addItemWithTitle:@"VNInput" action:nil keyEquivalent:@""];
        const CGFloat plainHeight = menu.size.height;
        item.image = icon;
        const CGFloat iconHeight = menu.size.height;
        printf("AppKit menu height: %.2f pt without icon; %.2f pt with icon\n", plainHeight, iconHeight);
        if (iconHeight > plainHeight + 4) {
            fprintf(stderr, "Input source icon must not enlarge the native menu row.\n");
            return 1;
        }
        return 0;
    }
}
