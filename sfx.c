#include <snes.h>

#include "sfx.h"

extern char SOUNDBANK__;
extern char sfx_laser, sfx_laser_end;
extern char sfx_explosion, sfx_explosion_end;

static brrsamples g_sfxLaser;
static brrsamples g_sfxExplosion;

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
    spcSetSoundEntry(15, 8, 2, explosion_len, (u8 *)&sfx_explosion, &g_sfxExplosion);
    spcSetSoundEntry(12, 8, 6, laser_len, (u8 *)&sfx_laser, &g_sfxLaser);
}

void sfx_process(void) {
    spcProcess();
}

void sfx_ui_confirm(void) {
    spcPlaySoundV(SFX_IDX_LASER, 10);
}

void sfx_shot(void) {
    spcPlaySoundV(SFX_IDX_LASER, 8);
}

void sfx_enemy_hit(void) {
    spcPlaySoundV(SFX_IDX_LASER, 6);
}

void sfx_enemy_down(void) {
    spcPlaySoundV(SFX_IDX_EXPLOSION, 15);
}

void sfx_level_up(void) {
    spcPlaySoundV(SFX_IDX_EXPLOSION, 12);
}

void sfx_player_down(void) {
    spcPlaySoundV(SFX_IDX_EXPLOSION, 15);
}

