#ifdef ESP8266
// These headers are installed when the ESP8266 is installed in board manager.
#include <ESP8266WiFi.h> // ESP8266 WiFi support.  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <ESP8266mDNS.h> // OTA - mDNSResponder (Multicast DNS) for the ESP8266 family.
#elif ESP32
// These headers are installed when the ESP32 is installed in board manager.
#include <WiFi.h>		// ESP32 Wifi support.  https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h
#include <ESPmDNS.h> // OTA - Multicast DNS for the ESP32.
#else
#include <WiFi.h> // Arduino Wi-Fi support.  This header is part of the standard library.  https://www.arduino.cc/en/Reference/WiFi
#endif

#include <PubSubClient.h>

#define LED 2

const char *ssid = "nunya";
const char *passwd = "nunya";
const char *broker = "nunya";
uint16_t port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient( wifiClient );


void mqttCallback( char *topic, byte *payload, unsigned int length )
{
	Serial.printf( "\nMessage arrived on Topic: '%s'\n", topic );

	char message[ 5 ] = { 0x00 };

	for( unsigned int i = 0; i < length; i++ )
		message[ i ] = ( char ) payload[ i ];

	message[ length ] = 0x00;
	Serial.println( message );
	String str_msg = String( message );
	if( str_msg.equals( "ON" ) )
		digitalWrite( LED, HIGH );
	else if( str_msg.equals( "on" ) )
		digitalWrite( LED, HIGH );
	else if( str_msg.equals( "OFF" ) )
		digitalWrite( LED, LOW );
	else if( str_msg.equals( "off" ) )
		digitalWrite( LED, LOW );
	else
		Serial.printf( "Unknown command '%s'\n", message );
} // End of mqttCallback() function.


void mqttConnect()
{
  digitalWrite( LED, LOW );
	mqttClient.setServer( broker, port );
	mqttClient.setCallback( mqttCallback );

	if( mqttClient.connect( "ESP-Client", NULL, NULL ) )
		Serial.print( "Connected to MQTT Broker.\n" );
	else
	{
		Serial.printf( "MQTT Broker connection failed with state %d\n", mqttClient.state() );
		delay( 5000 );
    return;
	}

	mqttClient.subscribe( "led1" );
  digitalWrite( LED, HIGH );
} // End of mqttConnect() function.


void setup()
{
  delay( 1000 );
	Serial.begin( 115200 );
	Serial.println( "\n\nBeginning setup()..." );
	pinMode( LED, OUTPUT );

	WiFi.begin( ssid, passwd );
	Serial.printf( "Connecting to AP %s\n", ssid );
	while( WiFi.status() != WL_CONNECTED )
	{
		Serial.print( "." );
		delay( 1000 );
	}
  Serial.println();
	Serial.print( "Connected to Wi-Fi, got an IP address : " );
	Serial.println( WiFi.localIP() );

	digitalWrite( LED, HIGH );
	Serial.println( "Function setup() has completed." );
} // End of setup() function.


void loop()
{
	mqttClient.loop();
  if( !mqttClient.connected() )
    mqttConnect();
} // End of loop() function.
