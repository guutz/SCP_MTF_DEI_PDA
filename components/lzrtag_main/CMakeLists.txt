idf_component_register(SRCS "core/heavy_weapon.cpp" "core/shot_weapon.cpp" "core/beam_weapon.cpp" "core/base_weapon.cpp" "core/handler.cpp" "core/player.cpp"
	"fx/animatorThread.cpp" "fx/ManeAnimator.cpp"
	"fx/sounds.cpp"
	"fx/vibrationHandler.cpp" "fx/mcp_access.cpp"
	INCLUDE_DIRS "include"
	REQUIRES XIRR AudioHandler MQTT_SubHandler BatteryManager json ESP32-MCP23008)
