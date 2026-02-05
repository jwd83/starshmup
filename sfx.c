#include <snes.h>

#include "sfx.h"

extern char SOUNDBANK__;
extern char sfx_laser, sfx_laser_end;
extern char sfx_explosion, sfx_explosion_end;

static brrsamples g_sfxLaser;
static brrsamples g_sfxExplosion;

static u8 g_sfxLockFrames = 0;
static u8 g_laserCooldownFrames = 0;

enum {
    // spcPlaySound() index 0 is the last sound registered via spcSetSoundEntry().
    SFX_IDX_LASER = 0,
    SFX_IDX_EXPLOSION = 1,
};

void sfx_init(void) {
    u16 explosion_len;
    u16 laser_len;

    // Initialize sound engine (takes some time).
    spcBoot();

    // Provide a soundbank origin (even if we only use BRR SFX).
    spcSetBank((u8 *)&SOUNDBANK__);

    // Reserve SPC RAM for BRR SFX (size * 256 bytes). Must fit our largest sample.
    spcAllocateSoundRegion(40);

    explosion_len = (u16)(&sfx_explosion_end - &sfx_explosion);
    laser_len = (u16)(&sfx_laser_end - &sfx_laser);

    // Register EXPLOSION first so LASER becomes index 0 (most frequently played).
    spcSetSoundEntry(15, 8, 3, explosion_len, (u8 *)&sfx_explosion, &g_sfxExplosion);
    spcSetSoundEntry(15, 8, 4, laser_len, (u8 *)&sfx_laser, &g_sfxLaser);
}

void sfx_process(void) {
    if (g_sfxLockFrames) {
        g_sfxLockFrames--;
    }
    if (g_laserCooldownFrames) {
        g_laserCooldownFrames--;
    }
    spcProcess();
}

static u8 sfx_can_play_laser(void) {
    return (g_sfxLockFrames == 0) && (g_laserCooldownFrames == 0);
}

void sfx_ui_confirm(void) {
    if (!sfx_can_play_laser()) return;
    g_laserCooldownFrames = 10;
    spcPlaySoundV(SFX_IDX_LASER, 15);
}

void sfx_shot(void) {
    if (!sfx_can_play_laser()) return;
    g_laserCooldownFrames = 8;
    spcPlaySoundV(SFX_IDX_LASER, 15);
}

void sfx_enemy_hit(void) {
    if (!sfx_can_play_laser()) return;
    g_laserCooldownFrames = 10;
    spcPlaySoundV(SFX_IDX_LASER, 12);
}

void sfx_enemy_down(void) {
    g_sfxLockFrames = 24;
    g_laserCooldownFrames = 24;
    spcPlaySoundV(SFX_IDX_EXPLOSION, 15);
}

void sfx_level_up(void) {
    g_sfxLockFrames = 28;
    g_laserCooldownFrames = 28;
    spcPlaySoundV(SFX_IDX_EXPLOSION, 12);
}

void sfx_player_down(void) {
    g_sfxLockFrames = 60;
    g_laserCooldownFrames = 60;
    spcPlaySoundV(SFX_IDX_EXPLOSION, 15);
}
