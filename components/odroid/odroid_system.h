#pragma once

#include "odroid_audio.h"
#include "odroid_display.h"
#include "odroid_input.h"
#include "odroid_overlay.h"
#include "odroid_netplay.h"
#include "odroid_sdcard.h"
#include "odroid_settings.h"
#include "esp_system.h"
#include "stdbool.h"

extern int8_t speedupEnabled;

typedef bool (*state_handler_t)(char *pathName);

typedef struct
{
     char *romPath;
     char *startAction;
     int8_t speedup;
     state_handler_t load;
     state_handler_t save;
} emu_state_t;

void odroid_system_init(int app_id, int sampleRate);
void odroid_system_emu_init(char **romPath, int8_t *startAction, state_handler_t load, state_handler_t save);
void odroid_system_set_app_id(int appId);
void odroid_system_quit_app(bool save);
bool odroid_system_save_state(int slot);
bool odroid_system_load_state(int slot);
void odroid_system_panic(const char *reason);
void odroid_system_halt();
void odroid_system_sleep();
void odroid_system_set_boot_app(int slot);
void odroid_system_set_led(int value);
void odroid_system_gpio_init();


inline uint get_elapsed_time_since(uint start)
{
     uint now = xthal_get_ccount();
     return ((now > start) ? now - start : ((uint64_t)now + (uint64_t)0xffffffff) - start);
}
