#include "native_session.hpp"
#include <QApplication>
#include <QTextEdit>
#include <QInputMethodEvent>
#include <cstdio>
#include <string>

// Exercise VNInput through Qt's real macOS NSTextInputClient bridge. This is
// an interactive desktop check, not an offscreen ctest: QNSView is required.
@interface QtClient : NSObject
@property(nonatomic, assign) id<NSTextInputClient> view;
@property(nonatomic) NSUInteger markedCalls;
- (instancetype)initWithView:(id<NSTextInputClient>)view;
@end
@implementation QtClient
- (instancetype)initWithView:(id<NSTextInputClient>)view
{
    if ((self = [super init])) _view = view;
    return self;
}
- (void)insertText:(id)value replacementRange:(NSRange)range
{ [_view insertText:value replacementRange:range]; }
- (void)setMarkedText:(id)value selectionRange:(NSRange)selection replacementRange:(NSRange)range
{ ++_markedCalls; [_view setMarkedText:value selectedRange:selection replacementRange:range]; }
- (NSRange)selectedRange { return [_view selectedRange]; }
- (NSAttributedString*)attributedSubstringFromRange:(NSRange)range
{ return [_view attributedSubstringForProposedRange:range actualRange:nullptr]; }
@end

class Editor : public QTextEdit {
public:
    bool receivedPreedit = false;
    void inputMethodEvent(QInputMethodEvent* event) override
    {
        if (!event->preeditString().isEmpty()) receivedPreedit = true;
        QTextEdit::inputMethodEvent(event);
    }
};

int main(int argc, char** argv)
{
    @autoreleasepool {
        QApplication app(argc, argv);
        Editor edit;
        edit.resize(400, 120); edit.show(); edit.activateWindow(); edit.setFocus();
        app.processEvents();
        auto* bridge = [[QtClient alloc] initWithView:(__bridge id<NSTextInputClient>)(void*)edit.winId()];
        auto* session = [VNInputInputSession new];
        vninput::Settings settings;
        const bool vni = argc > 1 && std::string(argv[1]) == "--vni";
        if (vni) settings.inputMethod = vninput::InputMethod::Vni;
        [session activate:(id<IMKTextInput>)bridge settings:settings];
        for (char c : vni ? std::string("tie6ng1 ") : std::string("tieesng ")) {
            NSString* character = [NSString stringWithFormat:@"%c", c];
            NSEvent* event = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0
                timestamp:0 windowNumber:0 context:nil characters:character charactersIgnoringModifiers:character
                isARepeat:NO keyCode:c == ' ' ? static_cast<unsigned short>(kVK_Space)
                                               : static_cast<unsigned short>(kVK_ANSI_A)];
            auto outcome = [session handleEvent:event client:(id<IMKTextInput>)bridge];
            if (!outcome.consumed) [bridge insertText:character replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
            app.processEvents();
        }
        [session close];
        const bool okay = edit.toPlainText() == QString::fromUtf8("tiếng ") &&
            !edit.receivedPreedit && bridge.markedCalls == 0;
        printf("Qt bridge: text=%s preedit=%d markedCalls=%lu\n", edit.toPlainText().toUtf8().constData(),
            edit.receivedPreedit, static_cast<unsigned long>(bridge.markedCalls));
        return okay ? 0 : 1;
    }
}
