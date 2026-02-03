#include <snes.h>
#include "scenes.h"

#define TITLE_ROW 8
#define PROMPT_ROW 14

void scene_title_enter(void) {
    // Clear any previous text and display title
    consoleDrawText(11, TITLE_ROW, "STARSHMUP");
    consoleDrawText(10, PROMPT_ROW, "PRESS START");
}

Scene scene_title_update(u16 pad) {
    if (pad & KEY_START) {
        // Clear title text before transitioning
        consoleDrawText(11, TITLE_ROW, "         ");
        consoleDrawText(10, PROMPT_ROW, "           ");
        return SCENE_GAMEPLAY;
    }
    return SCENE_TITLE;
}
