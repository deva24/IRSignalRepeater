#include <Arduino.h>
#line 1 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LinkedList.h>

#ifndef STASSID
#define STASSID "T22 902"
#define STAPSK "c0a8b74282"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

#pragma region led blink code
const int led = 2;

#line 20 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void blink(unsigned long wait_on, unsigned long wait_off);
#line 42 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void rx_isr();
#line 52 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void recorderLoop();
#line 69 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void handle_record_service();
#line 75 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void endofRecord();
#line 85 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void handle_fetch_service();
#line 104 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void handle_playback_service();
#line 110 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
String determineContentType(String url);
#line 138 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void handleNotFound();
#line 165 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void setupServer();
#line 204 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void setupWiFi();
#line 226 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void setupFS();
#line 234 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void setup();
#line 246 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void loop();
#line 20 "c:\\Users\\Deva\\Desktop\\Projects\\SigRepeat\\PlayMemories_Home.lnk\\PlayMemories_Home.lnk.ino"
void blink(unsigned long wait_on, unsigned long wait_off)
{
    digitalWrite(led, LOW);
    delay(wait_on);
    digitalWrite(led, HIGH);
    delay(wait_off);
}

#pragma endregion

#pragma region Recorder config

#define PIN_RX D3
#define PIN_CW D1
#define PIN_TX D2
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
    blink(500, 500);
}

#pragma endregion

#pragma region fetch service

void handle_fetch_service()
{
    String ret = "{timecode:[";

    int len = signalBuffer.size();
    for(int i=0; i<len; i++)
    {
        if(i>0)ret+=",";
        ret+= String(signalBuffer.get(i));
    }

    ret += "]}";

    server.send(200,"application/json",ret);
}

#pragma endregion

#pragma region handle record service
void handle_playback_service()
{
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
    Serial.begin(115200);

    setupWiFi();
    setupServer();
    setupFS();
}

#pragma endregion

void loop()
{
    recorderLoop();
    server.handleClient();
}
