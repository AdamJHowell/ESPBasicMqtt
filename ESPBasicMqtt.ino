#ifdef ESP8266
#include <ESP8266WiFi.h>// ESP8266 WiFi support.  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#else
#include <WiFi.h>// Wi-Fi support.  https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h or https://www.arduino.cc/en/Reference/WiFi
#endif
#include <PubSubClient.h>// PubSub is the MQTT API maintained by Nick O'Leary: https://github.com/knolleary/pubsubclient


struct WiFiClient espClient;         // Create a WiFiClient to connect to the local network.
PubSubClient mqttClient( espClient );// Create a PubSub MQTT client object that uses the WiFiClient.


char ipAddress[ 16 ];                          // A character array to hold the IP address.
char macAddress[ 18 ];                         // A character array to hold the MAC address, and append a dash and 3 numbers.
long rssi;                                     // A global to hold the Received Signal Strength Indicator.
unsigned int printInterval = 10000;            // How long to wait between MQTT publishes.
unsigned long printCount = 0;                  // A counter of how many times the stats have been published.
unsigned long lastPrintTime = 0;               // The last time a MQTT publish was performed.
unsigned long wifiConnectionTimeout = 10000;   // The amount of time to wait for a Wi-Fi connection.
unsigned long mqttReconnectInterval = 10000;   // The amount of time to wait for a MQTT connection.
const unsigned int MCU_LED = 2;                // The GPIO which the onboard LED is connected to.
const char *wifiSsid = "Red5";                 // Wi-Fi SSID.
const char *wifiPassword = "8012254722";       // Wi-Fi password.
const char *hostname = "GenericESP";           // The hostname.
const char *mqttBroker = "127.0.0.1";          // The MQTT broker address.
const int mqttPort = 1883;                     // The MQTT broker port.
const int JSON_DOC_SIZE = 512;                 //
const char *MQTT_COMMAND_TOPIC = "MqttCommand";//


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
}// End of lookupWifiCode() function.


/**
 * @brief wifiBasicConnect() will connect to a SSID.
 */
void wifiBasicConnect()
{
	// Turn the LED off to show Wi-Fi is not connected.
	digitalWrite( MCU_LED, LOW );

	Serial.printf( "Attempting to connect to Wi-Fi SSID '%s'", wifiSsid );
	WiFi.mode( WIFI_STA );
	WiFi.config( INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE );
	WiFi.setHostname( hostname );
	WiFi.begin( wifiSsid, wifiPassword );

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
		snprintf( ipAddress, 16, "%d.%d.%d.%d", WiFi.localIP()[ 0 ], WiFi.localIP()[ 1 ], WiFi.localIP()[ 2 ], WiFi.localIP()[ 3 ] );
		// Turn the LED on to show that Wi-Fi is connected.
		digitalWrite( MCU_LED, HIGH );
		return;
	}
	else
		Serial.println( "Wi-Fi failed to connect in the timeout period.\n" );
}// End of wifiBasicConnect() function.


/**
 * @brief mqttConnect() will connect to a MQTT broker.
 */
int mqttConnect( int maxAttempts )
{
	if( WiFi.status() == WL_CONNECTED )
	{
		Serial.println( "\nFunction mqttConnect() has initiated." );
		digitalWrite( MCU_LED, LOW );

		Serial.printf( "Attempting to connect to the MQTT broker at '%s:%d' up to %d times.\n", mqttBroker, mqttPort, maxAttempts );

		// int attemptNumber = 0;
		// Loop until MQTT has connected.
		//		while( !mqttClient.connected() && attemptNumber < maxAttempts )
		//		{
		//			// Put the macAddress and random number into clientId.
		//			char clientId[ 22 ];
		//			//		snprintf( clientId, 22, "%s-%03ld", macAddress, random( 999 ) );
		//			snprintf( clientId, 19, "%s", macAddress );
		//			// Connect to the broker using the MAC address for a clientID.  This guarantees that the clientID is unique.
		//			Serial.printf( "Connecting with client ID '%s'.\n", clientId );
		//			Serial.printf( "Attempt # %d....", ( attemptNumber + 1 ) );
		//			if( mqttClient.connect( clientId ) )
		//			{
		//				Serial.println( " connected." );
		//				digitalWrite( MCU_LED, HIGH );
		//				if( !mqttClient.setBufferSize( JSON_DOC_SIZE ) )
		//				{
		//					Serial.printf( "Unable to create a buffer %d bytes long!\n", JSON_DOC_SIZE );
		//					Serial.println( "Restarting the device!" );
		//					ESP.restart();
		//				}
		//				// Subscribe to the command topic.
		//				if( mqttClient.subscribe( MQTT_COMMAND_TOPIC ) )
		//					Serial.printf( "Successfully subscribed to topic '%s'.\n", MQTT_COMMAND_TOPIC );
		//				else
		//					Serial.printf( "Failed to subscribe to topic '%s'!\n", MQTT_COMMAND_TOPIC );
		//			}
		//			else
		//			{
		//				int mqttState = mqttClient.state();
		//				/*
		//				Possible values for client.state():
		//				MQTT_CONNECTION_TIMEOUT     -4		// Note: This also comes up when the clientID is already in use.
		//				MQTT_CONNECTION_LOST        -3
		//				MQTT_CONNECT_FAILED         -2
		//				MQTT_DISCONNECTED           -1
		//				MQTT_CONNECTED               0
		//				MQTT_CONNECT_BAD_PROTOCOL    1
		//				MQTT_CONNECT_BAD_CLIENT_ID   2
		//				MQTT_CONNECT_UNAVAILABLE     3
		//				MQTT_CONNECT_BAD_CREDENTIALS 4
		//				MQTT_CONNECT_UNAUTHORIZED    5
		//			*/
		//				Serial.printf( " failed!  Return code: %d\n", mqttState );
		//				if( mqttState == -4 )
		//					Serial.println( " - MQTT_CONNECTION_TIMEOUT" );
		//				else if( mqttState == 2 )
		//					Serial.println( " - MQTT_CONNECT_BAD_CLIENT_ID" );
		//				else
		//					Serial.println( "" );
		//
		//				Serial.printf( "Trying again in %lu seconds.\n\n", mqttReconnectInterval / 1000 );
		//				delay( mqttReconnectInterval );
		//			}
		//			attemptNumber++;
		//		}

		while( !mqttClient.connected() )
		{
			Serial.println( "Connecting to MQTT..." );

			if( mqttClient.connect( "ESP32Client" ) )
				Serial.println( "connected" );
			else
			{
				Serial.printf( "Failed with state %d\n", mqttClient.state() );
				delay( 2000 );
			}
		}

		if( !mqttClient.connected() )
		{
			Serial.println( "Unable to connect to the MQTT broker!" );
			return 0;
		}
		Serial.println( "Function mqttConnect() has completed.\n" );
	}
	return 1;
}// End of mqttConnect() function.


/**
 * @brief readTelemetry() will read the telemetry to global variables.
 */
void readTelemetry()
{
	rssi = WiFi.RSSI();
}// End of readTelemetry() function.


/**
 * @brief printTelemetry() will print the telemetry to the serial port.
 */
void printTelemetry()
{
	printCount++;
	Serial.printf( "Publish count %ld\n", printCount );
	Serial.printf( "MAC address: %s\n", macAddress );
	int wifiStatusCode = WiFi.status();
	char buffer[ 26 ];
	lookupWifiCode( wifiStatusCode, buffer );
	Serial.printf( "Wi-Fi status text: %s\n", buffer );
	Serial.printf( "Wi-Fi status code: %d\n", wifiStatusCode );
	if( wifiStatusCode == 3 )
	{
		Serial.printf( "IP address: %s\n", ipAddress );
		Serial.printf( "RSSI: %ld\n", rssi );
	}
	if( !mqttClient.connected() )
	{
		Serial.print( "MQTT status: " );
		Serial.println( mqttClient.connected() );
	}
}// End of printTelemetry() function.


/**
 * @brief setup() will configure the program.
 */
void setup()
{
	delay( 500 );
	// Start Serial communications.
	Serial.begin( 115200 );
	if( !Serial )
		delay( 1000 );
	Serial.println( "\n\nsetup() is beginning." );

	// Set GPIO 2 (MCU_LED) as an output.
	pinMode( MCU_LED, OUTPUT );
	// Turn the LED on.
	digitalWrite( MCU_LED, HIGH );

	// Set the MAC address variable to its value.
	snprintf( macAddress, 18, "%s", WiFi.macAddress().c_str() );
}// End of setup() function.


/**
 * @brief loop() repeats over and over.
 */
void loop()
{
	if( WiFi.status() != WL_CONNECTED )
		wifiBasicConnect();
	if( !mqttClient.connected() )
		mqttConnect( 3 );

	long time = millis();
	// Print the first time.  Avoid subtraction overflow.  Print every interval.
	if( lastPrintTime == 0 || ( time > printInterval && ( time - printInterval ) > lastPrintTime ) )
	{
		readTelemetry();
		printTelemetry();
		lastPrintTime = millis();

		Serial.printf( "Next print in %u seconds.\n\n", printInterval / 1000 );
	}
}// End of loop() function.
