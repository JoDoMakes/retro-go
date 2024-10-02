#pragma once

#include "applications.h"

typedef enum
{
    THEME = 0,
    GAMEBOY,
    GAMEGEAR,
    NES,
} setting_type_t;

typedef struct
{
    const char *name;
    tab_t *tab;
    bool initialized;
} setting_t;


void settings_init(void);

