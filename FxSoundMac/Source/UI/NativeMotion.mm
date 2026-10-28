// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#import <AppKit/AppKit.h>
#include "NativeMotion.h"

bool systemPrefersReducedMotion()
{
    return NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceMotion;
}
