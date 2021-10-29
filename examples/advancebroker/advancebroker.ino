#include<sMQTTBroker.h>

class MyBroker:public sMQTTBroker
{
public:
    bool onConnect(sMQTTClient *client, const std::string &username, const std::string &password)
    {
        // check username and password, if ok return true
        return true;
    };
	void onRemove(sMQTTClient*)
    {
    };
    void onPublish(const std::string &topic, const std::string &payload)
    {
        // client publish a message to the topic
        // you can parse the message, save it to a database, or whatever, based on your goals
    }
};

MyBroker broker;

void setup()
{
    Serial.begin(115200);
    const char* ssid = "SSID";         // The SSID (name) of the Wi-Fi network you want to connect to
    const char* password = "PASSWORD"; // The password of the Wi-Fi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
    }
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    
    const unsigned short mqttPort=1883;
    broker.init(mqttPort);
    // all done
    // your magic code
};
void loop()
{
    broker.update();
    // your magic code
};