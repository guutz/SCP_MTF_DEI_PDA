# CMakeLists.txt for CommunicationManager component

idf_component_register(SRCS "CommunicationManager.cpp" "EspMeshHandler.cpp" "mesh_netif.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_event nvs_flash lwip mqtt MQTT_SubHandler)
