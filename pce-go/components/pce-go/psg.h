#pragma once

#include <stdint.h>
#include <stddef.h>

int psg_init(int samplerate, bool stereo, bool downsample);
void psg_term(void);
void psg_update(int16_t *output, size_t length);
