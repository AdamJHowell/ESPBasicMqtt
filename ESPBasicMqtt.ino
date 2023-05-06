#ifdef ESP8266
#include <ESP8266WiFi.h> // ESP8266 WiFi support.  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#else

#include <WiFi.h> // Arduino Wi-Fi support.  This header is part of the standard library.  https://www.arduino.cc/en/Reference/WiFi

#endif

#include "PubSubClient.h"
#include "privateInfo.h"


char ipAddress[16];									// A character array to hold the IP address.
char macAddress[18];									// A character array to hold the MAC address, and append a dash and 3 numbers.
long rssi;												// A global to hold the Received Signal Strength Indicator.
unsigned int printInterval = 10000;				// How long to wait between stat printouts.
unsigned int ledBlinkInterval = 200;			// Time between blinks.
unsigned long printCount = 0;						// A counter of how many times the stats have been published.
unsigned long lastPrintTime = 0;					// The last time a MQTT publish was performed.
unsigned long lastLedBlinkTime = 0;				// The last time LED was blinked.
unsigned long lastBrokerConnect = 0;			// The last time a MQTT broker connection was attempted.
unsigned long brokerCoolDown = 7000;			// How long to wait between MQTT broker connection attempts.
unsigned long wifiConnectionTimeout = 15000; // The amount of time to wait for a Wi-Fi connection.
const unsigned int ONBOARD_LED = 2;				// The GPIO which the onboard LED is connected to.
//const char *HOSTNAME = "GenericESP";			// The HOSTNAME.  Defined in privateInfo.h
//const char *WIFI_SSID = "nunya";					// Wi-Fi SSID.  Defined in privateInfo.h
//const char *WIFI_PASSWORD = "nunya";				// Wi-Fi password.  Defined in privateInfo.h
//const char *BROKER_IP = "nunya";						// The broker address.  Defined in privateInfo.h
uint16_t port = 1883; // The broker port.


WiFiClient wifiClient;
PubSubClient mqttClient( wifiClient );

/**
 * @brief lookupWifiCode() will return the string for an integer code.
 */
void lookupWifiCode( int code, char *buffer )
{
	switch( code )
	{
		case 0:
			snprintf( buffer, 26, "%s", "Idle" );
			break;
		case 1:
			snprintf( buffer, 26, "%s", "No SSID" );
			break;
		case 2:
			snprintf( buffer, 26, "%s", "Scan completed" );
			break;
		case 3:
			snprintf( buffer, 26, "%s", "Connected" );
			break;
		case 4:
			snprintf( buffer, 26, "%s", "Connection failed" );
			break;
		case 5:
			snprintf( buffer, 26, "%s", "Connection lost" );
			break;
		case 6:
			snprintf( buffer, 26, "%s", "Disconnected" );
			break;
		default:
			snprintf( buffer, 26, "%s", "Unknown Wi-Fi status code" );
	}
} // End of lookupWifiCode() function.

/**
 * @brief lookupMQTTCode() will return the string for an integer state code.
 */
void lookupMQTTCode( int code, char *buffer )
{
	switch( code )
	{
		case -4:
			snprintf( buffer, 29, "%s", "Connection timeout" );
			break;
		case -3:
			snprintf( buffer, 29, "%s", "Connection lost" );
			break;
		case -2:
			snprintf( buffer, 29, "%s", "Connect failed" );
			break;
		case -1:
			snprintf( buffer, 29, "%s", "Disconnected" );
			break;
		case 0:
			snprintf( buffer, 29, "%s", "Connected" );
			break;
		case 1:
			snprintf( buffer, 29, "%s", "Bad protocol" );
			break;
		case 2:
			snprintf( buffer, 29, "%s", "Bad client ID" );
			break;
		case 3:
			snprintf( buffer, 29, "%s", "Unavailable" );
			break;
		case 4:
			snprintf( buffer, 29, "%s", "Bad credentials" );
			break;
		case 5:
			snprintf( buffer, 29, "%s", "Unauthorized" );
			break;
		default:
			snprintf( buffer, 29, "%s", "Unknown MQTT state code" );
	}
} // End of lookupMQTTCode() function.

/**
 * @brief wifiBasicConnect() will connect to a SSID.
 */
void wifiBasicConnect()
{
	// Turn the LED off to show Wi-Fi is not connected.
	digitalWrite( ONBOARD_LED, LOW );

	Serial.printf( "Attempting to connect to Wi-Fi SSID '%s'", WIFI_SSID );
	WiFi.mode( WIFI_STA );
	WiFi.config( INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE );
	WiFi.setHostname( HOSTNAME );
	WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

	unsigned long wifiConnectionStartTime = millis();

	// Loop until connected, or until wifiConnectionTimeout.
	while( WiFi.status() != WL_CONNECTED && ( millis() - wifiConnectionStartTime < wifiConnectionTimeout ) )
	{
		Serial.print( "." );
		delay( 1000 );
	}
	Serial.println( "" );

	if( WiFi.status() == WL_CONNECTED )
	{
		// Print that Wi-Fi has connected.
		Serial.println( "\nWi-Fi connection established!" );
		snprintf( ipAddress, 16, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2],
					 WiFi.localIP()[3] );
		// Turn the LED on to show that Wi-Fi is connected.
		digitalWrite( ONBOARD_LED, HIGH );
		return;
	}
	else
		Serial.println( "Wi-Fi failed to connect in the timeout period.\n" );
} // End of wifiBasicConnect() function.

/**
 * @brief readTelemetry() will read the telemetry and save values to global variables.
 */
void readTelemetry()
{
	rssi = WiFi.RSSI();
} // End of readTelemetry() function.

/**
 * @brief printTelemetry() will print the telemetry to the serial port.
 */
void printTelemetry()
{
	Serial.println();
	printCount++;

	Serial.printf( "Publish count %ld\n", printCount );
	Serial.println( __FILE__ );
	Serial.println();

	Serial.println( "Wi-Fi info:" );
	Serial.printf( "  MAC address: %s\n", macAddress );
	int wifiStatusCode = WiFi.status();
	char buffer[29] = "";
	lookupWifiCode( wifiStatusCode, buffer );
	Serial.printf( "  Wi-Fi status text: %s\n", buffer );
	Serial.printf( "  Wi-Fi status code: %d\n", wifiStatusCode );
	if( wifiStatusCode == 3 )
	{
		Serial.printf( "  IP address: %s\n", ipAddress );
		Serial.printf( "  RSSI: %ld\n", rssi );
	}
	Serial.println();

	Serial.println( "MQTT info:" );
	Serial.printf( "  Broker: %s:%d\n", BROKER_IP, port );
	int mqttStateCode = mqttClient.state();
	lookupMQTTCode( mqttStateCode, buffer );
	Serial.printf( "  MQTT state: %s\n", buffer );
	if( mqttClient.connected() )
	{
		Serial.print( "  MQTT broker domain: " );
		Serial.println( mqttClient.getServerDomain() );
		Serial.print( "  MQTT broker IP: " );
		Serial.println( mqttClient.getServerIP() );
		Serial.print( "  MQTT broker port: " );
		Serial.println( mqttClient.getServerPort() );
	}
	Serial.println();
} // End of printTelemetry() function.

void publishTelemetry()
{
	char valueBuffer[25] = "";
	snprintf( valueBuffer, 25, "%d", rssi );

	// Publish the RSSI.
	mqttClient.publish( "test/EspBasicMQTT/rssi", valueBuffer );
} // End of publishTelemetry() function.

/**
 * @brief mqttCallback() will process incoming messages on subscribed topics.
 */
void mqttCallback( char *topic, byte *payload, unsigned int length )
{
	Serial.printf( "\nMessage arrived on Topic: '%s'\n", topic );

	char message[5] = { 0x00 };

	for( unsigned int i = 0; i < length; i++ )
		message[i] = ( char ) payload[i];

	message[length] = 0x00;
	Serial.println( message );
	String str_msg = String( message );
	if( str_msg.equals( "ON" ) )
		digitalWrite( ONBOARD_LED, HIGH );
	else if( str_msg.equals( "on" ) )
		digitalWrite( ONBOARD_LED, HIGH );
	else if( str_msg.equals( "OFF" ) )
		digitalWrite( ONBOARD_LED, LOW );
	else if( str_msg.equals( "off" ) )
		digitalWrite( ONBOARD_LED, LOW );
	else
		Serial.printf( "Unknown command '%s'\n", message );
} // End of mqttCallback() function.

/**
 * @brief mqttConnect() will connect to the MQTT broker.
 */
void mqttConnect()
{
	long time = millis();
	// Connect the first time.  Avoid subtraction overflow.  Connect after cool down.
	if( lastBrokerConnect == 0 || ( time > brokerCoolDown && ( time - brokerCoolDown ) > lastBrokerConnect ) )
	{
		lastBrokerConnect = millis();
		digitalWrite( ONBOARD_LED, LOW );
		Serial.printf( "Connecting to broker at %s:%d...\n", BROKER_IP, port );
		mqttClient.setServer( BROKER_IP, port );
		mqttClient.setCallback( mqttCallback );

		if( mqttClient.connect( macAddress ) )
			Serial.print( "Connected to MQTT Broker.\n" );
		else
		{
			int mqttStateCode = mqttClient.state();
			char buffer[29];
			lookupMQTTCode( mqttStateCode, buffer );
			Serial.printf( "  MQTT state: %s\n", buffer );
			Serial.printf( "  MQTT state code: %d\n", mqttStateCode );
			return;
		}

		mqttClient.subscribe( "led1" );
		digitalWrite( ONBOARD_LED, HIGH );
	}
} // End of mqttConnect() function.

/**
 * @brief toggleLED() will change the state of the LED.
 * This function does not manage any timings.
 */
void toggleLED()
{
	if( digitalRead( ONBOARD_LED ) != 1 )
		digitalWrite( ONBOARD_LED, 1 );
	else
		digitalWrite( ONBOARD_LED, 0 );
} // End of toggleLED() function.

/**
 * @brief setup() will configure the program.
 */
void setup()
{
	delay( 1000 );
	Serial.begin( 115200 );
	if( !Serial )
		delay( 1000 );
	Serial.println( "\n" );
	Serial.println( "setup() is beginning." );

	// Set GPIO 2 (ONBOARD_LED) as an output.
	pinMode( ONBOARD_LED, OUTPUT );
	// Turn the LED on.
	digitalWrite( ONBOARD_LED, HIGH );

	// Set the MAC address variable to its value.
	snprintf( macAddress, 18, "%s", WiFi.macAddress().c_str() );
	Serial.println( "Function setup() has completed." );
} // End of setup() function.

/**
 * @brief loop() repeats over and over.
 */
void loop()
{
	if( WiFi.status() != WL_CONNECTED )
		wifiBasicConnect();
	else if( !mqttClient.connected() )
		mqttConnect();
	else
		mqttClient.loop();

	// Print the first currentTime.  Print every interval.
	if( lastPrintTime == 0 || ( ( millis() - lastPrintTime ) > printInterval ) )
	{
		readTelemetry();
		printTelemetry();
		lastPrintTime = millis();

		Serial.printf( "Next print in %u seconds.\n\n", printInterval / 1000 );
	}

	// Process the first currentTime.  Process every interval.
	if( lastLedBlinkTime == 0 || ( ( millis() - lastLedBlinkTime ) > ledBlinkInterval ) )
	{
		// If Wi-Fi is connected, but MQTT is not, blink the LED.
		if( WiFi.status() == WL_CONNECTED )
		{
			if( mqttClient.state() != 0 )
				toggleLED();						  // Toggle the LED state to show that Wi-Fi is connected by MQTT is not.
			else
				digitalWrite( ONBOARD_LED, 1 ); // Turn the LED on to show both Wi-Fi and MQTT are connected.
		}
		else
			digitalWrite( ONBOARD_LED, 0 ); // Turn the LED off to show that Wi-Fi is not connected.
		lastLedBlinkTime = millis();
	}
} // End of loop() function.
