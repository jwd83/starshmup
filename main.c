#include <snes.h>

#include "gfx.h"
#include "scenes.h"
#include "sfx.h"

// Font from data.asm
extern char tilfont, palfont;

// Screen dimensions
#define SCREEN_W 256
#define SCREEN_H 224

// VRAM layout
// Text: tiles at 0x3000, map at 0x6800 (from consoleInitText)
// BG2 grid: tiles at 0x4000, map at 0x5000
// Sprites: tiles at 0x8000
#define BG2_TILE_BASE 0x4000
#define BG2_MAP_BASE  0x5000
#define SPR_TILE_BASE 0x8000

// BG palette #1 starts at color index 16 (CGRAM entry, not byte offset)
#define BG_PAL1_CGRAM_ENTRY 16

// Tilemap entry helper (4bpp BGs)
#define BG_MAP_PAL(p) ((u16)((p) & 0x7) << 10)

// Gameplay constants
#define MAX_BULLETS 8
#define PLAYER_SPEED 2
#define BULLET_SPEED 4
#define ENEMY_SPEED 1
#define AUTOFIRE_INTERVAL 6
#define BULLET_COLLISION_RADIUS 10
#define PLAYER_COLLISION_RADIUS 12
#define PLAYER_SIZE 16
#define ENEMY_SIZE 16
#define BULLET_SIZE 8

#define HUD_ROW 1
#define HUD_LEVEL_LABEL_X 1
#define HUD_LEVEL_VALUE_X 8
#define HUD_KILLS_LABEL_X 13
#define HUD_KILLS_VALUE_X 20

typedef struct Bullet {
    s16 x, y;
    s8 vx, vy;
    u8 active;
} Bullet;

static u16 g_rng = 0xACE1u;

// Galois LFSR (16-bit) random number generator
static u16 rng_next_u16(void) {
    u16 lsb = g_rng & 1u;
    g_rng >>= 1;
    if (lsb) {
        g_rng ^= 0xB400u;
    }
    return g_rng;
}

static s16 clamp_s16(s16 v, s16 lo, s16 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static s16 iabs_s16(s16 v) {
    return (v < 0) ? (s16)-v : v;
}

// Format u16 as decimal string. Buffer must hold at least 6 chars.
static void u16_to_dec(u16 v, char* out) {
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

// Build 32x32 tilemap: major lines every 4 tiles, minor elsewhere
static void build_starfield_map(u16* map32x32) {
    const u16 pal_bits = BG_MAP_PAL(1);
    u16 x, y;

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            // Deterministic "random" based on tile coordinates.
            // Roughly 1/16 tiles contain a star; star variants 1..4.
            u16 r = (u16)(x * 1103u) ^ (u16)(y * 2503u) ^ 0xA5A5u;
            r ^= (u16)((r >> 3) | (r << 5));

            if ((r & 0x0F) == 0) {
                u16 tile = (u16)(1 + ((r >> 4) & 3));  // 1..4
                map32x32[y * 32 + x] = tile | pal_bits;
            } else {
                map32x32[y * 32 + x] = 0 | pal_bits;
            }
        }
    }
}

static void init_grid_bg2(void) {
    static u16 map32x32[32 * 32];

    dmaCopyVram((u8*)g_gridTiles4bpp, BG2_TILE_BASE, g_gridTiles4bpp_len);
    dmaCopyCGram((u8*)g_gridPal16, BG_PAL1_CGRAM_ENTRY, g_gridPal16_len);

    build_starfield_map(map32x32);
    dmaCopyVram((u8*)map32x32, BG2_MAP_BASE, sizeof(map32x32));

    REG_BG2SC = (u8)(((BG2_MAP_BASE >> 10) & 0x3F) << 2);
    // Note: video mode and REG_TM set later after text init
}

// Sprite palette CGRAM addresses (sprite palettes start at CGRAM entry 128)
#define SPR_PAL0_CGRAM 128  // Player
#define SPR_PAL1_CGRAM 144  // Enemy
#define SPR_PAL2_CGRAM 160  // Bullet

static void init_sprites(void) {
    // Load sprite tiles
    dmaCopyVram((u8*)g_spriteTiles4bpp, SPR_TILE_BASE, g_spriteTiles4bpp_len);

    // Load separate palettes for each sprite type
    dmaCopyCGram((u8*)g_playerPal, SPR_PAL0_CGRAM, g_playerPal_len);
    dmaCopyCGram((u8*)g_enemyPal, SPR_PAL1_CGRAM, g_enemyPal_len);
    dmaCopyCGram((u8*)g_bulletPal, SPR_PAL2_CGRAM, g_bulletPal_len);

    // Set sprite size: Small=8x8, Large=16x16
    oamInitGfxAttr(SPR_TILE_BASE, OBJ_SIZE8_L16);
}

// Spawn enemy at random screen edge
static void spawn_enemy(s16* ex, s16* ey) {
    const u16 r = rng_next_u16();
    const u8 edge = r & 3;
    const s16 max_x = SCREEN_W - ENEMY_SIZE;
    const s16 max_y = SCREEN_H - ENEMY_SIZE;

    switch (edge) {
        case 0:  // top
            *ex = r % (max_x + 1);
            *ey = 0;
            break;
        case 1:  // bottom
            *ex = r % (max_x + 1);
            *ey = max_y;
            break;
        case 2:  // left
            *ex = 0;
            *ey = r % (max_y + 1);
            break;
        default:  // right
            *ex = max_x;
            *ey = r % (max_y + 1);
            break;
    }
}

static void clear_bullets(Bullet* bullets) {
    u8 i;
    for (i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
    }
}

static void reset_player(s16* px, s16* py) {
    *px = (SCREEN_W / 2) - (PLAYER_SIZE / 2);
    *py = (SCREEN_H / 2) - (PLAYER_SIZE / 2);
}

// Hide all sprites (used during title/gameover screens)
static void hide_all_sprites(void) {
    u8 i;
    // Player 4 + enemy 4 + bullets 8 => 16 total
    for (i = 0; i < 16; i++) {
        oamSetEx(i * 4, OBJ_SMALL, OBJ_HIDE);
    }
}

// Reset gameplay state for a new game
static void reset_gameplay(s16* px, s16* py, s16* ex, s16* ey,
                           Bullet* bullets, u16* kills, u16* level, u8* enemy_hp) {
    *kills = 0;
    *level = 1;
    reset_player(px, py);
    spawn_enemy(ex, ey);
    clear_bullets(bullets);
    *enemy_hp = *level;
}

int main(void) {
    s16 player_x, player_y;
    s16 enemy_x, enemy_y;
    Bullet bullets[MAX_BULLETS];
    s8 aim_dx = 0;
    s8 aim_dy = -1;  // Default aim: up
    u16 kills = 0;
    u16 level = 1;
    u16 prev_kills = 0xFFFF;
    u16 prev_level = 0xFFFF;
    u8 enemy_hp = 1;
    char text_buf[20];
    u16 frame = 0;
    u16 scroll_x = 0;
    u16 scroll_y = 0;
    u16 pad, prev_pad = 0;
    s8 move_dx, move_dy;
    u8 i, obj, off_screen;
    s16 dx, dy;
    s16 bullet_cx, bullet_cy, enemy_cx, enemy_cy;  // centers for collision
    Scene current_scene = SCENE_TITLE;
    Scene next_scene;
    GameStats stats;

    sfx_init();
    consoleInit();

    // Initialize text system (BG1) - tiles at 0x3000, map at 0x6800
    consoleSetTextMapPtr(0x6800);
    consoleSetTextGfxPtr(0x3000);
    consoleSetTextOffset(0x0100);
    // PVSnesLib expects palette size in bytes (16 colors * 2 bytes each).
    consoleInitText(0, 16 * 2, &tilfont, &palfont);

    // BG1 setup for text
    bgSetGfxPtr(0, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_32x32);

    // Initialize grid background (BG2) and sprites
    init_grid_bg2();
    init_sprites();

    // BG2: grid - tiles at 0x6000, map at 0x7000
    bgSetGfxPtr(1, BG2_TILE_BASE);
    bgSetMapPtr(1, BG2_MAP_BASE, SC_32x32);

    // Configure video mode - enable BG1 (text) and BG2 (grid)
    setMode(BG_MODE1, 0);
    bgSetEnable(0);
    bgSetEnable(1);

    // Ensure backdrop is black (color index 0).
    setPaletteColor(0, 0x0000);

    // Screen on
    setBrightness(0xF);

    // Start at title screen
    hide_all_sprites();
    scene_title_enter();

    while (1) {
        pad = padsCurrent(0);

        switch (current_scene) {
            case SCENE_TITLE:
                // Only trigger on button press, not hold
                if ((pad & KEY_START) && !(prev_pad & KEY_START)) {
                    next_scene = scene_title_update(pad);
                    if (next_scene == SCENE_GAMEPLAY) {
                        sfx_ui_confirm();
                        // Initialize gameplay
                        reset_gameplay(&player_x, &player_y, &enemy_x, &enemy_y,
                                       bullets, &kills, &level, &enemy_hp);
                        aim_dx = 0;
                        aim_dy = -1;
                        prev_kills = 0xFFFF;
                        prev_level = 0xFFFF;
                        frame = 0;
                        current_scene = SCENE_GAMEPLAY;
                    }
                }

                // Scroll starfield even on title
                scroll_x++;
                if ((frame & 3) == 0) scroll_y++;
                break;

            case SCENE_GAMEPLAY:
                // Read d-pad input
                move_dx = 0;
                move_dy = 0;
                if (pad & KEY_LEFT) move_dx = -1;
                else if (pad & KEY_RIGHT) move_dx = 1;
                if (pad & KEY_UP) move_dy = -1;
                else if (pad & KEY_DOWN) move_dy = 1;

                // Update aim direction when moving
                if (move_dx || move_dy) {
                    aim_dx = move_dx;
                    aim_dy = move_dy;
                }

                // Move player
                player_x += move_dx * PLAYER_SPEED;
                player_y += move_dy * PLAYER_SPEED;
                player_x = clamp_s16(player_x, 0, SCREEN_W - PLAYER_SIZE);
                player_y = clamp_s16(player_y, 0, SCREEN_H - PLAYER_SIZE);

                // Autofire
                if ((frame % AUTOFIRE_INTERVAL) == 0) {
                    for (i = 0; i < MAX_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].active = 1;
                            bullets[i].x = player_x + (PLAYER_SIZE / 2) - (BULLET_SIZE / 2);
                            bullets[i].y = player_y + (PLAYER_SIZE / 2) - (BULLET_SIZE / 2);
                            bullets[i].vx = aim_dx * BULLET_SPEED;
                            bullets[i].vy = aim_dy * BULLET_SPEED;
                            sfx_shot();
                            break;
                        }
                    }
                }

                // Update bullets
                for (i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) continue;

                    bullets[i].x += bullets[i].vx;
                    bullets[i].y += bullets[i].vy;

                    // Remove if off-screen
                    off_screen = bullets[i].x < -BULLET_SIZE ||
                                 bullets[i].x > SCREEN_W + BULLET_SIZE ||
                                 bullets[i].y < -BULLET_SIZE ||
                                 bullets[i].y > SCREEN_H + BULLET_SIZE;
                    if (off_screen) {
                        bullets[i].active = 0;
                    }
                }

                // Enemy homing toward player
                if (enemy_x < player_x) enemy_x += ENEMY_SPEED;
                else if (enemy_x > player_x) enemy_x -= ENEMY_SPEED;
                if (enemy_y < player_y) enemy_y += ENEMY_SPEED;
                else if (enemy_y > player_y) enemy_y -= ENEMY_SPEED;

                // Bullet-enemy collision (use sprite centers)
                enemy_cx = enemy_x + (ENEMY_SIZE / 2);
                enemy_cy = enemy_y + (ENEMY_SIZE / 2);
                for (i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) continue;

                    bullet_cx = bullets[i].x + (BULLET_SIZE / 2);
                    bullet_cy = bullets[i].y + (BULLET_SIZE / 2);
                    dx = iabs_s16(bullet_cx - enemy_cx);
                    dy = iabs_s16(bullet_cy - enemy_cy);
                    if (dx < BULLET_COLLISION_RADIUS && dy < BULLET_COLLISION_RADIUS) {
                        u16 old_level;

                        bullets[i].active = 0;
                        old_level = level;
                        enemy_hp--;
                        if (enemy_hp == 0) {
                            sfx_enemy_down();
                            kills++;
                            // Level up every 10 kills
                            level = 1 + (kills / 10);
                            if (level > old_level) {
                                sfx_level_up();
                            }
                            spawn_enemy(&enemy_x, &enemy_y);
                            enemy_hp = level;
                        } else {
                            sfx_enemy_hit();
                        }
                        break;
                    }
                }

                // Player-enemy collision: game over (contact / overlap)
                if (player_x < (enemy_x + ENEMY_SIZE) &&
                    (player_x + PLAYER_SIZE) > enemy_x &&
                    player_y < (enemy_y + ENEMY_SIZE) &&
                    (player_y + PLAYER_SIZE) > enemy_y) {
                    // Transition to game over
                    sfx_player_down();
                    stats.kills = kills;
                    stats.level = level;
                    hide_all_sprites();
                    // Clear HUD
                    consoleDrawText(HUD_LEVEL_LABEL_X, HUD_ROW, "              ");
                    consoleDrawText(HUD_KILLS_LABEL_X, HUD_ROW, "              ");
                    scene_gameover_enter(&stats);
                    current_scene = SCENE_GAMEOVER;
                    break;
                }

                // Scroll starfield
                scroll_x++;
                if ((frame & 3) == 0) scroll_y++;

                // Draw player metasprite (4x 8x8 => 16x16, tiles 0-3, palette 0)
                oamSet(0,  player_x,      player_y,      2, 0, 0, 0, 0);
                oamSet(4,  player_x + 8,  player_y,      2, 0, 0, 1, 0);
                oamSet(8,  player_x,      player_y + 8,  2, 0, 0, 2, 0);
                oamSet(12, player_x + 8,  player_y + 8,  2, 0, 0, 3, 0);
                oamSetEx(0,  OBJ_SMALL, OBJ_SHOW);
                oamSetEx(4,  OBJ_SMALL, OBJ_SHOW);
                oamSetEx(8,  OBJ_SMALL, OBJ_SHOW);
                oamSetEx(12, OBJ_SMALL, OBJ_SHOW);

                // Draw enemy metasprite (4x 8x8 => 16x16, tiles 4-7, palette 1)
                oamSet(16, enemy_x,      enemy_y,      2, 0, 0, 4, 1);
                oamSet(20, enemy_x + 8,  enemy_y,      2, 0, 0, 5, 1);
                oamSet(24, enemy_x,      enemy_y + 8,  2, 0, 0, 6, 1);
                oamSet(28, enemy_x + 8,  enemy_y + 8,  2, 0, 0, 7, 1);
                oamSetEx(16, OBJ_SMALL, OBJ_SHOW);
                oamSetEx(20, OBJ_SMALL, OBJ_SHOW);
                oamSetEx(24, OBJ_SMALL, OBJ_SHOW);
                oamSetEx(28, OBJ_SMALL, OBJ_SHOW);

                // Draw bullets (OAM slots 8-15, tile 8, 8x8, palette 2)
                for (i = 0; i < MAX_BULLETS; i++) {
                    obj = (8 + i) * 4;
                    if (bullets[i].active) {
                        oamSet(obj, bullets[i].x, bullets[i].y, 2, 0, 0, 8, 2);
                        oamSetEx(obj, OBJ_SMALL, OBJ_SHOW);
                    } else {
                        oamSetEx(obj, OBJ_SMALL, OBJ_HIDE);
                    }
                }

                // Update HUD only when values change
                if (level != prev_level) {
                    prev_level = level;
                    u16_to_dec(level, text_buf);
                    consoleDrawText(HUD_LEVEL_LABEL_X, HUD_ROW, "LEVEL:");
                    consoleDrawText(HUD_LEVEL_VALUE_X, HUD_ROW, "     ");
                    consoleDrawText(HUD_LEVEL_VALUE_X, HUD_ROW, text_buf);
                }
                if (kills != prev_kills) {
                    prev_kills = kills;
                    u16_to_dec(kills, text_buf);
                    consoleDrawText(HUD_KILLS_LABEL_X, HUD_ROW, "KILLS:");
                    consoleDrawText(HUD_KILLS_VALUE_X, HUD_ROW, "     ");
                    consoleDrawText(HUD_KILLS_VALUE_X, HUD_ROW, text_buf);
                }
                break;

            case SCENE_GAMEOVER:
                // Only trigger on button press, not hold
                if ((pad & KEY_START) && !(prev_pad & KEY_START)) {
                    next_scene = scene_gameover_update(pad);
                    if (next_scene == SCENE_TITLE) {
                        sfx_ui_confirm();
                        scene_title_enter();
                        current_scene = SCENE_TITLE;
                    }
                }

                // Scroll starfield even on game over
                scroll_x++;
                if ((frame & 3) == 0) scroll_y++;
                break;
        }

        sfx_process();
        WaitForVBlank();

        // Apply scroll during VBlank to avoid tearing
        REG_BG2HOFS = scroll_x & 0xFF;
        REG_BG2HOFS = (scroll_x >> 8) & 0xFF;
        REG_BG2VOFS = scroll_y & 0xFF;
        REG_BG2VOFS = (scroll_y >> 8) & 0xFF;

        oamUpdate();
        prev_pad = pad;
        frame++;
    }

    return 0;
}
