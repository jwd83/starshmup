.include "hdr.asm"

.section ".rodata1" superfree

sfx_explosion:
.incbin "audio/explosion.brr"
sfx_explosion_end:

sfx_laser:
.incbin "audio/laser.brr"
sfx_laser_end:

.ends

