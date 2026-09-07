#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include "ec11.h"

// Process a single EC11 event
void ui_controller_handle_event(EC11_Event ev);

// Draw the UI based on current state
void ui_controller_draw(void);

#endif // UI_CONTROLLER_H
