#pragma once

#include <snes.h>

// Game scenes
typedef enum {
    SCENE_TITLE,
    SCENE_GAMEPLAY,
    SCENE_GAMEOVER
} Scene;

// Shared game state (accessible across scenes)
typedef struct {
    u16 kills;
    u16 level;
} GameStats;

// Scene functions
void scene_title_enter(void);
Scene scene_title_update(u16 pad);

void scene_gameover_enter(const GameStats* stats);
Scene scene_gameover_update(u16 pad);
