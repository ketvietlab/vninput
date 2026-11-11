#pragma once
#include "native_settings.hpp"
#include <vninput/engine.hpp>
#import <Carbon/Carbon.h>

namespace vninput::macos {
struct EventOutcome { bool consumed = false; bool toggle = false; };
}

@interface VNInputInputSession : NSObject
@property(nonatomic, readonly) NSString* markedText;
- (void)activate:(id<IMKTextInput>)client settings:(const vninput::Settings&)settings;
- (void)configure:(const vninput::Settings&)settings;
- (vninput::macos::EventOutcome)handleEvent:(NSEvent*)event client:(id<IMKTextInput>)client;
- (void)commitForClient:(id<IMKTextInput>)client;
- (void)deactivate:(id<IMKTextInput>)client;
- (void)close;
@end
