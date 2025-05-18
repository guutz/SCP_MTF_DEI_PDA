idf_component_register(SRCS "core/heavy_weapon.cpp" "core/shot_weapon.cpp" "core/beam_weapon.cpp" "core/base_weapon.cpp" "core/handler.cpp" "core/IR.cpp" "core/player.cpp" "core/setup.cpp"
	"fx/patterns/BasePattern.cpp" "fx/patterns/ShotFlicker.cpp" "fx/patterns/VestPattern.cpp"
	"fx/animatorThread.cpp" "fx/colorSets.cpp" "fx/ManeAnimator.cpp" "fx/patternModeHandling.cpp"
	"fx/sounds.cpp"
	"fx/vibrationHandler.cpp"
	INCLUDE_DIRS "include" "sounds"
	REQUIRES NetHelper XIRR AudioHandler NeoController MQTT_SubHandler BatteryManager json ESP32-MCP23008)
