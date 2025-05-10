/////////////////////////////////////////////
//// INCLUDES ///////////////////////////////
/////////////////////////////////////////////

#include "lvgl.h"
#include "lvgl_helpers.h"
#include "ui_manager.h"
#include "sd_manager.h" 
#include "stuff.h"
#include "esp_log.h"
#include <cinttypes>
#include <string> 
#include <vector> 
#include <map>    

LV_FONT_DECLARE(lv_font_firacode_16);
LV_IMG_DECLARE(SCP_Foundation);


////////////////////////////////////////////////////
/////// STATIC DECLARATIONS ////////////////////////
////////////////////////////////////////////////////

static const char* TAG_UI_MGR = "ui_manager";

// --- Screen Creation Function Implementations ---
static lv_obj_t* create_splash_screen(void);
// Wrapper for creating dynamic menu screens (uses globals G_TargetMenuNameForCreation and G_InvokingParentMenuName)
static lv_obj_t* create_dynamic_menu_screen_wrapper(void);
// Implementation for creating screens from definition
static lv_obj_t* create_screen_from_definition_impl(const MenuScreenDefinition* definition, const std::string& actual_invoking_parent_name);
// Wrapper for text screens
static lv_obj_t* text_screen_creator_wrapper(void);
// Implementation for text display screens
static lv_obj_t* create_text_display_screen_impl(const std::string& title, const std::string& content, bool is_file, const std::string& actual_invoking_parent_name);


// --- LVGL Task Callback for initial splash timeout ---
static void initial_splash_timeout_cb(lv_task_t *task);

// --- Event handler for dynamically created buttons ---
static void dynamic_button_event_handler(lv_obj_t * obj, lv_event_t event);


///////////////////////////////////////////////
/////// GLOBAL VARIABLES //////////////////////
///////////////////////////////////////////////

// --- Global Menu Data ---
std::map<std::string, MenuScreenDefinition> G_MenuScreens;
std::map<std::string, predefined_cpp_function_t> G_PredefinedFunctions;

// --- Navigation State Globals ---
std::string G_TargetMenuNameForCreation = ""; // Name of the menu to be created next
std::string G_InvokingParentMenuName = "";  // Name of the screen that is causing G_TargetMenuNameForCreation to be loaded.
                                            // This becomes the "back target" for the new screen.

// --- Globals for Text Screen Creation (used by text_screen_creator_wrapper) ---
std::string G_TextScreenTitle = "Text Viewer";
std::string G_TextScreenContent = "";
bool G_TextScreenIsFilePath = false;

// --- Global Style Objects ---
lv_style_t style_default_screen_bg;
lv_style_t style_default_label; // General purpose label style (e.g., for titles, static text)
lv_style_t style_default_button;
lv_style_t style_transparent_container; // For label containers


// --- UserData for Buttons ---
// This structure will be stored in each button's user_data
typedef struct {
    MenuItemDefinition item_def; // The original item definition from menu.txt
    std::string on_screen_name;  // Name of the screen this button resides on
    std::string invoking_parent_for_on_screen; // Name of the screen that invoked on_screen_name (this is the target for this screen's back button)
} ButtonActionContext;


//////////////////////////////////////////////////////////////////////
/// FUNCTION IMPLEMENTATIONS /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ui_styles_init(void) {
    // Default Screen Background Style
    lv_style_init(&style_default_screen_bg);
    lv_style_set_bg_color(&style_default_screen_bg, LV_STATE_DEFAULT, TERMINAL_COLOR_BACKGROUND);
    lv_style_set_bg_opa(&style_default_screen_bg, LV_STATE_DEFAULT, LV_OPA_COVER);

    // Default Label Style (for titles, static text items on menus)
    lv_style_init(&style_default_label);
    lv_style_set_text_font(&style_default_label, LV_STATE_DEFAULT, TERMINAL_FONT);
    lv_style_set_text_color(&style_default_label, LV_STATE_DEFAULT, TERMINAL_COLOR_FOREGROUND); // Using green
    lv_style_set_text_opa(&style_default_label, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_line_space(&style_default_label, LV_STATE_DEFAULT, TERMINAL_LABEL_LINE_SPACE);

    // Default Button Style (Normal and Pressed States)
    lv_style_init(&style_default_button);
    lv_style_set_radius(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_RADIUS);

    // Normal state
    lv_style_set_bg_opa(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_BG_OPA);
    lv_style_set_bg_color(&style_default_button, LV_STATE_DEFAULT, TERMINAL_COLOR_BACKGROUND); // Transparent, relies on border
    lv_style_set_border_width(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_BORDER_WIDTH);
    lv_style_set_border_color(&style_default_button, LV_STATE_DEFAULT, TERMINAL_COLOR_BUTTON_BORDER);
    lv_style_set_text_color(&style_default_button, LV_STATE_DEFAULT, TERMINAL_COLOR_BUTTON_TEXT);
    lv_style_set_text_opa(&style_default_button, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_text_font(&style_default_button, LV_STATE_DEFAULT, TERMINAL_FONT);
    lv_style_set_pad_all(&style_default_button, LV_STATE_DEFAULT, TERMINAL_BUTTON_INNER_PADDING);


    // Pressed state
    lv_style_set_bg_opa(&style_default_button, LV_STATE_PRESSED, LV_OPA_COVER); // Make background visible on press
    lv_style_set_bg_color(&style_default_button, LV_STATE_PRESSED, TERMINAL_COLOR_BUTTON_PRESSED_BG);
    lv_style_set_text_color(&style_default_button, LV_STATE_PRESSED, TERMINAL_COLOR_BUTTON_PRESSED_TEXT);
    lv_style_set_border_color(&style_default_button, LV_STATE_PRESSED, TERMINAL_COLOR_BUTTON_PRESSED_BORDER); // Change border on press

    // Style for transparent containers (used to wrap static labels) - remains the same
    lv_style_init(&style_transparent_container);
    lv_style_set_bg_opa(&style_transparent_container, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_transparent_container, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_all(&style_transparent_container, LV_STATE_DEFAULT, 0);
}

// Main UI screen loading function
void ui_load_active_target_screen(lv_scr_load_anim_t anim_type,
                                  uint32_t anim_time,
                                  uint32_t anim_delay,
                                  bool auto_delete_old,
                                  screen_create_func_t screen_creator_func) {
    if (!screen_creator_func) {
        ESP_LOGE(TAG_UI_MGR, "ui_load_active_target_screen: NULL screen creator function provided.");
        return;
    }
    ESP_LOGI(TAG_UI_MGR, "Preparing to load target screen: '%s', Invoked by: '%s'", G_TargetMenuNameForCreation.c_str(), G_InvokingParentMenuName.c_str());

    lv_obj_t *new_screen = screen_creator_func(); 

    if (!new_screen) {
        ESP_LOGE(TAG_UI_MGR, "ui_load_active_target_screen: Screen creator function failed for target '%s'.", G_TargetMenuNameForCreation.c_str());
        return;
    }

    ESP_LOGI(TAG_UI_MGR, "Loading screen '%s' with animation. Type: %d, Time: %" PRIu32 "ms", G_TargetMenuNameForCreation.c_str(), (int)anim_type, anim_time);
    lv_scr_load_anim(new_screen, anim_type, anim_time, anim_delay, auto_delete_old);
}


void ui_init(lv_task_t *current_init_task) {
    (void)current_init_task;
    ESP_LOGI(TAG_UI_MGR, "UI Init: Initializing styles and preparing splash screen.");

    ui_styles_init();

    G_PredefinedFunctions["diag1"] = example_predefined_function_1;
    G_PredefinedFunctions["diag2"] = example_predefined_function_2;

    if (parse_menu_definition_file("S:/DEI/menu.txt")) { 
        ESP_LOGI(TAG_UI_MGR, "Successfully parsed menu definitions.");
        if (!G_MenuScreens.empty()) {
            auto it = G_MenuScreens.find("MainMenu");
            if (it == G_MenuScreens.end()) {
                ESP_LOGW(TAG_UI_MGR, "'MainMenu' not found, defaulting to first parsed menu.");
                it = G_MenuScreens.begin(); 
            }
            if (it != G_MenuScreens.end()) {
                 G_TargetMenuNameForCreation = it->first;
            } else {
                ESP_LOGE(TAG_UI_MGR, "No 'MainMenu' or any other menu found after parsing. No initial screen target.");
                G_TargetMenuNameForCreation = ""; 
            }
        } else {
             ESP_LOGW(TAG_UI_MGR, "Menu definition file was empty or contained no valid menus.");
             G_TargetMenuNameForCreation = "";
        }
    } else {
        ESP_LOGE(TAG_UI_MGR, "Failed to parse menu definitions. UI might be non-functional.");
        G_TargetMenuNameForCreation = ""; 
    }
    
    G_InvokingParentMenuName = ""; 

    ESP_LOGI(TAG_UI_MGR, "Initial target menu after ui_init: '%s'", G_TargetMenuNameForCreation.c_str());

    lv_obj_t *splash_screen_obj = create_splash_screen();
    lv_disp_load_scr(splash_screen_obj);

    lv_task_t *transition_task = lv_task_create(initial_splash_timeout_cb, 3000, LV_TASK_PRIO_MID, NULL);
    lv_task_once(transition_task);
}

static void initial_splash_timeout_cb(lv_task_t *task) {
    (void)task;
    ESP_LOGI(TAG_UI_MGR, "Splash timeout. Transitioning to: '%s'", G_TargetMenuNameForCreation.c_str());
    if (!G_TargetMenuNameForCreation.empty() && G_MenuScreens.count(G_TargetMenuNameForCreation)) {
        ui_load_active_target_screen(LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, true, create_dynamic_menu_screen_wrapper);
    } else {
        ESP_LOGE(TAG_UI_MGR, "No initial menu target ('%s') or definition found. Cannot transition from splash.", G_TargetMenuNameForCreation.c_str());
    }
}

static lv_obj_t* create_splash_screen(void) {
    ESP_LOGI(TAG_UI_MGR, "Creating splash screen elements.");
    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    lv_obj_add_style(screen, LV_OBJ_PART_MAIN, &style_default_screen_bg);
    lv_obj_set_size(screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));

    lv_obj_t *scp_logo = lv_img_create(screen, NULL);
    lv_img_set_src(scp_logo, &SCP_Foundation); 
    lv_obj_set_style_local_image_recolor(scp_logo, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
    lv_obj_set_style_local_image_recolor_opa(scp_logo, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_align(scp_logo, NULL, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *loading_label = lv_label_create(screen, NULL);
    lv_obj_add_style(loading_label, LV_LABEL_PART_MAIN, &style_default_label);
    lv_label_set_text(loading_label, "Loading...");
    lv_obj_align(loading_label, scp_logo, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    
    return screen;
}

static lv_obj_t* create_dynamic_menu_screen_wrapper(void) {
    ESP_LOGD(TAG_UI_MGR, "Wrapper: Creating menu '%s', invoked by '%s'", G_TargetMenuNameForCreation.c_str(), G_InvokingParentMenuName.c_str());
    auto it = G_MenuScreens.find(G_TargetMenuNameForCreation);
    if (it != G_MenuScreens.end()) {
        return create_screen_from_definition_impl(&(it->second), G_InvokingParentMenuName);
    }
    ESP_LOGE(TAG_UI_MGR, "Menu definition not found in wrapper: %s", G_TargetMenuNameForCreation.c_str());
    return NULL; 
}

static lv_obj_t* create_screen_from_definition_impl(const MenuScreenDefinition* definition, const std::string& actual_invoking_parent_name) {
    if (!definition) {
        ESP_LOGE(TAG_UI_MGR, "Cannot create screen: null definition provided.");
        return NULL;
    }

    ESP_LOGI(TAG_UI_MGR, "Creating screen: '%s' (Title: '%s'). Invoked by: '%s'", 
             definition->name.c_str(), definition->title.c_str(), actual_invoking_parent_name.c_str());
    
    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    lv_obj_add_style(screen, LV_OBJ_PART_MAIN, &style_default_screen_bg); 
    lv_obj_set_size(screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL)); 
    ESP_LOGD(TAG_UI_MGR, "Screen actual size after set_size: %d x %d", lv_obj_get_width(screen), lv_obj_get_height(screen));


    lv_obj_t *title_label = lv_label_create(screen, NULL);
    lv_obj_add_style(title_label, LV_LABEL_PART_MAIN, &style_default_label);
    lv_label_set_text(title_label, definition->title.c_str());
    lv_obj_set_width(title_label, lv_obj_get_width(screen) - (2 * TERMINAL_PADDING_HORIZONTAL)); // Use new padding
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_SROLL_CIRC);
    lv_label_set_align(title_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(title_label, NULL, LV_ALIGN_IN_TOP_MID, 0, TERMINAL_PADDING_VERTICAL_TITLE_TOP); // Use new padding

    int item_y_offset = lv_obj_get_y(title_label) + lv_obj_get_height_fit(title_label) + TERMINAL_PADDING_VERTICAL_AFTER_TITLE; // Use new padding
    const int item_spacing = TERMINAL_ITEM_SPACING;                               // Use new spacing
    const int button_height = TERMINAL_BUTTON_HEIGHT;                             // Use new height
    const lv_coord_t horizontal_padding = TERMINAL_PADDING_HORIZONTAL;           // Use new padding

    for (const auto& item_def_from_vector : definition->items) {
        if (item_def_from_vector.render_type == RENDER_AS_STATIC_LABEL) {

            lv_obj_t* label_container = lv_cont_create(screen, NULL);
            lv_obj_add_style(label_container, LV_CONT_PART_MAIN, &style_transparent_container);

            // This correctly calculates the container's width as 280px
            lv_coord_t calculated_container_width = lv_obj_get_width(screen) - (2 * horizontal_padding);
            lv_obj_set_width(label_container, calculated_container_width);
            lv_cont_set_fit2(label_container, LV_FIT_NONE, LV_FIT_TIGHT); // Width fixed, height tight
            lv_obj_align(label_container, NULL, LV_ALIGN_IN_TOP_LEFT, horizontal_padding, item_y_offset);

            // Create the label as a child of the container
            lv_obj_t *static_label_obj = lv_label_create(label_container, NULL);
            lv_obj_add_style(static_label_obj, LV_LABEL_PART_MAIN, &style_default_label);

            // --- Try this order of operations ---
            // 1. Set the text first.
            lv_label_set_text(static_label_obj, item_def_from_vector.text_to_display.c_str());

            // 2. Then, set the long mode.
            lv_label_set_long_mode(static_label_obj, LV_LABEL_LONG_BREAK);

            // 3. Finally, set the width of the label.
            //    Use the explicitly calculated container width to be certain.
            lv_obj_set_width(static_label_obj, calculated_container_width);
            // --- End of suggested order ---

            lv_label_set_align(static_label_obj, LV_LABEL_ALIGN_LEFT);

            // Your logging:
            lv_coord_t actual_container_height = lv_obj_get_height(label_container);
            ESP_LOGD(TAG_UI_MGR, "Static label container ('%.20s...') cont_w:%d, cont_h:%d, y:%d. Label_w:%d, Label_h:%d",
                    item_def_from_vector.text_to_display.c_str(),
                    lv_obj_get_width(label_container), // Should be 280
                    actual_container_height,
                    item_y_offset,
                    lv_obj_get_width(static_label_obj), // Hopefully 280 now
                    lv_obj_get_height(static_label_obj));

            item_y_offset += actual_container_height + item_spacing;

        } else if (item_def_from_vector.render_type == RENDER_AS_BUTTON) {
            lv_obj_t *btn = lv_btn_create(screen, NULL);
            lv_obj_add_style(btn, LV_BTN_PART_MAIN, &style_default_button); 
            lv_obj_set_size(btn, lv_obj_get_width(screen) - (2 * horizontal_padding), button_height); 
            lv_obj_align(btn, NULL, LV_ALIGN_IN_TOP_MID, 0, item_y_offset);
            
            item_y_offset += lv_obj_get_height(btn) + item_spacing;

            lv_obj_t *btn_label_obj = lv_label_create(btn, NULL);
            lv_label_set_text(btn_label_obj, item_def_from_vector.text_to_display.c_str());
            lv_obj_align(btn_label_obj, NULL, LV_ALIGN_CENTER, 0, 0);

            ButtonActionContext* btn_ctx = new ButtonActionContext();
            btn_ctx->item_def = item_def_from_vector; 
            btn_ctx->on_screen_name = definition->name; 
            btn_ctx->invoking_parent_for_on_screen = actual_invoking_parent_name; 
            
            lv_obj_set_user_data(btn, btn_ctx);
            lv_obj_set_event_cb(btn, dynamic_button_event_handler);
        }
    }
    return screen;
}

static void dynamic_button_event_handler(lv_obj_t * obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
        ButtonActionContext* ctx = (ButtonActionContext*)lv_obj_get_user_data(obj);
        if (!ctx) {
            ESP_LOGE(TAG_UI_MGR, "Button event: No ButtonActionContext found.");
            return;
        }

        if (ctx->item_def.action == ACTION_NONE) {
            ESP_LOGW(TAG_UI_MGR, "Button clicked ('%s') but has ACTION_NONE. Ignoring.", ctx->item_def.text_to_display.c_str());
            return;
        }

        ESP_LOGI(TAG_UI_MGR, "Button clicked on screen '%s': Label '%s', Action: %d, Target: '%s'. This screen's invoking parent was '%s'",
                 ctx->on_screen_name.c_str(), ctx->item_def.text_to_display.c_str(), ctx->item_def.action, 
                 ctx->item_def.action_target.c_str(), ctx->invoking_parent_for_on_screen.c_str());

        screen_create_func_t next_screen_creator = nullptr;
        lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT; 

        switch (ctx->item_def.action) {
            case ACTION_NAVIGATE_SUBMENU:
                if (G_MenuScreens.count(ctx->item_def.action_target)) {
                    G_TargetMenuNameForCreation = ctx->item_def.action_target;
                    G_InvokingParentMenuName = ctx->on_screen_name; 
                    next_screen_creator = create_dynamic_menu_screen_wrapper;
                } else {
                    ESP_LOGE(TAG_UI_MGR, "Submenu definition not found: %s", ctx->item_def.action_target.c_str());
                }
                break;

            case ACTION_DISPLAY_TEXT_CONTENT_SCREEN:
            case ACTION_DISPLAY_TEXT_FILE_SCREEN: 
                G_TextScreenTitle = ctx->item_def.text_to_display; 
                G_TextScreenContent = ctx->item_def.action_target; 
                G_TextScreenIsFilePath = (ctx->item_def.action == ACTION_DISPLAY_TEXT_FILE_SCREEN);
                G_TargetMenuNameForCreation = G_TextScreenTitle; 
                G_InvokingParentMenuName = ctx->on_screen_name; 
                next_screen_creator = text_screen_creator_wrapper;
                break;

            case ACTION_EXECUTE_PREDEFINED_FUNCTION:
                if (G_PredefinedFunctions.count(ctx->item_def.action_target)) {
                    ESP_LOGI(TAG_UI_MGR, "Executing function: %s", ctx->item_def.action_target.c_str());
                    G_PredefinedFunctions[ctx->item_def.action_target](); 
                } else {
                    ESP_LOGE(TAG_UI_MGR, "Predefined function not found: %s", ctx->item_def.action_target.c_str());
                }
                break; 

            case ACTION_GO_BACK:
                anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
                if (!ctx->invoking_parent_for_on_screen.empty() && G_MenuScreens.count(ctx->invoking_parent_for_on_screen)) {
                    G_TargetMenuNameForCreation = ctx->invoking_parent_for_on_screen; 
                    
                    auto target_def_it = G_MenuScreens.find(G_TargetMenuNameForCreation); 
                    if (target_def_it != G_MenuScreens.end()) { 
                        G_InvokingParentMenuName = target_def_it->second.defined_parent_name; 
                         ESP_LOGI(TAG_UI_MGR, "Going back to '%s'. Its defined parent (new invoking parent) is '%s'.",
                                 G_TargetMenuNameForCreation.c_str(), G_InvokingParentMenuName.c_str());
                    } else { 
                        ESP_LOGW(TAG_UI_MGR, "Critical: Definition for back target '%s' not found. Setting invoking parent to empty.", G_TargetMenuNameForCreation.c_str());
                        G_InvokingParentMenuName = ""; 
                    }
                    next_screen_creator = create_dynamic_menu_screen_wrapper;
                } else {
                     ESP_LOGW(TAG_UI_MGR, "Back target '%s' is invalid or menu definition not found. Cannot go back.", ctx->invoking_parent_for_on_screen.c_str());
                     if (G_MenuScreens.count("MainMenu")) {
                         G_TargetMenuNameForCreation = "MainMenu";
                         auto main_menu_it = G_MenuScreens.find("MainMenu");
                         if (main_menu_it != G_MenuScreens.end()){
                            G_InvokingParentMenuName = main_menu_it->second.defined_parent_name; 
                         } else {
                            G_InvokingParentMenuName = ""; 
                         }
                         next_screen_creator = create_dynamic_menu_screen_wrapper;
                         ESP_LOGI(TAG_UI_MGR, "Defaulting back to MainMenu. Its parent is '%s'", G_InvokingParentMenuName.c_str());
                     } else {
                         ESP_LOGE(TAG_UI_MGR, "No valid back target and MainMenu not found.");
                     }
                }
                break;
            
            case ACTION_NONE: 
                 ESP_LOGW(TAG_UI_MGR, "Clicked button ('%s') has ACTION_NONE. This should not happen for interactive items.", ctx->item_def.text_to_display.c_str());
                 break;

            default:
                ESP_LOGW(TAG_UI_MGR, "Unknown action type: %d for button '%s'", ctx->item_def.action, ctx->item_def.text_to_display.c_str());
                break;
        }

        if (next_screen_creator) {
            ui_load_active_target_screen(anim, 300, 0, true, next_screen_creator);
        }

    } else if (event == LV_EVENT_DELETE) {
        ButtonActionContext* ctx = (ButtonActionContext*)lv_obj_get_user_data(obj);
        if (ctx) {
            ESP_LOGD(TAG_UI_MGR, "Deleting ButtonActionContext for: %s", ctx->item_def.text_to_display.c_str());
            delete ctx;
            lv_obj_set_user_data(obj, NULL); 
        }
    }
}

static lv_obj_t* text_screen_creator_wrapper(void) {
    ESP_LOGD(TAG_UI_MGR, "Wrapper: Creating text screen '%s', invoked by '%s'", G_TextScreenTitle.c_str(), G_InvokingParentMenuName.c_str());
    return create_text_display_screen_impl(G_TextScreenTitle, G_TextScreenContent, G_TextScreenIsFilePath, G_InvokingParentMenuName);
}

static lv_obj_t* create_text_display_screen_impl(const std::string& title, const std::string& content, bool is_file, const std::string& actual_invoking_parent_name) {
    ESP_LOGI(TAG_UI_MGR, "Creating text screen: '%s'. Invoked by: '%s'", title.c_str(), actual_invoking_parent_name.c_str());

    lv_obj_t *screen = lv_obj_create(NULL, NULL);
    lv_obj_add_style(screen, LV_OBJ_PART_MAIN, &style_default_screen_bg); 
    lv_obj_set_size(screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));

    lv_obj_t *title_label_ts = lv_label_create(screen, NULL); 
    lv_obj_add_style(title_label_ts, LV_LABEL_PART_MAIN, &style_default_label); 
    lv_label_set_text(title_label_ts, title.c_str());
    lv_obj_set_width(title_label_ts, lv_obj_get_width(screen) - 20);
    lv_label_set_long_mode(title_label_ts, LV_LABEL_LONG_SROLL_CIRC);
    lv_label_set_align(title_label_ts, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(title_label_ts, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);

    // Use a page for scrollable text content in LVGL 7
    lv_obj_t *text_page = lv_page_create(screen, NULL);
    lv_obj_set_size(text_page, lv_obj_get_width(screen) - 20, lv_obj_get_height(screen) - lv_obj_get_y(title_label_ts) - lv_obj_get_height(title_label_ts) - 60); 
    lv_obj_align(text_page, title_label_ts, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_style(text_page, LV_PAGE_PART_BG, &style_default_screen_bg); 
    lv_obj_add_style(text_page, LV_PAGE_PART_SCROLLABLE, &style_default_screen_bg); 
    lv_page_set_scrl_layout(text_page, LV_LAYOUT_COLUMN_LEFT);
    
    lv_obj_set_style_local_bg_color(text_page, LV_PAGE_PART_SCROLLBAR, LV_STATE_DEFAULT, TERMINAL_COLOR_FOREGROUND);
    lv_obj_set_style_local_bg_opa(text_page, LV_PAGE_PART_SCROLLBAR, LV_STATE_DEFAULT, LV_OPA_COVER);


    lv_obj_t *text_content_label_ts = lv_label_create(text_page, NULL); 
    lv_obj_add_style(text_content_label_ts, LV_LABEL_PART_MAIN, &style_default_label); 
    lv_label_set_long_mode(text_content_label_ts, LV_LABEL_LONG_BREAK); 
    lv_obj_set_width(text_content_label_ts, lv_page_get_width_fit(text_page)); 


    if (is_file) {
        esp_err_t read_res = lvgl_display_text_from_sd_file(text_content_label_ts, content.c_str()); 
        if (read_res != ESP_OK) {
            lv_label_set_text(text_content_label_ts, "Error: Could not load text from file.");
        }
    } else {
        lv_label_set_text(text_content_label_ts, content.c_str()); 
    }

    lv_obj_t *btn_back = lv_btn_create(screen, NULL);
    lv_obj_add_style(btn_back, LV_BTN_PART_MAIN, &style_default_button);
    lv_obj_set_size(btn_back, 100, 35); 
    lv_obj_align(btn_back, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);

    lv_obj_t *btn_back_label = lv_label_create(btn_back, NULL);
    lv_label_set_text(btn_back_label, "Back");
    lv_obj_align(btn_back_label, NULL, LV_ALIGN_CENTER, 0, 0);

    ButtonActionContext* back_btn_ctx = new ButtonActionContext();
    back_btn_ctx->item_def.action = ACTION_GO_BACK; 
    back_btn_ctx->item_def.text_to_display = "Back"; 
    back_btn_ctx->on_screen_name = title; 
    back_btn_ctx->invoking_parent_for_on_screen = actual_invoking_parent_name; 
    
    lv_obj_set_user_data(btn_back, back_btn_ctx); 
    lv_obj_set_event_cb(btn_back, dynamic_button_event_handler);

    return screen;
}

// --- Example Predefined Functions ---
void example_predefined_function_1(void) {
    ESP_LOGI(TAG_UI_MGR, "Executed: example_predefined_function_1");
}

void example_predefined_function_2(void) {
    ESP_LOGI(TAG_UI_MGR, "Executed: example_predefined_function_2");
}
