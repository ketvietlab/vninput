#include "native_session.hpp"

using namespace vninput;
using namespace vninput::macos;

@implementation VNInputInputSession {
    std::unique_ptr<Engine> _engine;
    Settings _settings;
    id<IMKTextInput> _client;
    NSString* _markedText;
    NSString* _inlineText;
    NSRange _inlineRange;
    BOOL _supportsDocumentReplacement;
}
- (instancetype)init
{
    if ((self = [super init])) {
        _engine = makeEngine(); _markedText = @""; _inlineText = @"";
        _inlineRange = NSMakeRange(NSNotFound, 0);
    }
    return self;
}
- (NSString*)markedText { return _markedText; }
- (void)clearInline
{
    _inlineText = @"";
    _inlineRange = NSMakeRange(NSNotFound, 0);
}
- (BOOL)inlineStillMatches
{
    if (!_inlineText.length) return YES;
    const NSRange selection = [_client selectedRange];
    if (selection.location == NSNotFound || selection.length ||
        selection.location != NSMaxRange(_inlineRange)) return NO;
    NSAttributedString* actual = [_client attributedSubstringFromRange:_inlineRange];
    return actual && [actual.string isEqualToString:_inlineText];
}
- (BOOL)applyInline:(const Composition&)composition
{
    NSString* committed = nativeText(composition.commit);
    NSString* next = nativeText(composition.preedit);
    if (!_inlineText.length && !next.length) {
        if (committed.length) [_client insertText:committed replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
        return YES;
    }
    if (_inlineText.length && !next.length && [committed isEqualToString:_inlineText]) {
        [self clearInline];
        return YES;
    }
    NSString* replacement = [committed stringByAppendingString:next];
    const NSRange selection = _inlineText.length ? _inlineRange : [_client selectedRange];
    if (selection.location == NSNotFound) return NO;
    // A new word replaces the current selection. Later edits replace only
    // the exact word VNInput last inserted, without marked text or backspaces.
    [_client insertText:replacement replacementRange:_inlineText.length ? selection : NSMakeRange(NSNotFound, NSNotFound)];
    _inlineText = next;
    _inlineRange = next.length ? NSMakeRange(selection.location + committed.length, next.length)
                               : NSMakeRange(NSNotFound, 0);
    return YES;
}
- (BOOL)apply:(const Composition&)composition
{
    if (!_client) { _engine->reset(); _markedText = @""; [self clearInline]; return NO; }
    @try {
        if (_inlineText.length || (_supportsDocumentReplacement &&
            [_client selectedRange].location != NSNotFound && !_markedText.length))
            return [self applyInline:composition];
        const NSRange currentSelection = NSMakeRange(NSNotFound, NSNotFound);
        if (!composition.commit.empty()) {
            [_client insertText:nativeText(composition.commit) replacementRange:currentSelection];
            _markedText = @""; // insertText replaces and unmarks the old composition.
        }
        NSString* next = nativeText(composition.preedit);
        if (![_markedText isEqualToString:next]) {
            // Qt's Cocoa bridge treats style zero as an underline; explicitly
            // make it transparent without changing the text color. Chromium
            // instead interprets transparent as "use text color", so this is
            // not a cross-host guarantee (see docs/macos.md).
            NSAttributedString* marked = [[NSAttributedString alloc] initWithString:next
                attributes:@{NSUnderlineStyleAttributeName: @(NSUnderlineStyleNone),
                    NSUnderlineColorAttributeName: NSColor.clearColor}];
            [_client setMarkedText:marked selectionRange:NSMakeRange(next.length, 0) replacementRange:currentSelection];
            _markedText = next;
        }
        return YES;
    } @catch (NSException*) {
        // A dead client must not poison this engine or a later input context.
        // Never log text/exception descriptions: they can contain user input.
        _engine->reset(); _markedText = @""; [self clearInline]; _client = nil;
        return NO;
    }
}
- (void)activate:(id<IMKTextInput>)client settings:(const Settings&)settings
{
    [self activate:client settings:settings
        applicationID:NSWorkspace.sharedWorkspace.frontmostApplication.bundleIdentifier];
}
- (void)activate:(id<IMKTextInput>)client settings:(const Settings&)settings
    applicationID:(NSString*)applicationID
{
    if (_client == client && client) { [self configure:settings]; return; }
    [self close];
    _client = client;
    // KetPlus Terminal is a PTY, not an editable document. It accepts IME
    // preedit/commit but cannot replace already-sent bytes from a prior key.
    _supportsDocumentReplacement = ![applicationID isEqualToString:@"com.ketsuite.ketplus.terminal"];
    _settings = settings;
    _engine->configure(settings);
    _engine->reset();
}
- (void)configure:(const Settings&)settings
{
    if (_settings == settings) return;
    _engine->configure(settings);
    [self apply:_engine->flush()];
    _settings = settings;
}
- (EventOutcome)handleEvent:(NSEvent*)event client:(id<IMKTextInput>)client
{
    if (!event || !client) { [self commitForClient:_client]; return {}; }
    if (event.type != NSEventTypeKeyDown) return {};
    if (_client != client) [self activate:client settings:_settings];
    @try {
        if (_inlineText.length && ![self inlineStillMatches]) {
            // The editor, undo, or another input source changed our word.
            // Never replace text whose ownership we can no longer verify.
            _engine->reset(); [self clearInline];
        }
    } @catch (NSException*) {
        _engine->reset(); [self clearInline];
    }
    if (const auto shortcut = parseShortcut(_settings.toggleShortcut); shortcut && matchesShortcut(event, *shortcut)) {
        [self commitForClient:client];
        return {true, !event.isARepeat};
    }
    KeyEvent key;
    switch (event.keyCode) {
    case kVK_Delete: key.key = Key::Backspace; break;
    case kVK_Escape: key.key = Key::Escape; break;
    case kVK_Return: case kVK_ANSI_KeypadEnter: key.key = Key::Enter; break;
    case kVK_Space: key.key = Key::Space; break;
    case kVK_LeftArrow: case kVK_RightArrow: case kVK_UpArrow: case kVK_DownArrow:
    case kVK_Home: case kVK_End: case kVK_PageUp: case kVK_PageDown:
    case kVK_ForwardDelete: case kVK_Tab: key.key = Key::Other; break;
    default: key.key = event.characters.length ? Key::Text : Key::Other; break;
    }
    key.text = unicodeText(event.characters);
    if (event.modifierFlags & NSEventModifierFlagShift) key.modifiers |= static_cast<std::uint8_t>(Modifier::Shift);
    if (event.modifierFlags & NSEventModifierFlagControl) key.modifiers |= static_cast<std::uint8_t>(Modifier::Control);
    if (event.modifierFlags & NSEventModifierFlagOption) key.modifiers |= static_cast<std::uint8_t>(Modifier::Alt);
    if (event.modifierFlags & NSEventModifierFlagCommand) key.modifiers |= static_cast<std::uint8_t>(Modifier::Meta);
    const auto result = _engine->process(key);
    if (!_markedText.length && !_inlineText.length && result.preedit.empty() && result.commit.empty())
        return {result.consumed, false};
    return {[self apply:result] && result.consumed, false};
}
- (void)commitForClient:(id<IMKTextInput>)client
{
    // Late callbacks from a previous client cannot commit into a new one.
    if (!client || _client != client) return;
    const auto result = _engine->flush();
    if (_markedText.length || _inlineText.length || !result.commit.empty()) [self apply:result];
}
- (void)deactivate:(id<IMKTextInput>)client
{
    if (client && client != _client) return;
    [self close];
}
- (void)close
{
    [self commitForClient:_client];
    _engine->reset(); _markedText = @""; [self clearInline]; _client = nil;
}
@end
