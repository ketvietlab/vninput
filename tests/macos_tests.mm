#include "native_session.hpp"
#include <cstdlib>
#include <iostream>

using namespace vninput;
using namespace vninput::macos;
namespace {
int checks = 0;
void check(bool value, const char* message)
{
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void equal(NSString* actual, NSString* expected, const char* message)
{
    if (![actual isEqualToString:expected]) std::cerr << actual.UTF8String << " != " << expected.UTF8String << '\n';
    check([actual isEqualToString:expected], message);
}
NSEvent* key(NSString* text, unsigned short code = kVK_ANSI_A, NSEventModifierFlags modifiers = 0, bool repeat = false)
{
    return [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:modifiers timestamp:0
        windowNumber:0 context:nil characters:text charactersIgnoringModifiers:text isARepeat:repeat keyCode:code];
}
}

// Implements the IMK methods used by the session, with real UTF-16 ranges.
@interface TestClient : NSObject
@property NSMutableString* text;
@property NSRange marked;
@property NSRange selection;
@property BOOL failNext;
@property NSUInteger commits;
@property BOOL unavailableRange;
- (void)insertText:(id)value replacementRange:(NSRange)range;
- (void)setMarkedText:(id)value selectionRange:(NSRange)selection replacementRange:(NSRange)range;
- (NSRange)selectedRange;
- (NSAttributedString*)attributedSubstringFromRange:(NSRange)range;
@end
@implementation TestClient
- (instancetype)init
{
    if ((self = [super init])) { _text = [NSMutableString new]; _marked = NSMakeRange(NSNotFound, 0); _selection = NSMakeRange(0, 0); }
    return self;
}
- (NSRange)target:(NSRange)range
{
    if (_failNext) { _failNext = NO; @throw [NSException exceptionWithName:@"DeadClient" reason:@"simulated" userInfo:nil]; }
    return range.location != NSNotFound ? range : _marked.location != NSNotFound ? _marked : _selection;
}
- (NSRange)selectedRange { return _unavailableRange ? NSMakeRange(NSNotFound, NSNotFound) : _selection; }
- (NSAttributedString*)attributedSubstringFromRange:(NSRange)range
{
    if (_unavailableRange || range.location == NSNotFound || NSMaxRange(range) > _text.length) return nil;
    return [[NSAttributedString alloc] initWithString:[_text substringWithRange:range]];
}
- (void)insertText:(id)value replacementRange:(NSRange)range
{
    NSString* text = [value isKindOfClass:[NSAttributedString class]] ? [value string] : value;
    const auto target = [self target:range];
    [_text replaceCharactersInRange:target withString:text];
    _selection = NSMakeRange(target.location + text.length, 0);
    _marked = NSMakeRange(NSNotFound, 0); ++_commits;
}
- (void)setMarkedText:(id)value selectionRange:(NSRange)selection replacementRange:(NSRange)range
{
    if ([value length]) {
        check([value isKindOfClass:NSAttributedString.class], "preedit includes explicit rendering attributes");
        for (NSUInteger i = 0; i < [value length]; ++i) {
            NSDictionary* attributes = [value attributesAtIndex:i effectiveRange:nullptr];
            NSNumber* style = attributes[NSUnderlineStyleAttributeName];
            NSColor* color = attributes[NSUnderlineColorAttributeName];
            check(style && style.integerValue == NSUnderlineStyleNone, "AppKit preedit has no underline");
            check(color && color.alphaComponent == 0, "Qt receives an explicit transparent underline color");
            check(!attributes[NSForegroundColorAttributeName], "hiding underline does not hide or recolor text");
        }
    }
    NSString* text = [value isKindOfClass:[NSAttributedString class]] ? [value string] : value;
    const auto target = [self target:range];
    [_text replaceCharactersInRange:target withString:text];
    _marked = text.length ? NSMakeRange(target.location, text.length) : NSMakeRange(NSNotFound, 0);
    _selection = NSMakeRange(target.location + selection.location, selection.length);
}
@end

namespace {
id<IMKTextInput> client(TestClient* c) { return (id<IMKTextInput>)c; }
void type(VNInputInputSession* session, TestClient* c, NSString* input)
{
    for (NSUInteger i = 0; i < input.length; ++i) {
        NSString* text = [input substringWithRange:NSMakeRange(i, 1)];
        const auto code = [text isEqualToString:@" "] ? static_cast<unsigned short>(kVK_Space) : static_cast<unsigned short>(kVK_ANSI_A);
        const auto result = [session handleEvent:key(text, code) client:client(c)];
        if (!result.consumed) [c insertText:text replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    }
}
void sessions()
{
    Settings settings;
    auto* session = [VNInputInputSession new]; auto* a = [TestClient new]; auto* b = [TestClient new];
    [session activate:client(a) settings:settings];
    type(session, a, @"tiees"); equal(a.text, @"tiế", "tone before coda");
    type(session, a, @"n"); equal(a.text, @"tiến", "tone survives n");
    type(session, a, @"g"); equal(a.text, @"tiếng", "tone survives ng");
    [session activate:client(a) settings:settings]; equal(a.text, @"tiếng", "duplicate activation keeps composition");
    type(session, a, @" "); equal(a.text, @"tiếng ", "commit before forwarded space");
    check(a.marked.location == NSNotFound, "inline mode never marks text");
    type(session, a, @"dd");
    [session activate:client(b) settings:settings];
    equal(a.text, @"tiếng đ", "client switch commits to original client");
    check(a.marked.location == NSNotFound, "old client has no stale marked text");
    type(session, b, @"aas"); [session commitForClient:client(a)]; [session deactivate:client(a)];
    equal(b.text, @"ấ", "late old callback cannot affect new client");
    check(b.marked.location == NSNotFound && !session.markedText.length, "new word has no marked text");
    [session deactivate:client(b)]; [session commitForClient:client(b)]; [session close];
    equal(b.text, @"ấ", "deactivate/close idempotent"); check(b.commits == 3, "inline edits made before close");
    for (int i = 0; i < 250; ++i) {
        auto* c = [TestClient new]; [session activate:client(c) settings:settings];
        type(session, c, @"ass "); type(session, c, @"tieesng"); [session deactivate:client(c)];
        equal(c.text, @"as tiếng", "new context recovers escaped state");
    }
    auto* c = [TestClient new]; [session activate:client(c) settings:settings];
    type(session, c, @"tieengs");
    check([session handleEvent:key(@"", kVK_Delete) client:client(c)].consumed, "Backspace handled");
    equal(c.text, @"tiến", "Backspace replaces inline word");
    [session handleEvent:key(@"", kVK_Escape) client:client(c)]; equal(c.text, @"tieens", "Escape restores raw word");
    [session commitForClient:client(c)];
    c = [TestClient new]; [session activate:client(c) settings:settings];
    [c insertText:@"🐈" replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    type(session, c, @"dd "); equal(c.text, @"🐈đ ", "UTF-16 client offsets after emoji");
    c.selection = NSMakeRange(0, c.text.length);
    type(session, c, @"aas "); equal(c.text, @"ấ ", "selection replacement");
    type(session, c, @"dd");
    auto outcome = [session handleEvent:key(@"", kVK_LeftArrow) client:client(c)];
    check(!outcome.consumed && c.marked.location == NSNotFound, "navigation commits and passes");
    settings.inputMethod = InputMethod::Vni; type(session, c, @" aa"); [session configure:settings];
    type(session, c, @" a61 "); equal(c.text, @"ấ đ â ấ ", "method switch does not lose old preedit");
    type(session, c, @"a6");
    outcome = [session handleEvent:key(@"Ω", kVK_ANSI_Z, NSEventModifierFlagOption) client:client(c)];
    check(outcome.consumed && outcome.toggle, "Option+Z matches physical key despite Option text");
    check(c.marked.location == NSNotFound, "toggle commits pending text");
    check(![session handleEvent:key(@"Ω", kVK_ANSI_Z, NSEventModifierFlagOption, true) client:client(c)].toggle, "repeat cannot oscillate V/E");
    settings.enabled = false; [session configure:settings]; type(session, c, @" a61 ");
    check([c.text hasSuffix:@"â a61 "], "disabled mode forwards unchanged");
    settings.enabled = true; [session configure:settings]; type(session, c, @"a61 ");
    check([c.text hasSuffix:@"a61 ấ "], "enabled after toggle recovers");
    type(session, c, @"a6");
    check(![session handleEvent:key(@"c", kVK_ANSI_C, NSEventModifierFlagCommand) client:client(c)].consumed, "Command shortcut forwarded");
    check(c.marked.location == NSNotFound, "Command shortcut commits before copy");
    c = [TestClient new]; [session activate:client(c) settings:Settings{}]; c.failNext = YES;
    check(![session handleEvent:key(@"a") client:client(c)].consumed, "disconnected client cannot swallow key");
    type(session, c, @"tieesng "); equal(c.text, @"tiếng ", "next session recovers after client exception");
    c = [TestClient new]; c.unavailableRange = YES;
    [session activate:client(c) settings:Settings{}]; type(session, c, @"tieesng");
    equal(c.text, @"tiếng", "clients without document access use marked text");
    check(c.marked.location != NSNotFound, "fallback keeps composition");
    [session close];
    c = [TestClient new];
    [session activate:client(c) settings:Settings{}
        applicationID:@"com.ketsuite.ketplus.terminal"];
    type(session, c, @"dd "); equal(c.text, @"đ ", "terminal composes before sending text to PTY");
    check(c.commits == 2, "terminal sends one committed word and forwarded space");
    check(c.marked.location == NSNotFound, "terminal preedit ends at word boundary");
    [session close];
    c = [TestClient new];
    Settings terminalVni; terminalVni.inputMethod = InputMethod::Vni;
    [session activate:client(c) settings:terminalVni
        applicationID:@"com.ketsuite.ketplus.terminal"];
    type(session, c, @"tie6ng1 ");
    equal(c.text, @"tiếng ", "terminal VNI converts before commit");
    check(c.commits == 2, "terminal VNI does not send raw keys to PTY");
    [session close];
    c = [TestClient new]; [session activate:client(c) settings:Settings{}];
    type(session, c, @"tie"); c.selection = NSMakeRange(0, 0);
    type(session, c, @"a"); equal(c.text, @"atie", "cursor move cannot rewrite old word");
    c = [TestClient new]; [session activate:client(c) settings:Settings{}];
    type(session, c, @"tie"); [c.text replaceCharactersInRange:NSMakeRange(0, 3) withString:@"xyz"];
    type(session, c, @"a"); equal(c.text, @"xyza", "external same-length edit cannot be replaced by stale word");
    c = [TestClient new]; [session activate:client(c) settings:Settings{}];
    type(session, c, @"tie"); c.unavailableRange = YES;
    type(session, c, @"a"); equal(c.text, @"tiea", "lost document access keeps existing committed text");
    check(c.marked.location != NSNotFound, "lost document access falls back to marked text");
    [session close];
}
void configAndEncoding()
{
    const auto shortcut = parseShortcut("Ctrl+Shift+V");
    check(shortcut && shortcut->modifiers == (NSEventModifierFlagCommand | NSEventModifierFlagShift), "Qt Ctrl maps to Command");
    check(parseShortcut("Meta+Space")->modifiers == NSEventModifierFlagControl, "Qt Meta maps to Control");
    check(!parseShortcut("Alt+Alt+Z") && !parseShortcut("Z") && !parseShortcut("Ctrl+Unknown"), "invalid shortcuts rejected");
    check(parseShortcut("Alt++")->modifiers == (NSEventModifierFlagOption | NSEventModifierFlagShift), "plus chord");
    equal(nativeText(unicodeText(@"Tiếng Việt 🐈 𐐀")), @"Tiếng Việt 🐈 𐐀", "UTF-16/32 roundtrip");
    check(unicodeText(@"").empty(), "empty Unicode");
    const unichar invalid[] = {0xd800, 'x', 0xdc00};
    equal(nativeText(unicodeText([[NSString alloc] initWithCharacters:invalid length:3])), @"�x�", "invalid surrogate handled");
    NSString* directory = [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSUUID UUID] UUIDString]];
    [[NSFileManager defaultManager] createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* path = [directory stringByAppendingPathComponent:@"settings.json"];
    check(readSettings(path).has_value(), "missing file uses defaults");
    NSMutableDictionary* json = [@{@"schemaVersion": @1, @"inputMethod": @"vni", @"tonePlacement": @"classic",
        @"enabled": @NO, @"freeTone": @YES, @"restoreOnEscape": @YES, @"toggleShortcut": @"Alt+Z"} mutableCopy];
    auto write = [&] { [[NSJSONSerialization dataWithJSONObject:json options:0 error:nil] writeToFile:path atomically:YES]; };
    write(); const auto settings = readSettings(path);
    check(settings && settings->inputMethod == InputMethod::Vni && !settings->enabled, "schema compatible");
    json[@"enabled"] = @1; write(); check(!readSettings(path), "numeric boolean rejected"); json[@"enabled"] = @NO;
    json[@"schemaVersion"] = @YES; write(); check(!readSettings(path), "boolean version rejected"); json[@"schemaVersion"] = @999;
    write(); check(!readSettings(path), "future schema rejected"); json[@"schemaVersion"] = @1;
    json[@"inputMethod"] = @"unknown"; write(); check(!readSettings(path), "invalid method rejected");
    [@"broken JSON" writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil];
    check(!readSettings(path), "corrupt file rejected");
    [[NSMutableData dataWithLength:65537] writeToFile:path atomically:YES]; check(!readSettings(path), "bounded config read");
    [[NSFileManager defaultManager] removeItemAtPath:directory error:nil];
}
}
int main()
{
    @autoreleasepool { configAndEncoding(); sessions(); }
    std::cout << "macOS adapter: " << checks << " checks passed\n";
}
