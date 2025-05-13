idf_component_register(
    SRCS
    "assets/SCP_Foundation.c"
    "assets/lv_font_firacode_4.c"
    "assets/lv_font_firacode_8.c"
    "assets/lv_font_firacode_12.c"
    "assets/lv_font_firacode_16.c"
    "ui_manager.cpp"
    "sd_manager.cpp"
    "main.cpp"
    "setup.cpp"
    "ota_manager.cpp"
    "mcp23008_wrapper.cpp"
    "cd4053b_wrapper.cpp"
    "joystick.cpp"
    INCLUDE_DIRS "."
)
