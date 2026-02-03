#include <snes.h>
#include "scenes.h"

#define GAMEOVER_ROW 6
#define LEVEL_ROW 10
#define KILLS_ROW 12
#define PROMPT_ROW 16

// Local copy of stats for display
static GameStats s_stats;

// Format u16 as decimal string. Buffer must hold at least 6 chars.
static void format_number(u16 v, char* out) {
    char tmp[6];
    u8 n = 0;
    u8 i;

    if (v == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    while (v > 0) {
        tmp[n++] = '0' + (v % 10);
        v /= 10;
    }

    for (i = 0; i < n; i++) {
        out[i] = tmp[n - 1 - i];
    }
    out[n] = '\0';
}

void scene_gameover_enter(const GameStats* stats) {
    char buf[12];

    s_stats = *stats;

    // Display game over screen
    consoleDrawText(11, GAMEOVER_ROW, "GAME OVER");

    consoleDrawText(10, LEVEL_ROW, "LEVEL: ");
    format_number(s_stats.level, buf);
    consoleDrawText(17, LEVEL_ROW, buf);

    consoleDrawText(10, KILLS_ROW, "KILLS: ");
    format_number(s_stats.kills, buf);
    consoleDrawText(17, KILLS_ROW, buf);

    consoleDrawText(10, PROMPT_ROW, "PRESS START");
}

Scene scene_gameover_update(u16 pad) {
    if (pad & KEY_START) {
        // Clear game over text before transitioning
        consoleDrawText(11, GAMEOVER_ROW, "         ");
        consoleDrawText(10, LEVEL_ROW, "            ");
        consoleDrawText(10, KILLS_ROW, "            ");
        consoleDrawText(10, PROMPT_ROW, "           ");
        return SCENE_TITLE;
    }
    return SCENE_GAMEOVER;
}
