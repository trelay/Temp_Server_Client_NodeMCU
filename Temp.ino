#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#define ONE_WIRE_BUS D4

//Common setting for both client and server.
const char* ssid = "Trelay";
const char* password = "KW83ca<M";
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
char local_ip[16];
IPAddress _ip_address;

//Setting for Server
ESP8266WebServer server(80);
char temperatureString[6];

//Setting for client
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "192.168.0.106";
const char* outTopic="TEMP";
long lastMsg = 0;
char msg[50];

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
    snprintf (msg, 50, "%s:%s", local_ip, "Start");
      client.publish(outTopic, msg);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float getTemperature() {
  float temp;
  
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  
  return temp;
}

void setup(void){
  //Pre-setting: Serial output, Wifi
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  
  _ip_address = WiFi.localIP();
  sprintf(local_ip, "%d.%d.%d.%d",_ip_address[0],_ip_address[1],_ip_address[2],_ip_address[3]);
  Serial.println(local_ip);  
  
  //Server Setting
  server.on("/", []() {
    float temperature = getTemperature();
    dtostrf(temperature, 2, 2, temperatureString);
  
    String title = "Temperature";
    String cssClass = "mediumhot";
  
    if (temperature < 18)
      cssClass = "cold";
    else if (temperature > 25)
      cssClass = "hot";
  
    String message = "<!DOCTYPE html><html><head><title>" + title + "</title><meta charset=\"utf-8\" /><meta name=\"viewport\" content=\"width=device-width\" /><link href='https://fonts.googleapis.com/css?family=Advent+Pro' rel=\"stylesheet\" type=\"text/css\"><style>\n";
    message += "html {height: 100%;}";
    message += "div {color: #fff;font-family: 'Advent Pro';font-weight: 400;left: 50%;position: absolute;text-align: center;top: 50%;transform: translateX(-50%) translateY(-50%);}";
    message += "h2 {font-size: 90px;font-weight: 400; margin: 0}";
    message += "body {height: 100%;}";
    message += ".cold {background: linear-gradient(to bottom, #7abcff, #0665e0 );}";
    message += ".mediumhot {background: linear-gradient(to bottom, #81ef85,#057003);}";
    message += ".hot {background: linear-gradient(to bottom, #fcdb88,#d32106);}";
    message += "</style></head><body class=\"" + cssClass + "\"><div><h1>" + title +  "</h1><h2>" + temperatureString + "&nbsp;<small>&deg;C</small></h2></div></body></html>";
    
    server.send(200, "text/html", message);
  });
  server.begin();
  Serial.println("Temperature web server started!");
  
  //MQTT Client setting
  client.setServer(mqtt_server, 1883);
}

void publish_temp(long now){
  lastMsg = now;
  
  float temperature = getTemperature();
  dtostrf(temperature, 2, 2, temperatureString);
  snprintf (msg, 50, "%s:%s", local_ip, temperatureString);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(outTopic, msg);
}

void loop() {
  //loop for web server
  server.handleClient();

  //loop for MQTT Client
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 30000) 
    publish_temp(now);
}




