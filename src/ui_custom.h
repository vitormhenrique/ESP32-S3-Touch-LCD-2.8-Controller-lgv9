#ifndef UI_CUSTOM_H
#define UI_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

// Redirect to the new modular UI library
#include "ui/ui.h"

// Define legacy compatibility macros/prototypes if needed to support existing integration code
// For now, we'll try to keep the same function signatures in the new files or provide mapping here.

// Map legacy function names to new modular functions if they differ

// The public API in ui.h and screen specific headers should match what src/ui_custom_integration.cpp expects.
// Let's check src/ui_custom.h to ensure we expose everything.

void ui_custom_init(void);
void ui_custom_destroy(void);
void ui_custom_update(void);

#ifdef __cplusplus
}
#endif

#endif // UI_CUSTOM_H
