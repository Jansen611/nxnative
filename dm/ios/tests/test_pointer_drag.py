import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest


class PointerDragTests(unittest.TestCase):
    # Compile the same state machine used by UIKit without requiring an iPad.
    def test_gestures(self):
        compiler = shutil.which('clang')
        if not compiler:
            self.skipTest('clang is required')
        sourceDirectory = pathlib.Path(__file__).resolve().parents[1] / 'Sources'
        source = r'''
#include <assert.h>
#include "DmPointerDrag.h"

int main(void) {
    DmPointerDrag state = {0};
    int32_t rightX, rightY;
    assert(!PointerDragEnd(&state, false));
    PointerDragMove(&state, 100, 100);
    assert(!state.dragging);

    PointerDragBegin(&state, 20, 30, 0);
    PointerDragMove(&state, 22, 32);
    PointerDragSample(&state, 0.02, &rightX, &rightY);
    assert(rightX == 0 && rightY == 0);
    assert(PointerDragEnd(&state, false));
    assert(!PointerDragEnd(&state, false));

    PointerDragBegin(&state, 0, 0, 0);
    PointerDragMove(&state, 5, 0);
    assert(state.dragging);
    PointerDragSample(&state, 0.02, &rightX, &rightY);
    assert(rightX > 0 && rightY == 0);
    PointerDragSample(&state, 0.04, &rightX, &rightY);
    assert(rightX == 0 && rightY == 0);
    PointerDragMove(&state, 0, -8);
    PointerDragSample(&state, 0.06, &rightX, &rightY);
    assert(rightX < 0 && rightY > 0);
    assert(!PointerDragEnd(&state, false));

    PointerDragBegin(&state, 0, 0, 0);
    PointerDragMove(&state, 8, 0);
    PointerDragSample(&state, 0.16, &rightX, &rightY);
    assert(rightX == 32767 && rightY == 0);
    PointerDragMove(&state, 10008, 10000);
    PointerDragSample(&state, 0.18, &rightX, &rightY);
    assert(rightX > 0 && rightY < 0);
    assert(hypot(rightX, rightY) <= 32768);
    assert(!PointerDragEnd(&state, false));

    PointerDragBegin(&state, 0, 0, 0);
    PointerDragMove(&state, 6, 0);
    PointerDragMove(&state, 12, 0);
    PointerDragSample(&state, 0.32, &rightX, &rightY);
    assert(rightX == (int32_t)lrint(0.75 * 32767));
    PointerDragSample(&state, 0.32, &rightX, &rightY);
    assert(rightX == 0 && rightY == 0);
    PointerDragReset(&state);
    PointerDragSample(&state, 0.34, &rightX, &rightY);
    assert(rightX == 0 && rightY == 0 && !state.active);

    PointerDragBegin(&state, 0, 0, 0);
    PointerDragMove(&state, 0, -6);
    PointerDragSample(&state, 0.16, &rightX, &rightY);
    assert(rightX == 0 && rightY == (int32_t)lrint(0.75 * 32767));
    assert(!PointerDragEnd(&state, false));

    PointerDragBegin(&state, 0, 0, 0);
    assert(!PointerDragEnd(&state, true));
    PointerDragBegin(&state, 0, 0, 0);
    PointerDragMove(&state, 8, 0);
    PointerDragMove(&state, 0, 0);
    assert(!PointerDragEnd(&state, false));
    PointerDragBegin(&state, 0, 0, 0);
    PointerDragMove(&state, 8, 0);
    PointerDragBegin(&state, 40, 40, 1);
    PointerDragSample(&state, 1.02, &rightX, &rightY);
    assert(rightX == 0 && rightY == 0 && !state.dragging);
    assert(PointerDragEnd(&state, false));
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='pointer drag ') as temporary:
            executable = pathlib.Path(temporary) / 'pointer-drag-check'
            subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(sourceDirectory),
                            '-x', 'c', '-', '-o', str(executable), '-lm'], input=source, text=True, check=True)
            subprocess.run([str(executable)], check=True)

    # Exercise production touch routing with Foundation doubles, not a second implementation.
    @unittest.skipUnless(sys.platform == 'darwin', 'Foundation is required')
    def test_touch_routing(self):
        compiler = shutil.which('clang')
        if not compiler:
            self.skipTest('clang is required')
        sourceDirectory = pathlib.Path(__file__).resolve().parents[1] / 'Sources'
        implementation = (sourceDirectory / 'DmGamepad.m').read_text()
        publish = implementation[implementation.index('static pthread_mutex_t g_mu'):implementation.index('// Map physical key')]
        pointer = implementation[implementation.index('// Keep indirect clicks'):implementation.index('- (void)recompute')]
        touches = implementation[implementation.index('- (void)pushGame {'):implementation.index('// Removing the overlay')]
        source = r'''
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include <assert.h>
#include <pthread.h>
#include "DmPointerDrag.h"

enum { UIApplicationStateActive = 0 };
enum { UISceneActivationStateForegroundActive = 0 };
enum { UITouchTypeDirect, UITouchTypeIndirectPointer };
enum { UIEventButtonMaskPrimary = 1, UIEventButtonMaskSecondary = 2 };
enum { C_NONE = 0, C_STICK, C_DPAD, C_FACE, C_BTN, C_SWAP, C_GAME };
static NSString *const UIWindowDidResignKeyNotification = @"window-resign";
static NSString *const UISceneWillDeactivateNotification = @"scene-deactivate";
static double testTime;
static double CACurrentMediaTime(void) { return testTime; }
static double dist(CGPoint first, CGPoint second) { return hypot(first.x - second.x, first.y - second.y); }
static const double K_STICK_GRAB = 1.4;
static const double K_DPAD_GRAB = 1.3;
typedef struct { uint32_t finger; float x, y; } dm_touch;
static dm_touch reportedTouches[16];
static int reportedCount;
static int32_t reportedRightX, reportedRightY;
static uint64_t reportedButtons;
static void dm_input_set(uint64_t buttons, int32_t leftX, int32_t leftY, int32_t rightX, int32_t rightY) {
    (void)leftX; (void)leftY;
    reportedButtons = buttons; reportedRightX = rightX; reportedRightY = rightY;
}
static void dm_touch_set(const dm_touch *touches, int count) {
    assert(count >= 0 && count <= 16);
    reportedCount = count;
    if (count) memcpy(reportedTouches, touches, sizeof(*touches) * count);
}
@interface NSValue (PointerTest)
+ (instancetype)valueWithCGPoint:(CGPoint)point;
- (CGPoint)CGPointValue;
@end
@implementation NSValue (PointerTest)
+ (instancetype)valueWithCGPoint:(CGPoint)point { return [self value:&point withObjCType:@encode(CGPoint)]; }
- (CGPoint)CGPointValue { CGPoint point; [self getValue:&point size:sizeof(point)]; return point; }
@end
@interface UIWindowScene : NSObject
@property int activationState;
@end
@implementation UIWindowScene
@end
@interface UIViewController : NSObject
@property UIViewController *presentedViewController;
@end
@implementation UIViewController
@end
@interface UIWindow : NSObject
@property BOOL isKeyWindow;
@property UIWindowScene *windowScene;
@property UIViewController *rootViewController;
@end
@implementation UIWindow
@end
@interface UIApplication : NSObject
@property int applicationState;
+ (instancetype)sharedApplication;
@end
@implementation UIApplication
+ (instancetype)sharedApplication { static UIApplication *application; if (!application) application = [self new]; return application; }
@end
@interface UIView : NSObject
@property CGRect bounds;
@property UIWindow *window;
@end
@implementation UIView
@end
@interface UITouch : NSObject
@property int type;
@property CGPoint position;
- (CGPoint)locationInView:(UIView *)view;
@end
@implementation UITouch
- (CGPoint)locationInView:(UIView *)view { (void)view; return self.position; }
@end
@interface UIEvent : NSObject
@property NSUInteger buttonMask;
@end
@implementation UIEvent
@end

PUBLISH_METHODS

@interface DmGamepad : UIView {
@public
    UITouch *_pointerTouch, *_game[16], *_stick[2];
    DmPointerDrag _pointerDrag;
    NSTimer *_pointerTimer;
    NSMutableArray<NSValue *> *_pointerClicks;
    NSInteger _pointerClickSlot;
    CGPoint _pointerClickPosition, _stickNow[2], _rCentre;
    CFTimeInterval _pointerClickUntil, _pointerClickGapUntil;
    BOOL _keyboardEditing, _padOff, _tap, _full;
    NSMapTable<UITouch *, NSNumber *> *_owner, *_faceOf;
}
- (void)pushGame;
- (void)recompute;
- (int)btnAt:(CGPoint)point;
- (BOOL)inSwap:(CGPoint)point;
- (CGPoint)faceCenter;
- (CGFloat)faceGap;
- (CGFloat)faceR;
- (CGPoint)dpadCenter;
- (CGFloat)dpadR;
- (CGPoint)stickHome;
- (CGFloat)stickR:(int)index;
- (BOOL)inRightArea:(CGPoint)point;
- (uint64_t)faceAt:(CGPoint)point;
- (void)nextLayout;
@end
@implementation DmGamepad
- (instancetype)init {
    if ((self = [super init])) {
        _owner = [NSMapTable strongToStrongObjectsMapTable]; _faceOf = [NSMapTable strongToStrongObjectsMapTable];
        _pointerClicks = [NSMutableArray array]; _pointerClickSlot = -1; _padOff = YES;
        self.bounds = CGRectMake(0, 0, 1024, 768);
        self.window = [UIWindow new]; self.window.isKeyWindow = YES;
        self.window.windowScene = [UIWindowScene new]; self.window.rootViewController = [UIViewController new];
    }
    return self;
}
- (void)recompute {}
- (int)btnAt:(CGPoint)point { (void)point; return 0; }
- (BOOL)inSwap:(CGPoint)point { (void)point; return NO; }
- (CGPoint)faceCenter { return CGPointZero; }
- (CGFloat)faceGap { return 1; }
- (CGFloat)faceR { return 1; }
- (CGPoint)dpadCenter { return CGPointZero; }
- (CGFloat)dpadR { return 1; }
- (CGPoint)stickHome { return CGPointZero; }
- (CGFloat)stickR:(int)index { (void)index; return 1; }
- (BOOL)inRightArea:(CGPoint)point { (void)point; return NO; }
- (uint64_t)faceAt:(CGPoint)point { (void)point; return 0; }
- (void)nextLayout {}
POINTER_METHODS
TOUCH_METHODS
@end

static UITouch *MakeTouch(int type, double positionX, double positionY) {
    UITouch *touch = [UITouch new]; touch.type = type; touch.position = CGPointMake(positionX, positionY); return touch;
}
static void Click(DmGamepad *view, UIEvent *event, double positionX, double positionY) {
    UITouch *touch = MakeTouch(UITouchTypeIndirectPointer, positionX, positionY);
    [view touchesBegan:[NSSet setWithObject:touch] withEvent:event];
    [view touchesEnded:[NSSet setWithObject:touch] withEvent:event];
}
static void Tick(DmGamepad *view, double timestamp) { testTime = timestamp; [view PointerTick:view->_pointerTimer]; }
int main(void) {
    @autoreleasepool {
        DmGamepad *view = [DmGamepad new];
        UIEvent *event = [UIEvent new]; event.buttonMask = UIEventButtonMaskSecondary;
        Click(view, event, 512, 384);
        assert(!view->_pointerTouch && !view->_pointerClicks.count && !view->_pointerTimer);
        event.buttonMask = UIEventButtonMaskPrimary;
        view->_padOff = NO;
        Click(view, event, 512, 384);
        assert(view->_pointerClicks.count == 1 && view->_owner.count == 0);
        Tick(view, 0.02);
        assert(reportedCount == 1 && reportedTouches[0].finger == 0);
        assert(fabs(reportedTouches[0].x - 0.5) < 0.001 && fabs(reportedTouches[0].y - 0.5) < 0.001);
        Tick(view, 0.12);
        assert(reportedCount == 0 && !view->_pointerTimer);
        view->_padOff = YES;

        UITouch *pointer = MakeTouch(UITouchTypeIndirectPointer, 512, 384);
        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        assert(reportedCount == 0);
        pointer.position = CGPointMake(532, 374);
        [view touchesMoved:[NSSet setWithObject:pointer] withEvent:event];
        g_keyboard_btn = 1; g_touch_rx = -12000;
        Tick(view, 0.14);
        assert(reportedRightX > 0 && reportedRightY > 0 && reportedButtons == 1 && reportedCount == 0);
        Tick(view, 0.16);
        assert(reportedRightX == -12000 && reportedRightY == 0 && reportedButtons == 1);
        [view touchesEnded:[NSSet setWithObject:pointer] withEvent:event];
        assert(!view->_pointerClicks.count && g_pointer_rx == 0 && g_pointer_ry == 0);
        assert(g_touch_rx == -12000 && g_keyboard_btn == 1);
        g_touch_rx = 0; g_keyboard_btn = 0;
        [view reset];

        Click(view, event, 512, 384); Tick(view, 0.20);
        UITouch *finger = MakeTouch(UITouchTypeDirect, 256, 384);
        [view touchesBegan:[NSSet setWithObject:finger] withEvent:event];
        assert(reportedCount == 2 && view->_game[1] == finger && view->_pointerClickSlot == 0);
        finger.position = CGPointMake(768, 384);
        [view touchesMoved:[NSSet setWithObject:finger] withEvent:event];
        assert(reportedCount == 2 && fabs(reportedTouches[0].x - 0.75) < 0.001);
        Tick(view, 0.30);
        assert(reportedCount == 1 && reportedTouches[0].finger == 1 && view->_game[1] == finger);
        [view touchesEnded:[NSSet setWithObject:finger] withEvent:event];
        assert(reportedCount == 0);

        NSMutableArray *fingers = [NSMutableArray array];
        for (int slot = 0; slot < 16; slot++) {
            UITouch *touch = MakeTouch(UITouchTypeDirect, 512, 384); [fingers addObject:touch];
            [view touchesBegan:[NSSet setWithObject:touch] withEvent:event];
        }
        Click(view, event, 512, 384); Tick(view, 0.32);
        assert(reportedCount == 16 && view->_pointerClicks.count == 1 && view->_pointerClickSlot == -1);
        [view touchesEnded:[NSSet setWithObject:fingers[4]] withEvent:event]; Tick(view, 0.34);
        assert(reportedCount == 16 && view->_pointerClickSlot == 4 && !view->_pointerClicks.count);
        [view reset]; assert(reportedCount == 0 && !view->_pointerTimer);

        Click(view, event, 256, 384); Click(view, event, 768, 384);
        Tick(view, 0.40); assert(reportedCount == 1 && view->_pointerClicks.count == 1);
        assert(fabs(reportedTouches[0].x - 0.25) < 0.001);
        Tick(view, 0.50); assert(reportedCount == 0 && view->_pointerClicks.count == 1);
        Tick(view, 0.52); assert(reportedCount == 1 && !view->_pointerClicks.count);
        assert(fabs(reportedTouches[0].x - 0.75) < 0.001);
        Tick(view, 0.62); assert(reportedCount == 0 && !view->_pointerTimer);
        Click(view, event, 512, 10); assert(!view->_pointerClicks.count);
        [view reset];

        pointer = MakeTouch(UITouchTypeIndirectPointer, 512, 384);
        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        [view touchesCancelled:[NSSet setWithObject:pointer] withEvent:event];
        assert(!view->_pointerClicks.count && !view->_pointerTouch);
        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        pointer.position = CGPointMake(540, 384);
        [view touchesMoved:[NSSet setWithObject:pointer] withEvent:event]; Tick(view, 0.64);
        assert(g_pointer_rx > 0);
        view.window.isKeyWindow = NO; Tick(view, 0.66);
        assert(!view->_pointerTouch && !view->_pointerTimer && g_pointer_rx == 0 && reportedCount == 0);
        view.window.isKeyWindow = YES;
        [view touchesMoved:[NSSet setWithObject:pointer] withEvent:event];
        [view touchesEnded:[NSSet setWithObject:pointer] withEvent:event];
        assert(!view->_pointerClicks.count && g_pointer_rx == 0);

        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        view.window.rootViewController.presentedViewController = [UIViewController new];
        [view touchesEnded:[NSSet setWithObject:pointer] withEvent:event];
        assert(!view->_pointerClicks.count);
        view.window.rootViewController.presentedViewController = nil; [view reset];
        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        [view PointerInterrupted:[NSNotification notificationWithName:UIWindowDidResignKeyNotification object:[UIWindow new]]];
        assert(view->_pointerTouch == pointer);
        [view PointerInterrupted:[NSNotification notificationWithName:UIWindowDidResignKeyNotification object:view.window]];
        assert(!view->_pointerTouch && !view->_pointerTimer);
        [view touchesEnded:[NSSet setWithObject:pointer] withEvent:event];
        assert(!view->_pointerClicks.count);

        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        view->_keyboardEditing = YES; Tick(view, 0.68);
        assert(!view->_pointerTouch && !view->_pointerTimer);
        view->_keyboardEditing = NO;
        event.buttonMask = UIEventButtonMaskPrimary;
        [view touchesBegan:[NSSet setWithObject:pointer] withEvent:event];
        event.buttonMask = UIEventButtonMaskSecondary;
        [view touchesMoved:[NSSet setWithObject:pointer] withEvent:event];
        [view touchesEnded:[NSSet setWithObject:pointer] withEvent:event];
        assert(!view->_pointerClicks.count && !view->_pointerTouch && !view->_pointerTimer);
        [view reset];
    }
    return 0;
}
'''
        source = source.replace('PUBLISH_METHODS', publish).replace('POINTER_METHODS', pointer).replace('TOUCH_METHODS', touches)
        with tempfile.TemporaryDirectory(prefix='pointer routing ') as temporary:
            executable = pathlib.Path(temporary) / 'pointer-routing-check'
            subprocess.run([compiler, '-fobjc-arc', '-Wall', '-Wextra', '-Werror', '-I', str(sourceDirectory),
                            '-x', 'objective-c', '-', '-framework', 'Foundation', '-framework', 'CoreGraphics',
                            '-o', str(executable)], input=source, text=True, check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == '__main__':
    unittest.main()