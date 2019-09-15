#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <LinkedList.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

struct ListenerValuePair
{
    String key;
    void (*callback)(String data);
};

class DweetStream
{
private:
    String sessionID;
    WiFiClient client;
    byte mode;
    byte cntDown;

    bool fin;
    byte opcode;
    bool mask;
    uint64_t len;
    uint64_t ilen;
    char *buff;
    String msg;
    LinkedList<ListenerValuePair *> listeners = LinkedList<ListenerValuePair *>();

    bool ready = false;

    void incomming(String msg)
    {
        // Serial.println("MCU <-- SRV");
        // Serial.println(msg);
        bool doSend = false;
        String sendMsg;

        if (msg == "1::")
        {
            doSend = true;
            sendMsg = "1::/stream";
        }
        else if (msg == "1::/stream")
        {
            ready = true;
            //doSend = true;
            //Serial.println("Listening");
        }
        else if (msg == "2::")
        {
            doSend = true;
            sendMsg = "2::";
        }
        else if (msg.startsWith("5::/stream:"))
        {
            //Serial.println("IN");
            inStream(msg.substring(11));
        }

        if (doSend)
        {
            //Serial.println("MCU --> SRV");
            //Serial.println(sendMsg);
            write(sendMsg);
        }
    }

    void inStream(String msg)
    {
        //StaticJsonDocument<1000> doc;
        DynamicJsonDocument doc(4096);
        deserializeJson(doc,msg);
        String k = doc["name"];

        if(k=="new_dweet")
        {
            JsonArray arr = doc["args"].as<JsonArray>();
            for(int i=0; i<arr.size();i++)
            {
                JsonObject item = arr[i];
                String thing = item["thing"];
                String content = "";
                serializeJson(item["content"], content);
                for(int j=0;j<listeners.size(); j++)
                {
                    ListenerValuePair *lvp = listeners.get(i);
                    if(lvp->key == thing)
                    {
                        lvp->callback(content);
                    }
                }
            }
        }
        //Serial.println(doc["args"][0]["thing"]);
    }
    void write(String msg)
    {
        int lenMax = 0x7F;
        int msgLen = msg.length();
        int i = 0;
        while (msgLen > 0)
        {
            int sendLen = lenMax;

            if (sendLen > msgLen)
                sendLen = msgLen;

            int isendLen = sendLen;

            uint8_t *writeBuff = new uint8_t[sendLen + 2];

            //OpCode
            writeBuff[0] = 1;

            //Len
            writeBuff[1] = sendLen;

            //Fin flag
            if (msgLen <= lenMax)
                writeBuff[0] |= 0x80;

            while (sendLen-- > 0)
            {
                writeBuff[i + 2] = (uint8_t)msg.charAt(i);
                i++;
                msgLen--;
            }
            client.write(writeBuff, isendLen + 2);
            // Serial.print("DEBUG len: ");
            // Serial.println(isendLen);

            delete[] writeBuff;
        }
    }

public:
    void getSession()
    {
        WiFiClient client;
        HTTPClient http;

        Serial.print("[HTTP] begin...\n");
        if (http.begin(client, "http://dweet.io/socket.io/1/"))
        {
            Serial.print("[HTTP] GET...\n");
            int httpCode = http.GET();

            if (httpCode > 0)
            {
                Serial.printf("[HTTP] GET... code: %d\n", httpCode);

                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    String payload = http.getString();
                    Serial.println(payload);
                    this->sessionID = "";
                    int i;
                    char c;
                    for (i = 0; i < payload.length(); i++)
                    {
                        c = payload.charAt(i);
                        if (c == ':')
                            break;
                        this->sessionID += c;
                    }
                    if (i == payload.length())
                        this->sessionID = "";

                    Serial.println(this->sessionID);
                }
            }
            else
            {
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        }
        else
        {
            Serial.printf("[HTTP} Unable to connect\n");
        }
    }

    void connect()
    {
        mode = 0;
        String ws_link = "http://dweet.io/socket.io/1/websocket/" + this->sessionID;

        //Serial.println(ws_link);

        if (!client.connect("dweet.io", 80))
        {
            Serial.println("Connection failed");
            return;
        }

        while (!client.connected())
            ;
        String handShakePacket = "";
        handShakePacket += "GET /socket.io/1/websocket/" + this->sessionID + " HTTP/1.1\r\n";
        handShakePacket += "Upgrade: websocket\r\n";
        handShakePacket += "Connection: Upgrade\r\n";
        handShakePacket += "Sec-WebSocket-Version: 13\r\n";
        handShakePacket += "Sec-WebSocket-Key: 7VYhOKhSt+Pygr9Ek6z+9A==\r\n";
        handShakePacket += "Host: dweet.io\r\n";
        handShakePacket += "\r\n";
        client.print(handShakePacket);

        int avl = 0;
        for (; avl == 0; avl = client.available())
            ;

        while (client.available() == 0)
            ;

        char l1, l2, l3, l4;

        while (client.available() > 0)
        {
            l1 = l2;
            l2 = l3;
            l3 = l4;
            l4 = (char)client.read();
            //Serial.print(l4);

            if (l1 == '\r' && l3 == '\r' && l2 == '\n' && l4 == '\n')
                break;
            while (client.available() == 0)
                ;
        }

        //Serial.println(data1);
        //Serial.println("Looping");

        while (!ready)
            loop();
    }
    void loop()
    {
        byte l4;

        //String dbuff = "";

        while (client.available() > 0)
        {
            l4 = (byte)client.read();
            //dbuff += (char)l4;
            switch (mode)
            {
            case 0:
                // Fin Flag & opcode
                fin = ((0x80 & l4) > 0);
                opcode = (0x07 & l4);
                mode = 1;
                msg = "";
                break;
            case 1:
                // mask & len
                mask = ((0x80 & l4) > 0);
                len = (0x7F & l4);
                ilen = 0;
                if (len == 126)
                {
                    mode = 11;
                    cntDown = 2;
                    len = 0;
                    break;
                }
                else if (len == 127)
                {
                    mode = 12;
                    cntDown = 8;
                    len = 0;
                    break;
                }
                mode = 2;
                buff = new char[len + 1];
                buff[len] = 0;
                break;

            case 11:
                // len is 126 which marks next two bytes to be the length
            case 12:
                // len is 127 which marks next 8 bytes to be the length

                len <<= 8;
                len |= l4;
                cntDown--;
                if (cntDown == 0)
                {
                    mode = 2;
                    buff = new char[len + 1];
                    buff[len] = 0;
                }

                break;

            case 2:
                buff[ilen] = (char)l4;
                ilen++;
                if (ilen >= len)
                {
                    mode = 0;
                    // Serial.print("Fin: ");
                    // Serial.println(fin);

                    // Serial.print("Opcode: ");
                    // Serial.println(opcode);

                    // Serial.print("Mask: ");
                    // Serial.println(mask);

                    // Serial.print("Len: ");
                    // Serial.println((unsigned long)len);

                    // Serial.print("Msg: \"");
                    // Serial.print(buff);
                    // Serial.println("\"");
                    msg += buff;
                    if (fin)
                    {
                        incomming(msg);
                        msg = "";
                    }

                    delete[] buff;
                }

                break;
            }
        }

        // if (dbuff != "")
        // {
        //     Serial.print("DEBUG : ");
        //     Serial.println(dbuff);
        //     for (int i = 0; i < 5; i++)
        //     {
        //         Serial.print(dbuff.charAt(i));
        //         Serial.print(" - ");
        //         Serial.println((int)dbuff.charAt(i));
        //     }
        // }
    }

    void listenFor(String name, void (*callback)(String data))
    {
        ListenerValuePair *lvp = new ListenerValuePair();

        lvp->callback = callback;
        lvp->key = name;

        listeners.add(lvp);

        String sendMsg = "5::/stream:{\"name\":\"subscribe\",\"args\":[{\"thing\":\"" + name + "\",\"key\":null}]}";
        write(sendMsg);
    }

    ~DweetStream()
    {
    }
};

DweetStream ds;

void setup()
{

    Serial.begin(115200);
    // Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();

    for (uint8_t t = 4; t > 0; t--)
    {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    Serial.println("Set wifi mode");
    WiFi.mode(WIFI_STA);
    Serial.println("Connecting Wifi");
    WiFi.begin("T22 902", "c0a8b74282");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected Wifi");

    Serial.println("Getting session");
    ds.getSession();

    Serial.println("Connecting");
    ds.connect();
    Serial.println("Connected");
    ds.listenFor("my-thing-name", [](String msg) {
        Serial.println("Output");
        Serial.println(msg);
    });
}

void loop()
{
    ds.loop();
}
