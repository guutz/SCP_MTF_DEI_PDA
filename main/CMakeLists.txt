idf_component_register(
    SRCS 
    "SCP_Foundation.c" 
    "lv_font_firacode_4.c" 
    "lv_font_firacode_8.c" 
    "lv_font_firacode_12.c"
    "lv_font_firacode_16.c"
    "ui_manager.cpp"
    "sd_manager.cpp"
    "main.cpp"
    "../components/ESP32-MCP23008/mcp23008.c"
    INCLUDE_DIRS "."
    "../components/ESP32-MCP23008/include"
)
