#include <rg_system.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "settings.h"
#include "gui.h"

static setting_t setting;
static listbox_t option_list;

const uint8_t option_count = 2;
char options[][15] = { "Volume", "Theme" };

const uint8_t volume_option_count = 1;
char volume_options[][32] = {"Loudness    <  %d  >" };

const uint8_t theme_option_count = 4;
char theme_options[][15] = { "iPod", "Gameboy", "Blinding", "Aliens Turret" };

const uint8_t gameboy_option_count = 3;
char gameboy_options[][15] = { "Palette", "Framerate", "Scaling" };
const uint8_t gameboy_palette_option_count = 5;
char gameboy_palette_options[][15] = { "DMG", "Pocket", "Light", "GBC", "SGB" };


static void tab_refresh(setting_t *setting)
{
    tab_t *tab = setting->tab;
    size_t items_count = 0;
    char *ext = NULL;

    if (!tab)
        return;

    memset(&tab->status, 0, sizeof(tab->status));

    gui_resize_list(tab, option_count);
    for(int i = 0; i<option_count; i++){
        sprintf(tab->listbox.items[i].text, options[i]);
    }
    tab->listbox.cursor = 0;
}

void settings_display_gameboy_palette()
{
    uint8_t sel = 0;
    rg_gui_event_t event = RG_DIALOG_INIT;
    uint32_t joystick = 0, joystick_old;
    uint64_t joystick_last = 0;
    uint8_t currentPalette = rg_settings_get_number("gb", "Palette", 0);

    option_list.capacity = gameboy_palette_option_count;    
    option_list.length = gameboy_palette_option_count;
    option_list.items = realloc(option_list.items, option_list.capacity * sizeof(listbox_item_t));
    for(int i = 0; i < gameboy_palette_option_count; i++){
        listbox_item_t *item = &option_list.items[i];
        snprintf(item->text, 128, gameboy_palette_options[i]);
        if (i + 32 == currentPalette){
            option_list.cursor = i;
            sel = i;
        }
    }

    int details_l_margin =  186;
    joystick_old = rg_input_read_gamepad();
    while(joystick_old & RG_KEY_A){
        joystick_old = rg_input_read_gamepad();
    }

    while (event != RG_DIALOG_CLOSE)
    {
        joystick_old = ((rg_system_timer() - joystick_last) > 300000) ? 0 : joystick;
        joystick = rg_input_read_gamepad();
        event = RG_DIALOG_VOID;

        if (joystick ^ joystick_old)
        {
            if (joystick & RG_KEY_UP) {
                if (sel == 0) {
                    sel = option_list.capacity - 1;
                } else {
                    --sel;
                }
            }
            else if (joystick & RG_KEY_DOWN) {
                if (++sel > option_list.capacity - 1) sel = 0;
            }
            else if (joystick & (RG_KEY_B|RG_KEY_OPTION|RG_KEY_MENU)) {
                event = RG_DIALOG_DISMISS;
                rg_settings_commit();
            }
            if (joystick & RG_KEY_A) {
                rg_settings_set_number("gb", "Palette", 32 + sel);
            }

            if (event == RG_DIALOG_DISMISS) {
                event = RG_DIALOG_CLOSE;
                sel = 0;
            }

            option_list.cursor = sel;

            joystick_last = rg_system_timer();
        }

        if (rg_input_quit_pressed()){
            event = RG_DIALOG_CLOSE;
        } else {
            gui_draw_default_background();
            setting.tab->navpath = const_string("Gameboy Palette");
            gui_draw_header(setting.tab, 0);
            gui_draw_optionlist(&option_list);
        }

        if (event == RG_DIALOG_CLOSE)
            break;

        rg_gui_flush();
        rg_task_delay(20);
        rg_system_tick(0);
    }

    rg_input_wait_for_key(joystick, false);

    rg_display_force_redraw();

}

void set_gameboy_settings(){
    option_list.capacity = gameboy_option_count;    
    option_list.length = gameboy_option_count;
    option_list.items = realloc(option_list.items, option_list.capacity * sizeof(listbox_item_t));
    for(int i = 0; i < gameboy_option_count; i++){
        listbox_item_t *item = &option_list.items[i];
        snprintf(item->text, 128, gameboy_options[i]);
    }
}

void settings_display_gameboy()
{
    uint8_t sel = 0;
    rg_gui_event_t event = RG_DIALOG_INIT;
    uint32_t joystick = 0, joystick_old;
    uint64_t joystick_last = 0;

    set_gameboy_settings();

    int details_l_margin =  186;
    joystick_old = rg_input_read_gamepad();
    while(joystick_old & RG_KEY_A){
        joystick_old = rg_input_read_gamepad();
    }

    while (event != RG_DIALOG_CLOSE)
    {
        joystick_old = ((rg_system_timer() - joystick_last) > 300000) ? 0 : joystick;
        joystick = rg_input_read_gamepad();
        event = RG_DIALOG_VOID;

        if (joystick ^ joystick_old)
        {
            if (joystick & RG_KEY_UP) {
                if (sel == 0) {
                    sel = option_list.capacity - 1;
                } else {
                    --sel;
                }
            }
            else if (joystick & RG_KEY_DOWN) {
                if (++sel > option_list.capacity - 1) sel = 0;
            }
            else if (joystick & (RG_KEY_B|RG_KEY_OPTION|RG_KEY_MENU)) {
                event = RG_DIALOG_DISMISS;
            }
            if (joystick & RG_KEY_A) {
                settings_display_gameboy_palette();
            }

            if (event == RG_DIALOG_DISMISS) {
                event = RG_DIALOG_CLOSE;
                sel = 0;
            }

            option_list.cursor = sel;

            joystick_last = rg_system_timer();
        }

        if (rg_input_quit_pressed()){
            event = RG_DIALOG_CLOSE;
        } else {
            gui_draw_default_background();
            setting.tab->navpath = const_string("Gameboy");
            gui_draw_header(setting.tab, 0);
            gui_draw_optionlist(&option_list);
        }

        if (event == RG_DIALOG_CLOSE)
            break;

        rg_gui_flush();
        rg_task_delay(20);
        rg_system_tick(0);
    }

    rg_input_wait_for_key(joystick, false);

    rg_display_force_redraw();

}



void settings_display_theme_settings()
{
    uint8_t sel = 0;
    rg_gui_event_t event = RG_DIALOG_INIT;
    uint32_t joystick = 0, joystick_old;
    uint64_t joystick_last = 0;
    
    option_list.capacity = theme_option_count;    
    option_list.capacity = theme_option_count;    
    option_list.length = theme_option_count;
    option_list.items = realloc(option_list.items, option_list.capacity * sizeof(listbox_item_t));
    for(int i = 0; i < theme_option_count; i++){
        listbox_item_t *item = &option_list.items[i];
        snprintf(item->text, 128, theme_options[i]);
        if (gui.color_theme == i){
            option_list.cursor = i;
            sel = i;
        }
    }


    int details_l_margin =  186;
    joystick_old = rg_input_read_gamepad();
    while(joystick_old & RG_KEY_A){
        joystick_old = rg_input_read_gamepad();
    }

    while (event != RG_DIALOG_CLOSE)
    {
        joystick_old = ((rg_system_timer() - joystick_last) > 300000) ? 0 : joystick;
        joystick = rg_input_read_gamepad();
        event = RG_DIALOG_VOID;

        if (joystick ^ joystick_old)
        {
            if (joystick & RG_KEY_UP) {
                if (sel == 0) {
                    sel = option_list.capacity - 1;
                } else {
                    --sel;
                }
            }
            else if (joystick & RG_KEY_DOWN) {
                if (++sel > option_list.capacity - 1) sel = 0;
            }
            else if (joystick & (RG_KEY_B|RG_KEY_OPTION|RG_KEY_MENU)) {
                event = RG_DIALOG_DISMISS;
            }
            if (joystick & RG_KEY_A) {
                gui.color_theme = sel;
                gui_save_config();
            }

            if (event == RG_DIALOG_DISMISS) {
                event = RG_DIALOG_CLOSE;
                sel = 0;
            }

            option_list.cursor = sel;

            joystick_last = rg_system_timer();
        }

        if (rg_input_quit_pressed()){
            event = RG_DIALOG_CLOSE;
        } else {
            gui_draw_default_background();
            setting.tab->navpath = const_string("Theme");
            gui_draw_header(setting.tab, 0);
            gui_draw_optionlist(&option_list);
        }

        if (event == RG_DIALOG_CLOSE)
            break;

        rg_gui_flush();
        rg_task_delay(20);
        rg_system_tick(0);
    }

    rg_input_wait_for_key(joystick, false);

    rg_display_force_redraw();

}

void settings_display_volume_settings()
{
    uint8_t sel = 0;
    rg_gui_event_t event = RG_DIALOG_INIT;
    uint32_t joystick = 0, joystick_old;
    uint64_t joystick_last = 0;
    uint8_t fixed_volume = (int)rg_settings_get_number(NS_GLOBAL, "Volume", 50);
     
    option_list.capacity = volume_option_count;    
    option_list.length = volume_option_count;
    option_list.items = realloc(option_list.items, option_list.capacity * sizeof(listbox_item_t));
    listbox_item_t *item = &option_list.items[0];
    snprintf(item->text, 128, volume_options[0], fixed_volume );


    int details_l_margin =  186;
    joystick_old = rg_input_read_gamepad();
    while(joystick_old & RG_KEY_A){
        joystick_old = rg_input_read_gamepad();
    }

    while (event != RG_DIALOG_CLOSE)
    {
        joystick_old = ((rg_system_timer() - joystick_last) > 300000) ? 0 : joystick;
        joystick = rg_input_read_gamepad();
        event = RG_DIALOG_VOID;

        if (joystick ^ joystick_old)
        {
            if (joystick & RG_KEY_UP) {
                if (sel == 0) {
                    sel = option_list.capacity - 1;
                } else {
                    --sel;
                }
            }
            else if (joystick & RG_KEY_DOWN) {
                if (++sel > option_list.capacity - 1) sel = 0;
            }
            else if (joystick & (RG_KEY_B|RG_KEY_OPTION|RG_KEY_MENU)) {
                event = RG_DIALOG_DISMISS;
                rg_settings_commit();
            }

            if (joystick & RG_KEY_LEFT) {
                item = &option_list.items[sel];

                if (fixed_volume == 0) {
                    fixed_volume = 100;
                } else {
                    fixed_volume--;
                }
                snprintf(item->text, 128, volume_options[0], fixed_volume);
                rg_settings_set_number(NS_GLOBAL, "Volume", fixed_volume);
                rg_gui_flush();
            }

            if (joystick & RG_KEY_RIGHT) {
                item = &option_list.items[sel];
                fixed_volume++;
                if (fixed_volume > 100) fixed_volume = 0;
                snprintf(item->text, 128, volume_options[0], fixed_volume);
                rg_settings_set_number(NS_GLOBAL, "Volume", fixed_volume);
                rg_gui_flush();
            }

            if (event == RG_DIALOG_DISMISS) {
                event = RG_DIALOG_CLOSE;
                sel = 0;
            }

            option_list.cursor = sel;

            joystick_last = rg_system_timer();
        }

        if (rg_input_quit_pressed()){
            event = RG_DIALOG_CLOSE;
        } else {
            gui_draw_default_background();
            setting.tab->navpath = const_string("Volume");
            gui_draw_header(setting.tab, 0);
            gui_draw_optionlist(&option_list);
        }

        if (event == RG_DIALOG_CLOSE)
            break;

        rg_gui_flush();
        rg_task_delay(20);
        rg_system_tick(0);
    }

    rg_input_wait_for_key(joystick, false);

    rg_display_force_redraw();

}


static void event_handler(gui_event_t event, tab_t *tab)
{
    listbox_item_t *item = gui_get_selected_item(tab);
    // book_t *book = (book_t *)tab->arg;

    if (event == TAB_INIT)
    {
    }
    else if (event == TAB_REFRESH)
    {
        // tab_refresh(tab);
    }
    else if (event == TAB_ENTER || event == TAB_SCROLL)
    {
        gui_set_status(tab, NULL, "");
        gui_set_preview(tab, NULL);
    }
    else if (event == TAB_LEAVE)
    {
        //
    }
    else if (event == TAB_IDLE)
    {
        if ((gui.idle_counter % 100) == 0)
            crc_cache_idle_task(tab);
    }
    else if (event == TAB_ACTION)
    {
        // for(int i = 0; i < option_count; i++){
        //     if (strcmp(item->text, options[i]) == 0) {
        //         gui.color_theme = i;
        //         gui_save_config();
        //         break;
        //     } 
        // }
        
        if (setting.tab->navpath == NULL) {
            if (strcmp(item->text, options[0]) == 0) {
                settings_display_volume_settings();
            } 
            if (strcmp(item->text, options[1]) == 0) {
                settings_display_theme_settings();
            } 
            if (strcmp(item->text, options[2]) == 0) {
                settings_display_gameboy_palette();
            } 
       }

      setting.tab->navpath = NULL;
      //  setting_t *setting = (setting_t *)tab->arg;
       //tab_refresh(setting);

    }
    else if (event == TAB_BACK)
    {
        // This is now reserved for subfolder navigation (go back)
    }
}


void settings_init(void)
{
    setting_t *psetting = &setting;
    psetting->name = "Settings";
    psetting->tab = gui_add_tab(psetting->name, psetting->name, psetting, event_handler);
    psetting->initialized = true; 
    
    
    tab_refresh(psetting);
}
