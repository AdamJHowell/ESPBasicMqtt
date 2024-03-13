cmake_minimum_required( VERSION 3.23 )
project( ESPBasicMqtt )

set( CMAKE_CXX_STANDARD 14 )

include_directories(
		.
		"C:/Users/Adam/AppData/Local/Arduino15/packages/esp32/hardware/esp32/2.0.14/libraries/WiFi/src"
		"C:/Users/Adam/AppData/Local/Arduino15/packages/esp32/hardware/esp32/2.0.14/tools/sdk/esp32/include/esp_wifi/include"
		// LWIP defines types such as uint16_t.
		"C:/Users/Adam/AppData/Local/Arduino15/packages/esp32/hardware/esp32/2.0.14/tools/sdk/esp32/include/lwip/port/esp32/include/arch"

		"C:/Users/Adam/AppData/Local/Arduino15/packages/esp8266/hardware/esp8266/3.1.2/cores/esp8266"
		"C:/Users/Adam/AppData/Local/Arduino15/packages/esp8266/hardware/esp8266/3.1.2/libraries/ESP8266WiFi/src"
		// LWIP defines types such as uint16_t.
		"C:/Users/Adam/AppData/Local/Arduino15/packages/esp8266/hardware/esp8266/3.1.2/tools/sdk/lwip2/include"
)

add_executable( ESPBasicMqtt
		ESPBasicMqtt.ino
		privateInfo.h
		PubSubClient.cpp
		PubSubClient.h
		privateInfo.h )

set_target_properties( ESPBasicMqtt PROPERTIES LINKER_LANGUAGE CXX )
