#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFi.h>
#include "fauxmoESP.h"
#include "BluetoothSerial.h"

#define app1 "luz quarto"
#define app2 "tv"

const char* ssid;
const char *password;

const uint16_t irLedSend = 4;
bool isEnviarSinal = false;

IRsend irsend(irLedSend);
fauxmoESP fauxmo;
BluetoothSerial SerialBT;

void wifiSetup() {
  WiFi.mode(WIFI_STA);
  Serial.printf("[WIFI] Connecting to %s ", ssid);
  Serial.println(password);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void initFauxmo() {
  fauxmo.createServer(true); 
  fauxmo.setPort(80); 
  fauxmo.enable(true);
  fauxmo.addDevice(app1);
  fauxmo.addDevice(app2);

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {    
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    if ( (strcmp(device_name, app1) == 0) ) {
      Serial.println("RELAY 1 switched by Alexa");
      if (state) {
        digitalWrite(12, LOW);
      } else {
        digitalWrite(12, HIGH);
      }
    }
    if ( (strcmp(device_name, app2) == 0) ) {
      Serial.println("RELAY 2 switched by Alexa");
      isEnviarSinal = true;
    }
  });
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  irsend.begin();

  SerialBT.begin("ESP32 Gian Teste");
  Serial.println("ESP32 INICIADO");

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

}

void loop() {
  fauxmo.handle();
  delay(500);

  if(isEnviarSinal) {
    Serial.println("---> Comando IR");
    irsend.sendNEC(0x20DF10EF);
    delay(500);
    isEnviarSinal = false;
  }

  if(Serial.available()){
    SerialBT.write(Serial.read());
  }

  if(SerialBT.available()) {
    String serialBT = SerialBT.readString();
    if (serialBT.startsWith("CONW")) {
      String msgConexao = serialBT.substring(4);
      int posSeparador = msgConexao.indexOf(":");
      String rede = msgConexao.substring(0, posSeparador);
      String senha = msgConexao.substring(posSeparador+1, msgConexao.length()-1);
      senha.trim();
      ssid = rede.c_str();
      password = senha.c_str();
      
      wifiSetup();
      initFauxmo();
    }
    
    //Serial.write(SerialBT.read());
  }
}