#include <PubSubClient.h>
#include <WiFi.h>

#define LED 2

const char *ssid = "Red5";
const char *passwd = "8012254722";

WiFiClient wifiClient;
PubSubClient mqttClient( wifiClient );

void mqttCallback( char *topic, byte *payload, unsigned int length )
{
	Serial.print( "Message arrived on Topic:" );
	Serial.print( topic );

	char message[ 5 ] = { 0x00 };

	for( int i = 0; i < length; i++ )
		message[ i ] = ( char ) payload[ i ];

	message[ length ] = 0x00;
	Serial.print( message );
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
		Serial.printf( "Unknown command '%s'", message );
} // End of mqttCallback() function.

void setup()
{
	Serial.begin( 115200 );
	Serial.println( "Beginning setup()..." );
	pinMode( LED, OUTPUT );
	WiFi.begin( ssid, passwd );

	Serial.print( "Connecting to AP" );
	while( WiFi.status() != WL_CONNECTED )
	{
		Serial.print( "." );
		delay( 1000 );
	}
	Serial.print( "Connected to WiFi AP, Got an IP address :" );
	Serial.print( WiFi.localIP() );

	mqttClient.setServer( "192.168.55.200", 1883 );
	mqttClient.setCallback( mqttCallback );

	if( mqttClient.connect( "ESP-Client", NULL, NULL ) )
		Serial.print( "Connected to MQTT Broker" );
	else
	{
		Serial.print( "MQTT Broker connection failed" );
		Serial.print( mqttClient.state() );
		delay( 1000 );
	}

	mqttClient.subscribe( "led1" );
	Serial.println( "Function setup() has completed." );
	digitalWrite( LED, HIGH );
} // End of setup() function.

void loop()
{
	mqttClient.loop();
} // End of loop() function.
