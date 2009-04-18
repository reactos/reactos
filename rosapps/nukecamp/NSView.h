#import <Cocoa/Cocoa.h>

@interface NSView : NSResponder {
    IBOutlet NSView *nextKeyView;
}
- (IBAction)fax:(id)sender;
- (IBAction)print:(id)sender;
@end
