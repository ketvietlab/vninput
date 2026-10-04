#include "native_settings.hpp"
#import <Carbon/Carbon.h>
#include <cstdio>
#include <cstdlib>

using namespace vninput;
using namespace vninput::macos;
namespace {
void pump(double seconds)
{
    NSDate* end = [NSDate dateWithTimeIntervalSinceNow:seconds];
    while (end.timeIntervalSinceNow > 0) {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:end inMode:NSDefaultRunLoopMode dequeue:YES];
        if (event) [NSApp sendEvent:event];
    }
}
void expect(BOOL value, NSString* message)
{
    if (!value) @throw [NSException exceptionWithName:@"VNInputLiveCheck" reason:message userInfo:nil];
}
void sendKey(NSTextView* view, NSString* text, unsigned short code, NSEventModifierFlags flags = 0)
{
    NSEvent* event = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:flags
        timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil
        characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
    // This is the application's real NSTextInputContext -> macOS -> IMKServer
    // path, not a direct call to VNInputInputSession or the C++ engine.
    [NSApp postEvent:event atStart:NO];
    pump(0.04);
}
void sendText(NSTextView* view, NSString* text)
{
    for (NSUInteger i = 0; i < text.length; ++i) {
        NSString* character = [text substringWithRange:NSMakeRange(i, 1)];
        if ([character isEqualToString:@" "]) sendKey(view, character, kVK_Space);
        else {
            const auto parsed = parseShortcut(std::string("Alt+") + character.uppercaseString.UTF8String);
            expect(parsed.has_value(), @"Unsupported test character");
            sendKey(view, character, parsed->keyCode);
        }
    }
}
}

@interface LiveCheck : NSObject<NSApplicationDelegate>
@property NSWindow* window;
@property NSTextView* first;
@property NSTextView* second;
@property NSString* output;
@property NSRunningApplication* priorApp;
@property TISInputSourceRef priorSource;
@property BOOL manual;
@end
@implementation LiveCheck
- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(180, 200, 600, 320)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"VNInput — Native input verification";
    self.first = [[NSTextView alloc] initWithFrame:NSMakeRect(20, 165, 560, 130)];
    self.second = [[NSTextView alloc] initWithFrame:NSMakeRect(20, 20, 560, 130)];
    self.first.accessibilityLabel = @"First input field";
    self.second.accessibilityLabel = @"Second input field";
    for (NSTextView* view in @[self.first, self.second]) {
        view.font = [NSFont systemFontOfSize:20]; view.richText = NO;
        view.automaticSpellingCorrectionEnabled = NO;
        view.automaticTextReplacementEnabled = NO;
        [self.window.contentView addSubview:view];
    }
    [self.window makeKeyAndOrderFront:nil]; [NSApp activate];
    [self.window makeFirstResponder:self.first];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (self.manual) {
            [self.first.inputContext activate];
            self.first.inputContext.selectedKeyboardInputSource = @"vn.vninput.inputmethod.VNInput";
        } else [self runChecks];
    });
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app { (void)app; return YES; }
- (void)applicationWillTerminate:(NSNotification*)notification
{
    (void)notification;
    if (self.priorSource) { TISSelectInputSource(self.priorSource); CFRelease(self.priorSource); self.priorSource = nullptr; }
}
- (void)runChecks
{
    int status = 0;
    NSMutableArray* checks = [NSMutableArray array];
    @try {
        const auto settings = readSettingsFile(settingsPath());
        expect(settings && settings->enabled, @"Enable Vietnamese in saved VNInput settings before this interactive check.");
        const bool telex = settings->inputMethod == InputMethod::Telex;
        [self.first.inputContext activate];
        self.first.inputContext.selectedKeyboardInputSource = @"vn.vninput.inputmethod.VNInput";
        pump(1.5);
        expect(inputSourceStatus() == SourceStatus::Selected, @"macOS has not made the installed VNInput source selectable. A new login may be needed.");
        sendText(self.first, telex ? @"tiees" : @"tie61");
        expect([self.first.string isEqualToString:@"tiế"] && self.first.hasMarkedText, @"Live preedit did not produce tiế.");
        [checks addObject:@"early tone: tiế"];
        sendText(self.first, @"ng ");
        expect([self.first.string isEqualToString:@"tiếng "] && !self.first.hasMarkedText, @"Live commit did not produce tiếng.");
        [checks addObject:@"commit: tiếng"];
        sendText(self.first, telex ? @"dd" : @"d9");
        [self.window makeFirstResponder:self.second];
        [self.second.inputContext activate];
        self.second.inputContext.selectedKeyboardInputSource = @"vn.vninput.inputmethod.VNInput";
        pump(0.2);
        sendText(self.second, telex ? @"dduowngf " : @"d9u7o7ng2 ");
        expect([self.first.string isEqualToString:@"tiếng đ"] && !self.first.hasMarkedText, @"Focus change lost/duplicated pending text.");
        expect([self.second.string isEqualToString:@"đường "], @"Second input context did not compose independently.");
        [checks addObject:@"focus/second context: đường"];
        self.second.inputContext.selectedKeyboardInputSource = @"com.apple.keylayout.ABC";
        pump(0.2); sendText(self.second, @"abc ");
        self.second.inputContext.selectedKeyboardInputSource = @"vn.vninput.inputmethod.VNInput";
        pump(0.2); sendText(self.second, telex ? @"tieengs " : @"tie6ng1 ");
        expect([self.second.string isEqualToString:@"đường abc tiếng "], @"Reactivation after ABC did not recover.");
        [checks addObject:@"ABC → VNInput recovery"];
        if (self.output.length) {
            [[NSFileManager defaultManager] createDirectoryAtPath:self.output withIntermediateDirectories:YES attributes:nil error:nil];
            NSView* view = self.window.contentView;
            NSBitmapImageRep* bitmap = [view bitmapImageRepForCachingDisplayInRect:view.bounds];
            [view cacheDisplayInRect:view.bounds toBitmapImageRep:bitmap];
            [[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}]
                writeToFile:[self.output stringByAppendingPathComponent:@"native-input.png"] atomically:YES];
        }
    } @catch (NSException* exception) {
        status = 1; [checks addObject:exception.reason ? exception.reason : @"Native input check failed"];
        [checks addObject:[NSString stringWithFormat:@"Test field 1: %@; marked=%d; source=%@",
            self.first.string, self.first.hasMarkedText, self.first.inputContext.selectedKeyboardInputSource]];
        [checks addObject:[NSString stringWithFormat:@"Test field 2: %@", self.second.string]];
    } @finally {
        [self.first.inputContext discardMarkedText]; [self.second.inputContext discardMarkedText];
        if (self.priorSource) {
            const auto restored = TISSelectInputSource(self.priorSource);
            if (restored != noErr) { status = 1; [checks addObject:@"Could not restore the previous input source"]; }
            CFRelease(self.priorSource); self.priorSource = nullptr;
        }
        [self.window orderOut:nil];
        [self.priorApp activateWithOptions:0];
    }
    NSData* evidence = [NSJSONSerialization dataWithJSONObject:@{@"passed": @(status == 0), @"checks": checks}
        options:NSJSONWritingPrettyPrinted error:nil];
    if (self.output.length) {
        [[NSFileManager defaultManager] createDirectoryAtPath:self.output withIntermediateDirectories:YES attributes:nil error:nil];
        [evidence writeToFile:[self.output stringByAppendingPathComponent:@"native-input.json"] atomically:YES];
    }
    fwrite(evidence.bytes, 1, evidence.length, stdout); puts("");
    std::exit(status);
}
@end

int main(int argc, const char* argv[])
{
    @autoreleasepool {
        if (argc != 3 || (std::string_view(argv[1]) != "--run" && std::string_view(argv[1]) != "--manual")) {
            fprintf(stderr, "Explicit desktop check: vninput-native-check --run /path/to/evidence\n"); return 2;
        }
        auto* delegate = [LiveCheck new];
        delegate.manual = std::string_view(argv[1]) == "--manual";
        delegate.output = [NSString stringWithUTF8String:argv[2]];
        delegate.priorApp = [NSWorkspace sharedWorkspace].frontmostApplication;
        delegate.priorSource = TISCopyCurrentKeyboardInputSource();
        [NSApplication sharedApplication]; NSApp.delegate = delegate;
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp run];
    }
}
