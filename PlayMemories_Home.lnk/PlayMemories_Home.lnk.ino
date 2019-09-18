
/**
 *  d888888b  .d88b.  d8888b.  .d88b.  
 *  `~~88~~' .8P  Y8. 88  `8D .8P  Y8. 
 *     88    88    88 88   88 88    88 
 *     88    88    88 88   88 88    88 
 *     88    `8b  d8' 88  .8D `8b  d8' 
 *     YP     `Y88P'  Y8888D'  `Y88P'  
 * 
 *  x Implement playback
 *  ► Implement save
 *  ► Implement load
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LinkedList.h>
#include <limits.h>

#ifndef STASSID
#define STASSID "T22 902"
#define STAPSK "c0a8b74282"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

#pragma region pin config
#define PIN_RX D1
#define PIN_CW D2
#define PIN_TX D3
const int led = 2;
#pragma endregion

#pragma region led blink code

void blink(unsigned long wait_on, unsigned long wait_off)
{
	digitalWrite(led, LOW);
	delay(wait_on);
	digitalWrite(led, HIGH);
	delay(wait_off);
}

#pragma endregion

#pragma region Recorder config

const int PIN_RXT = digitalPinToInterrupt(PIN_RX);
#define RecordTimeout 200

static LinkedList<unsigned long> signalBuffer;
static unsigned long recordingSince = 0;
static bool isRecording = false;

ICACHE_RAM_ATTR void rx_isr()
{
	if (!isRecording)
	{
		isRecording = true;
		recordingSince = millis();
	}
	signalBuffer.add(micros());
}

void recorderLoop()
{

	if (isRecording)
	{
		unsigned long now = millis();
		unsigned long diff = now - recordingSince;

		if (diff > RecordTimeout)
		{
			detachInterrupt(PIN_RXT);
			isRecording = false;
			endofRecord();
		}
	}
}

void handle_record_service()
{
	blink(200, 500);
	attachInterrupt(PIN_RXT, rx_isr, CHANGE);
}

void endofRecord()
{
	blink(100, 500);
	
	unsigned long off=signalBuffer.get(0);
	unsigned long temp = 0;
	unsigned long diff = 0;
	LinkedList <unsigned long>tempBuff;

	// saving the delta in tempbuff

	for (int i=1; i<signalBuffer.size(); i++)
	{
		temp = signalBuffer.get(i);
	
		if(temp>off)
			diff = temp-off;
		else
			diff=temp + (ULONG_MAX - off);

		tempBuff.add(diff);
		off=temp;
	}


	//copying tempbuff to sigbuff;
	signalBuffer.clear();
	for(int i=0; i<tempBuff.size();i++)
		signalBuffer.add(tempBuff.get(i));
	
	tempBuff.clear();
	blink(500, 500);
}

#pragma endregion

#pragma region fetch service

void handle_fetch_service()
{
	String ret = "{\"timecode\":[0";

	int len = signalBuffer.size();
	unsigned long acc=0;
	for (int i = 0; i < len; i++)
	{
		acc+=signalBuffer.get(i);
		ret += "," + String(acc);
	}

	ret += "]}";

	server.send(200, "application/json", ret);
}

#pragma endregion

#pragma region handle playback service
void handle_playback_service()
{
	int len = signalBuffer.size();
	bool state = true;

	analogWriteFreq ( 40000 );
	analogWrite ( PIN_CW, 511 );

	blink(200,1000);

	for(int i=0; i<len; i++)
	{
		if(state)
			digitalWrite ( PIN_TX, HIGH);
		else
			digitalWrite ( PIN_TX, LOW);
		
		delayMicroseconds(signalBuffer.get(i));

		state = !state;
	}
	digitalWrite( PIN_TX, LOW );

	delay(1000);
	blink(200,1000);
}
#pragma endregion

#pragma region http server

String
determineContentType(String url)
{
	String ct;
	String ext = "";

	for (int i = url.length(); i >= 0; i--)
	{
		if (url.charAt(i) == '.')
		{
			ext = url.substring(i);
			break;
		}
	}

	if (ext == ".htm" || ext == ".html")
		return "text/html";

	if (ext = ".js")
		return "text/javascript";

	if (ext == ".css")
		return "text/css";

	return "application/octet-stream";
}

void handleNotFound()
{
	String uri = server.uri();
	if (uri == "/")
	{
		server.sendHeader("Location", "/index.html");
		server.send(302, "text/plain", "");
		return;
	}

	if (SPIFFS.exists(uri))
	{
		File file = SPIFFS.open(uri, "r");
		String ct = determineContentType(uri);

		server.streamFile(file, ct);
	}
	else
	{
		Serial.println("Request url not found ");
		Serial.print("\"");
		Serial.print(uri);
		Serial.println("\"");
		server.send(404, "text/plain", "File not found\n" + uri);
	}
}

void setupServer()
{
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});

	// @todo
	// implement services

	server.on("/record.service", handle_record_service);
	server.on("/fetchRecord.service", handle_fetch_service);
	server.on("/playback.service", handle_playback_service);

	server.on("/saveRecord.service", []() {

	});

	server.on("/listRecord.service", []() {

	});

	server.on("/loadsignal.service", []() {

	});

	server.on("/trim.service", []() {

	});

	server.onNotFound(handleNotFound);

	server.begin();
	Serial.println("HTTP server started");
}

#pragma endregion

#pragma region setup stuff

void setupWiFi()
{
	pinMode(led, OUTPUT);
	digitalWrite(led, 0);
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.println("");

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void setupFS()
{
	if (SPIFFS.begin())
		Serial.println("FS started");
	else
		Serial.println("FS Failed");
}

void setup()
{
	pinMode(led, OUTPUT);
	pinMode(PIN_RX, INPUT);
	pinMode(PIN_CW, OUTPUT);
	pinMode(PIN_TX, OUTPUT);

	Serial.begin(115200);

	delay(1000);
	Serial.println("Begining");
	delay(1000);

	blink(100, 200);
	blink(100, 600);

	setupWiFi();
	setupServer();
	setupFS();

	blink(100, 200);
	blink(100, 200);
	blink(100, 200);
}

#pragma endregion

void loop()
{
	recorderLoop();
	server.handleClient();
}
