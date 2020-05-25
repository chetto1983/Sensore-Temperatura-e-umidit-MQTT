/*
 Name:		Controllo_Ventola.ino
 Created:	20/12/2018 09:48:20
 Author:	Davide Marchetto
*/


#include <ESP8266HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>




constexpr auto Led = 2;
Adafruit_BME280 bme;



const char* ssid = "*****";
const char* password = "******";
const char* mqtt_server = "*******";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
float  temperatura;
float umidita;
int setpoint = 55;
int timeout = 0;
int sensor_error = 0;

void reset()
{
	ESP.reset();
}

void setup_wifi() {

	delay(10);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {

		delay(1000);
		Serial.print(".");
		timeout++;


		// se dopo 5 minuti non si riprende riavvio
		if (timeout >= 300)
		{
			reset();
		}
	}
	timeout = 0;

	randomSeed(micros());

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {

	String Intopic = topic;
	String messsage;
	char in_msg[50] = " ";
	for (int i = 0; i < length; i++) {
		in_msg[i] = (char)payload[i];
	}
	messsage = in_msg;
}

void reconnect() {
	// Loop until we're reconnected
	while (!client.connected()) {
		// Create a random client ID
		String clientId = "ESP8266Client-";
		clientId += String(random(0xffff), HEX);
		// Attempt to connect
		if (client.connect(clientId.c_str())) {
			Serial.println("connected");
			snprintf(msg, 50, "%d", setpoint);
			client.publish("bagno/setpoint", msg);
			timeout = 0;
		}
		else {

			delay(1000);
			timeout++;

			if (timeout >= 300)
			{
				reset();
			}
		}
	}
}

void setup() {
	pinMode(Led, OUTPUT);
	Serial.begin(115200);
	if (!bme.begin())
	{
		
		while (bme.begin()) 
		{
			Serial.println("errore del sensore");
			delay(500);
		}

	}
	setup_wifi();
	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);

}

void loop() {


	if (WiFi.status() != WL_CONNECTED)
	{
		setup_wifi();
	}

	if (!client.connected()) {
		reconnect();
	}
	client.loop();

	long now = millis();

	if (sensor_error >= 6)
	{
		reset();
	}

	if (now - lastMsg > 10000) {
		lastMsg = now;
		umidita = bme.readHumidity();
		temperatura = bme.readTemperature();
		float pressione = bme.readPressure()/100.0F;
		if (isnan(umidita) || isnan(temperatura)) {

			client.publish("bagno/sensore", "Allarme");
			sensor_error++;
		}
		else
		{
			client.publish("bagno/sensore", "OK");
			snprintf(msg, 50, "%f", temperatura);
			client.publish("bagno/temperatura", msg);
			snprintf(msg, 50, "%f", umidita);
			client.publish("bagno/umidita", msg);
			snprintf(msg, 50, "%f", pressione);
			client.publish("bagno/pressione", msg);
			sensor_error = 0;
		}
		client.publish("bagno/scheda", "ON");
	}
}

