#include <snes.h>

#include "gfx.h"

#define SCREEN_W 256
#define SCREEN_H 224

// VRAM layout (chosen to avoid common console/font locations).
#define BG2_TILE_BASE 0x6000
#define BG2_MAP_BASE 0x7000
#define SPR_TILE_BASE 0x4000

// BG palette #1 starts at color index 16.
#define BG_PAL1_CGRAM_BYTE_OFFSET (16 * 2)

// Tilemap entry helpers (4bpp BGs).
#define BG_MAP_PAL(p) ((u16)((p) & 0x7) << 10)

typedef struct Bullet {
    s16 x, y;
    s8 vx, vy;
    u8 active;
} Bullet;

static u16 g_rng = 0xACE1u;

static u16 rng_next_u16(void) {
    // Galois LFSR (16-bit)
    u16 lsb = (u16)(g_rng & 1u);
    g_rng >>= 1;
    if (lsb) {
        g_rng ^= 0xB400u;
    }
    return g_rng;
}

static s16 clamp_s16(s16 v, s16 lo, s16 hi) {
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static s16 iabs_s16(s16 v) { return (v < 0) ? (s16)-v : v; }

static void u16_to_dec(u16 v, char* out, u8 out_cap) {
    // Writes null-terminated decimal. out_cap must be >= 2.
    char tmp[6];
    u8 n = 0;
    if (out_cap < 2) {
        return;
    }
    if (v == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    while (v > 0 && n < (u8)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    if (n + 1 > out_cap) {
        n = (u8)(out_cap - 1);
    }
    for (u8 i = 0; i < n; i++) {
        out[i] = tmp[n - 1 - i];
    }
    out[n] = '\0';
}

static void build_grid_map(u16* map32x32) {
    // Two tiles: 0=minor, 1=major. Use BG palette #1.
    const u16 pal_bits = BG_MAP_PAL(1);
    for (u16 y = 0; y < 32; y++) {
        for (u16 x = 0; x < 32; x++) {
            u16 tile = (u16)(((x & 3u) == 0u || (y & 3u) == 0u) ? 1u : 0u);
            map32x32[y * 32 + x] = (u16)(tile | pal_bits);
        }
    }
}

static void init_grid_bg2(void) {
    static u16 map32x32[32 * 32];

    // Upload tiles and palette
    dmaCopyVram((u8*)g_gridTiles4bpp, BG2_TILE_BASE, g_gridTiles4bpp_len);
    dmaCopyCGram((u8*)g_gridPal16, BG_PAL1_CGRAM_BYTE_OFFSET, g_gridPal16_len);

    // Build and upload map
    build_grid_map(map32x32);
    dmaCopyVram((u8*)map32x32, BG2_MAP_BASE, sizeof(map32x32));

    // Mode 1 (BG1/BG2 4bpp, BG3 2bpp). Keep BG3 enabled for console text.
    REG_BGMODE = 1;

    // BG2 tiles live at BG2_TILE_BASE; BG1 left at 0.
    REG_BG12NBA = (u8)(((BG2_TILE_BASE / 0x1000) & 0x0F) << 4);

    // BG2 map at BG2_MAP_BASE, size 32x32.
    REG_BG2SC = (u8)((((BG2_MAP_BASE >> 10) & 0x3F) << 2) | 0);

    // Enable BG2 + BG3 + OBJ on main screen.
    REG_TM = 0x16;
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

static void spawn_enemy(s16* ex, s16* ey) {
    const u16 r = rng_next_u16();
    const u8 edge = (u8)(r & 3u);

    // 16x16 enemy => keep inside 0..(W-16), 0..(H-16)
    const s16 min_x = 0;
    const s16 max_x = (s16)(SCREEN_W - 16);
    const s16 min_y = 0;
    const s16 max_y = (s16)(SCREEN_H - 16);

    switch (edge) {
        case 0: // top
            *ex = (s16)(r % (u16)(max_x + 1));
            *ey = min_y;
            break;
        case 1: // bottom
            *ex = (s16)(r % (u16)(max_x + 1));
            *ey = max_y;
            break;
        case 2: // left
            *ex = min_x;
            *ey = (s16)(r % (u16)(max_y + 1));
            break;
        default: // right
            *ex = max_x;
            *ey = (s16)(r % (u16)(max_y + 1));
            break;
    }
}

int main(void) {
    // Keep the font/console on BG3, and render the grid on BG2 behind it.
    consoleInit();

    // Force blank during VRAM/CGRAM setup.
    REG_INIDISP = 0x80;

    init_grid_bg2();
    init_sprites();

    // Turn screen on (brightness max).
    REG_INIDISP = 0x0F;

    consoleDrawText(2, 1, "starshmup");
    consoleDrawText(2, 2, "dpad move, autofire");

    s16 player_x = (SCREEN_W / 2) - 8;
    s16 player_y = (SCREEN_H / 2) - 8;
    s16 enemy_x = 0;
    s16 enemy_y = 0;
    spawn_enemy(&enemy_x, &enemy_y);

    Bullet bullets[8];
    for (u8 i = 0; i < 8; i++) {
        bullets[i].active = 0;
    }

    // Last non-zero direction (8-way). Defaults up.
    s8 aim_dx = 0;
    s8 aim_dy = -1;

    u16 score = 0;
    char score_num[6];
    char score_line[16];

    u16 frame = 0;
    u16 scroll_x = 0;
    u16 scroll_y = 0;

    while (1) {
        WaitForVBlank();

        scanPads();
        const u16 pad = padsCurrent(0);

        s8 move_dx = 0;
        s8 move_dy = 0;
        if (pad & KEY_LEFT)
            move_dx = -1;
        else if (pad & KEY_RIGHT)
            move_dx = 1;
        if (pad & KEY_UP)
            move_dy = -1;
        else if (pad & KEY_DOWN)
            move_dy = 1;

        if (move_dx || move_dy) {
            aim_dx = move_dx;
            aim_dy = move_dy;
        }

        // Player movement
        player_x = (s16)(player_x + (s16)move_dx * 2);
        player_y = (s16)(player_y + (s16)move_dy * 2);
        player_x = clamp_s16(player_x, 0, (s16)(SCREEN_W - 16));
        player_y = clamp_s16(player_y, 0, (s16)(SCREEN_H - 16));

        // Autofire: spawn a bullet every 6 frames.
        if ((frame % 6u) == 0u) {
            for (u8 i = 0; i < 8; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = 1;
                    bullets[i].x = (s16)(player_x + 4);
                    bullets[i].y = (s16)(player_y + 4);
                    bullets[i].vx = (s8)(aim_dx * 4);
                    bullets[i].vy = (s8)(aim_dy * 4);
                    break;
                }
            }
        }

        // Update bullets
        for (u8 i = 0; i < 8; i++) {
            if (!bullets[i].active)
                continue;
            bullets[i].x = (s16)(bullets[i].x + bullets[i].vx);
            bullets[i].y = (s16)(bullets[i].y + bullets[i].vy);
            if (bullets[i].x < -8 || bullets[i].x > (SCREEN_W + 8) || bullets[i].y < -8 || bullets[i].y > (SCREEN_H + 8)) {
                bullets[i].active = 0;
            }
        }

        // Enemy homes toward player (1 px/frame).
        if (enemy_x < player_x)
            enemy_x++;
        else if (enemy_x > player_x)
            enemy_x--;
        if (enemy_y < player_y)
            enemy_y++;
        else if (enemy_y > player_y)
            enemy_y--;

        // Bullet/enemy collision.
        for (u8 i = 0; i < 8; i++) {
            if (!bullets[i].active)
                continue;
            if (iabs_s16((s16)(bullets[i].x - enemy_x)) < 10 && iabs_s16((s16)(bullets[i].y - enemy_y)) < 10) {
                bullets[i].active = 0;
                score++;
                spawn_enemy(&enemy_x, &enemy_y);
                break;
            }
        }

        // Player/enemy collision => reset score.
        if (iabs_s16((s16)(player_x - enemy_x)) < 12 && iabs_s16((s16)(player_y - enemy_y)) < 12) {
            score = 0;
            player_x = (SCREEN_W / 2) - 8;
            player_y = (SCREEN_H / 2) - 8;
            spawn_enemy(&enemy_x, &enemy_y);
            for (u8 i = 0; i < 8; i++) {
                bullets[i].active = 0;
            }
        }

        // Update score text (BG3 console).
        u16_to_dec(score, score_num, sizeof(score_num));
        for (u8 i = 0; i < 15; i++) {
            score_line[i] = ' ';
        }
        score_line[15] = '\0';
        score_line[0] = 'S';
        score_line[1] = 'C';
        score_line[2] = 'O';
        score_line[3] = 'R';
        score_line[4] = 'E';
        score_line[5] = ':';
        score_line[6] = ' ';
        for (u8 i = 0; i < (u8)sizeof(score_num) && (u8)(7 + i) < 15; i++) {
            if (score_num[i] == '\0') {
                break;
            }
            score_line[7 + i] = score_num[i];
        }
        consoleDrawText(2, 4, score_line);

        // Scroll the grid slightly for “interstellar graph paper”.
        scroll_x++;
        if ((frame & 3u) == 0u) {
            scroll_y++;
        }
        REG_BG2HOFS = (u8)(scroll_x & 0xFF);
        REG_BG2HOFS = (u8)((scroll_x >> 8) & 0xFF);
        REG_BG2VOFS = (u8)(scroll_y & 0xFF);
        REG_BG2VOFS = (u8)((scroll_y >> 8) & 0xFF);

        // Draw sprites
        // Player sprite (tiles 0-3), large (16x16)
        oamSet(0, player_x, player_y, 2, 0, 0, 0, 0);
        oamSetEx(0, OBJ_LARGE, OBJ_SHOW);

        // Enemy sprite (tiles 4-7), large (16x16)
        oamSet(1, enemy_x, enemy_y, 2, 0, 0, 4, 0);
        oamSetEx(1, OBJ_LARGE, OBJ_SHOW);

        // Bullets sprite (tile 8), small (8x8)
        for (u8 i = 0; i < 8; i++) {
            const u8 obj = (u8)(2 + i);
            if (!bullets[i].active) {
                oamSetEx(obj, OBJ_SMALL, OBJ_HIDE);
                continue;
            }
            oamSet(obj, bullets[i].x, bullets[i].y, 1, 0, 0, 8, 0);
            oamSetEx(obj, OBJ_SMALL, OBJ_SHOW);
        }

        oamUpdate();

        frame++;
    }

    return 0;
}
