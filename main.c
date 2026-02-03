#include <snes.h>

#include "gfx.h"

// Screen dimensions
#define SCREEN_W 256
#define SCREEN_H 224

// VRAM layout (chosen to avoid common console/font locations)
#define BG2_TILE_BASE 0x6000
#define BG2_MAP_BASE 0x7000
#define SPR_TILE_BASE 0x4000

// BG palette #1 starts at color index 16
#define BG_PAL1_CGRAM_BYTE_OFFSET (16 * 2)

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
#define SPRITE_SIZE 16
#define BULLET_SIZE 8

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
static void build_grid_map(u16* map32x32) {
    const u16 pal_bits = BG_MAP_PAL(1);
    u16 x, y;
    u8 is_major;

    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            is_major = ((x & 3) == 0) || ((y & 3) == 0);
            map32x32[y * 32 + x] = (is_major ? 1 : 0) | pal_bits;
        }
    }
}

static void init_grid_bg2(void) {
    static u16 map32x32[32 * 32];

    dmaCopyVram((u8*)g_gridTiles4bpp, BG2_TILE_BASE, g_gridTiles4bpp_len);
    dmaCopyCGram((u8*)g_gridPal16, BG_PAL1_CGRAM_BYTE_OFFSET, g_gridPal16_len);

    build_grid_map(map32x32);
    dmaCopyVram((u8*)map32x32, BG2_MAP_BASE, sizeof(map32x32));

    REG_BGMODE = 1;  // Mode 1: BG1/BG2 4bpp, BG3 2bpp
    REG_BG12NBA = (u8)(((BG2_TILE_BASE / 0x1000) & 0x0F) << 4);
    REG_BG2SC = (u8)(((BG2_MAP_BASE >> 10) & 0x3F) << 2);
    REG_TM = 0x16;  // Enable BG2 + BG3 + OBJ
}

static void init_sprites(void) {
    // Small=8x8, Large=16x16
    oamInitGfxSet((u8*)g_spriteTiles4bpp,
                  g_spriteTiles4bpp_len,
                  (u8*)g_spritePal,
                  g_spritePal_len,
                  0,
                  SPR_TILE_BASE,
                  OBJ_SIZE8_L16);
}

// Spawn enemy at random screen edge
static void spawn_enemy(s16* ex, s16* ey) {
    const u16 r = rng_next_u16();
    const u8 edge = r & 3;
    const s16 max_x = SCREEN_W - SPRITE_SIZE;
    const s16 max_y = SCREEN_H - SPRITE_SIZE;

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
    *px = (SCREEN_W / 2) - (SPRITE_SIZE / 2);
    *py = (SCREEN_H / 2) - (SPRITE_SIZE / 2);
}

int main(void) {
    s16 player_x, player_y;
    s16 enemy_x, enemy_y;
    Bullet bullets[MAX_BULLETS];
    s8 aim_dx = 0;
    s8 aim_dy = -1;  // Default aim: up
    u16 score = 0;
    char score_buf[16];
    u16 frame = 0;
    u16 scroll_x = 0;
    u16 scroll_y = 0;
    u16 pad;
    s8 move_dx, move_dy;
    u8 i, obj, off_screen;
    s16 dx, dy;

    consoleInit();
    REG_INIDISP = 0x80;  // Force blank during setup
    init_grid_bg2();
    init_sprites();
    oamInitGfxAttr(SPR_TILE_BASE, OBJ_SIZE8_L16);
    REG_INIDISP = 0x0F;  // Screen on, max brightness

    consoleDrawText(2, 1, "starshmup");
    consoleDrawText(2, 2, "dpad move, autofire");

    reset_player(&player_x, &player_y);
    spawn_enemy(&enemy_x, &enemy_y);
    clear_bullets(bullets);

    while (1) {
        WaitForVBlank();
        pad = padsCurrent(0);

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
        player_x = clamp_s16(player_x, 0, SCREEN_W - SPRITE_SIZE);
        player_y = clamp_s16(player_y, 0, SCREEN_H - SPRITE_SIZE);

        // Autofire
        if ((frame % AUTOFIRE_INTERVAL) == 0) {
            for (i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = 1;
                    bullets[i].x = player_x + (SPRITE_SIZE / 2) - (BULLET_SIZE / 2);
                    bullets[i].y = player_y + (SPRITE_SIZE / 2) - (BULLET_SIZE / 2);
                    bullets[i].vx = aim_dx * BULLET_SPEED;
                    bullets[i].vy = aim_dy * BULLET_SPEED;
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

        // Bullet-enemy collision
        for (i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;

            dx = iabs_s16(bullets[i].x - enemy_x);
            dy = iabs_s16(bullets[i].y - enemy_y);
            if (dx < BULLET_COLLISION_RADIUS && dy < BULLET_COLLISION_RADIUS) {
                bullets[i].active = 0;
                score++;
                spawn_enemy(&enemy_x, &enemy_y);
                break;
            }
        }

        // Player-enemy collision: reset game
        dx = iabs_s16(player_x - enemy_x);
        dy = iabs_s16(player_y - enemy_y);
        if (dx < PLAYER_COLLISION_RADIUS && dy < PLAYER_COLLISION_RADIUS) {
            score = 0;
            reset_player(&player_x, &player_y);
            spawn_enemy(&enemy_x, &enemy_y);
            clear_bullets(bullets);
        }

        // Update score display
        u16_to_dec(score, score_buf);
        consoleDrawText(2, 4, "SCORE:          ");
        consoleDrawText(9, 4, score_buf);

        // Scroll background grid
        scroll_x++;
        if ((frame & 3) == 0) scroll_y++;

        REG_BG2HOFS = scroll_x & 0xFF;
        REG_BG2HOFS = (scroll_x >> 8) & 0xFF;
        REG_BG2VOFS = scroll_y & 0xFF;
        REG_BG2VOFS = (scroll_y >> 8) & 0xFF;

        // Draw player (OAM slot 0, tiles 0-3, 16x16)
        // OAM IDs are byte offsets: slot N = N * 4
        oamSet(0, player_x, player_y, 2, 0, 0, 0, 0);
        oamSetEx(0, OBJ_LARGE, OBJ_SHOW);

        // Draw enemy (OAM slot 1, tiles 4-7, 16x16)
        oamSet(4, enemy_x, enemy_y, 2, 0, 0, 4, 0);
        oamSetEx(4, OBJ_LARGE, OBJ_SHOW);

        // Draw bullets (OAM slots 2-9, tile 8, 8x8)
        for (i = 0; i < MAX_BULLETS; i++) {
            obj = (2 + i) * 4;  // Convert slot to byte offset
            if (bullets[i].active) {
                oamSet(obj, bullets[i].x, bullets[i].y, 1, 0, 0, 8, 0);
                oamSetEx(obj, OBJ_SMALL, OBJ_SHOW);
            } else {
                oamSetEx(obj, OBJ_SMALL, OBJ_HIDE);
            }
        }

        oamUpdate();
        frame++;
    }

    return 0;
}
