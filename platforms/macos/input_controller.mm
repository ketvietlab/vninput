#include "native_session.hpp"
#import <InputMethodKit/InputMethodKit.h>

@interface VNInputInputController : IMKInputController
@end

@implementation VNInputInputController {
    VNInputInputSession* _session;
}
- (id)initWithServer:(IMKServer*)server delegate:(id)delegate client:(id)client
{
    if ((self = [super initWithServer:server delegate:delegate client:client])) {
        _session = [VNInputInputSession new];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(configurationChanged:)
            name:vninput::macos::runtimeNotification object:nil];
    }
    return self;
}
- (void)dealloc { [[NSNotificationCenter defaultCenter] removeObserver:self]; }
- (NSUInteger)recognizedEvents:(id)sender
{
    (void)sender;
    // KeyDown only keeps IMK's default mouse-outside-composition commit handling.
    return NSEventMaskKeyDown;
}
- (void)activateServer:(id)sender
{
    VNInputConfiguration* config = [VNInputConfiguration shared];
    [config reload];
    [_session activate:sender settings:[config settings]];
    @try { [(id<IMKTextInput>)sender overrideKeyboardWithKeyboardNamed:@"com.apple.keylayout.ABC"]; }
    @catch (NSException*) { /* Some clients cannot override the keyboard layout. */ }
}
- (BOOL)handleEvent:(NSEvent*)event client:(id)sender
{
    const auto result = [_session handleEvent:event client:sender];
    if (result.toggle) [[VNInputConfiguration shared] toggleEnabled];
    return result.consumed;
}
- (void)commitComposition:(id)sender { [_session commitForClient:sender]; }
- (id)composedString:(id)sender { (void)sender; return _session.markedText; }
- (void)deactivateServer:(id)sender { [_session deactivate:sender]; }
- (void)inputControllerWillClose { [_session close]; }
- (void)configurationChanged:(NSNotification*)notification
{
    (void)notification;
    [_session configure:[[VNInputConfiguration shared] settings]];
}
- (NSMenu*)menu
{
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"VNInput"];
    NSMenuItem* enabled = [[NSMenuItem alloc] initWithTitle:@"Tiếng Việt" action:@selector(toggleVietnamese:) keyEquivalent:@""];
    enabled.target = self;
    enabled.state = [[VNInputConfiguration shared] settings].enabled ? NSControlStateValueOn : NSControlStateValueOff;
    [menu addItem:enabled];
    [menu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* settings = [[NSMenuItem alloc] initWithTitle:@"Cài đặt VNInput…" action:@selector(showPreferences:) keyEquivalent:@""];
    settings.target = self; [menu addItem:settings];
    return menu;
}
- (void)toggleVietnamese:(id)sender { (void)sender; [[VNInputConfiguration shared] toggleEnabled]; }
- (void)showPreferences:(id)sender
{
    (void)sender;
    NSURL* app = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"vn.vninput.settings"];
    if (!app) {
        NSString* path = [NSHomeDirectory() stringByAppendingPathComponent:@"Applications/VNInput Settings.app"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:path]) app = [NSURL fileURLWithPath:path];
    }
    if (app) [[NSWorkspace sharedWorkspace] openApplicationAtURL:app configuration:[NSWorkspaceOpenConfiguration configuration]
        completionHandler:nil];
}
@end
