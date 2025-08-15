/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2013-2016 zilongshanren
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#import "axmol/ui/UIEditBox/Mac/UIEditBoxMac.h"
#include "axmol/base/Director.h"
#include "axmol/ui/UIEditBox/Mac/UISingleLineTextField.h"
#include "axmol/ui/UIEditBox/Mac/UIPasswordTextField.h"
#include "axmol/ui/UIEditBox/Mac/UIMultilineTextField.h"

#define getEditBoxImplMac() ((ax::ui::EditBoxImplMac*)_editBox)

@implementation UIEditBoxImplMac

- (instancetype)initWithFrame:(NSRect)frameRect editBox:(void*)editBox
{
    self = [super init];

    if (self)
    {

        _editState     = NO;
        self.frameRect = frameRect;

        self.editBox            = editBox;
        self.dataInputMode      = ax::ui::EditBox::InputFlag::LOWERCASE_ALL_CHARACTERS;
        self.keyboardReturnType = ax::ui::EditBox::KeyboardReturnType::DEFAULT;

        [self createMultiLineTextField];
    }

    return self;
}

- (void)createSingleLineTextField
{
    CCUISingleLineTextField* textField = [[[CCUISingleLineTextField alloc] initWithFrame:self.frameRect] autorelease];

    self.textInput = textField;
}

- (void)createMultiLineTextField
{
    CCUIMultilineTextField* textView = [[[CCUIMultilineTextField alloc] initWithFrame:self.frameRect] autorelease];
    [textView setVerticallyResizable:NO];
    self.textInput = textView;
}

- (void)createPasswordTextField
{
    CCUIPasswordTextField* textField = [[[CCUIPasswordTextField alloc] initWithFrame:self.frameRect] autorelease];

    self.textInput = textField;
}

- (void)setTextInput:(NSView<AXUITextInput>*)textInput
{
    if (_textInput == textInput)
    {
        return;
    }

    // Migrate properties
    textInput.axui_textColor   = _textInput.axui_textColor ?: [NSColor whiteColor];
    textInput.axui_text        = _textInput.axui_text ?: @"";
    textInput.axui_placeholder = _textInput.axui_placeholder ?: @"";
    textInput.axui_font        = _textInput.axui_font ?: [NSFont systemFontOfSize:self.frameRect.size.height * 3 / 2];
    textInput.axui_maxLength   = getEditBoxImplMac()->getMaxLength();
    textInput.axui_alignment   = _textInput.axui_alignment;

    [_textInput removeFromSuperview];
    [_textInput release];

    _textInput = [textInput retain];

    [_textInput performSelector:@selector(setTextColor:) withObject:_textInput.axui_textColor];
    [_textInput performSelector:@selector(setBackgroundColor:) withObject:[NSColor clearColor]];

    if (![_textInput isKindOfClass:[NSTextView class]])
    {
        [_textInput performSelector:@selector(setBordered:) withObject:nil];
    }
    _textInput.hidden     = NO;
    _textInput.wantsLayer = YES;

    [_textInput axui_setDelegate:self];

    [self setInputFlag:self.dataInputMode];
    [self setReturnType:self.keyboardReturnType];
}

- (void)updateFrame:(CGRect)rect
{
    NSRect frame      = self.textInput.frame;
    frame.origin.x    = rect.origin.x;
    frame.origin.y    = rect.origin.y;
    frame.size.height = rect.size.height;
    frame.size.width  = rect.size.width;

    self.textInput.frame = frame;
}

- (void)dealloc
{
    self.textInput = nil;

    [super dealloc];
}

- (NSWindow*)window
{
    auto renderView = ax::Director::getInstance()->getRenderView();
    return (NSWindow*)renderView->getCocoaWindow();
}

- (void)openKeyboard
{
    [self.window.contentView addSubview:self.textInput];

    if (![self.textInput isKindOfClass:[NSTextView class]])
    {
        [self.textInput becomeFirstResponder];
    }
    else
    {
        [self.window makeFirstResponder:self.textInput];
    }

    auto editbox = getEditBoxImplMac()->getEditBox();
    auto oldPos  = editbox->getPosition();
    editbox->setPosition(oldPos + ax::Vec2(10, 20));
    editbox->setPosition(oldPos);
}

- (void)closeKeyboard
{
    if (![self.textInput isKindOfClass:[NSTextView class]])
    {
        [self.textInput resignFirstResponder];
    }

    [self.textInput removeFromSuperview];
}

- (const char*)getText
{
    return [self.textInput.axui_text UTF8String];
}

- (void)controlTextDidBeginEditing:(NSNotification*)notification
{
    _editState = YES;

    getEditBoxImplMac()->editBoxEditingDidBegin();
}

- (void)controlTextDidEndEditing:(NSNotification*)notification
{
    _editState = NO;

    getEditBoxImplMac()->editBoxEditingDidEnd([self getText], [self getEndAction:notification]);
}

- (void)setMaxLength:(int)length
{
    self.textInput.axui_maxLength = length;
}

/**
 * Called each time when the text field's text has changed.
 */
- (void)controlTextDidChange:(NSNotification*)notification
{
    getEditBoxImplMac()->editBoxEditingChanged([self getText]);
}

- (NSString*)getDefaultFontName
{
    return self.textInput.axui_font.fontName ?: @"";
}

- (void)setInputMode:(ax::ui::EditBox::InputMode)inputMode
{
    // multiline input
    if (inputMode == ax::ui::EditBox::InputMode::ANY)
    {
        if (![self.textInput isKindOfClass:[NSTextView class]])
        {
            [self createMultiLineTextField];
        }
    }
    else
    {
        if (self.dataInputMode != ax::ui::EditBox::InputFlag::PASSWORD)
        {
            if (![self.textInput isKindOfClass:[NSTextField class]])
            {
                [self createSingleLineTextField];
            }
        }
    }
}

- (void)setInputFlag:(ax::ui::EditBox::InputFlag)inputFlag
{
    if (self.dataInputMode == inputFlag)
    {
        return;
    }

    if (self.dataInputMode == ax::ui::EditBox::InputFlag::PASSWORD &&
        inputFlag != ax::ui::EditBox::InputFlag::PASSWORD)
    {
        [self createSingleLineTextField];
    }

    if (self.dataInputMode != ax::ui::EditBox::InputFlag::PASSWORD &&
        inputFlag == ax::ui::EditBox::InputFlag::PASSWORD)
    {
        [self createPasswordTextField];
    }

    switch (inputFlag)
    {
    case ax::ui::EditBox::InputFlag::PASSWORD:
        self.dataInputMode = inputFlag;
        break;
    case ax::ui::EditBox::InputFlag::INITIAL_CAPS_WORD:
        AXLOGD("INITIAL_CAPS_WORD not implemented");
        break;
    case ax::ui::EditBox::InputFlag::INITIAL_CAPS_SENTENCE:
        AXLOGD("INITIAL_CAPS_SENTENCE not implemented");
        break;
    case ax::ui::EditBox::InputFlag::INITIAL_CAPS_ALL_CHARACTERS:
        AXLOGD("INITIAL_CAPS_ALL_CHARACTERS not implemented");
        break;
    case ax::ui::EditBox::InputFlag::SENSITIVE:
        AXLOGD("SENSITIVE not implemented");
        break;
    case ax::ui::EditBox::InputFlag::LOWERCASE_ALL_CHARACTERS:
        AXLOGD("LOWERCASE_ALL_CHARACTERS not implemented");
        break;
    default:
        break;
    }
}

- (void)setReturnType:(ax::ui::EditBox::KeyboardReturnType)returnType
{
    AXLOGD("setReturnType not implemented");
}

- (void)setTextHorizontalAlignment:(ax::TextHAlignment)alignment
{
    // swizzle center & right, for some reason they're backwards on !TARGET_OS_IPHONE
    if (alignment == ax::TextHAlignment::CENTER)
        alignment = ax::TextHAlignment::RIGHT;
    else if (alignment == ax::TextHAlignment::RIGHT)
        alignment = ax::TextHAlignment::CENTER;
    self.textInput.axui_alignment = static_cast<NSTextAlignment>(alignment);
}

- (void)setPlaceHolder:(const char*)text
{
    self.textInput.axui_placeholder = [NSString stringWithUTF8String:text];
}

- (void)setVisible:(BOOL)visible
{
    self.textInput.hidden = !visible;
}

- (void)setTextColor:(NSColor*)color
{
    self.textInput.axui_textColor = color;
}

- (void)setFont:(NSFont*)font
{
    if (font != nil)
    {
        self.textInput.axui_font = font;
    }
}

- (void)setPlaceholderFontColor:(NSColor*)color
{
    self.textInput.axui_placeholderColor = color;
}

- (void)setPlaceholderFont:(NSFont*)font
{
    self.textInput.axui_placeholderFont = font;
}

- (void)setText:(NSString*)text
{
    self.textInput.axui_text = text;
}

- (BOOL)textShouldBeginEditing:(NSText*)textObject  // YES means do it
{
    _editState = YES;

    getEditBoxImplMac()->editBoxEditingDidBegin();
    return YES;
}

- (void)textDidEndEditing:(NSNotification*)notification
{
    _editState = NO;

    getEditBoxImplMac()->editBoxEditingDidEnd([self getText], [self getEndAction:notification]);
}

- (ax::ui::EditBoxDelegate::EditBoxEndAction)getEndAction:(NSNotification*)notification
{
    auto type                  = ax::ui::EditBoxDelegate::EditBoxEndAction::UNKNOWN;
    NSUInteger reasonForEnding = [[[notification userInfo] objectForKey:@"NSTextMovement"] unsignedIntValue];
    if (reasonForEnding == NSTabTextMovement)
    {
        type = ax::ui::EditBoxDelegate::EditBoxEndAction::TAB_TO_NEXT;
    }
    else if (reasonForEnding == NSBacktabTextMovement)
    {
        type = ax::ui::EditBoxDelegate::EditBoxEndAction::TAB_TO_PREVIOUS;
    }
    else if (reasonForEnding == NSReturnTextMovement)
    {
        type = ax::ui::EditBoxDelegate::EditBoxEndAction::RETURN;
    }
    return type;
}

- (void)textDidChange:(NSNotification*)notification
{
    NSTextView* textView = notification.object;

    const char* inputText = [textView.string UTF8String];

    getEditBoxImplMac()->editBoxEditingChanged(inputText);
}

- (BOOL)textView:(NSTextView*)textView
    shouldChangeTextInRange:(NSRange)affectedCharRange
          replacementString:(NSString*)replacementString
{
    int maxLength = getEditBoxImplMac()->getMaxLength();
    if (maxLength < 0)
    {
        return YES;
    }

    if (affectedCharRange.length + affectedCharRange.location > textView.string.length)
    {
        return NO;
    }

    NSUInteger oldLength         = textView.string.length;
    NSUInteger replacementLength = replacementString.length;
    NSUInteger rangeLength       = affectedCharRange.length;

    NSUInteger newLength = oldLength - rangeLength + replacementLength;

    return newLength <= maxLength;
}

@end
