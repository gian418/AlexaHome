#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WiFi.h>
#include "fauxmoESP.h"
#include "BluetoothSerial.h"
#include <SPIFFS.h>

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
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Nao foi possivel conectar");
    }

    if (WiFi.status() == WL_NO_SSID_AVAIL) {
      Serial.println("O SSID informado nao foi encontrado");
    }

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

void informacoesTamanhoSPIFFS() {
  Serial.println("\nInformações ------------------------------");
  Serial.printf("totalBytes: %u\nusedBytes: %u\nfreeBytes: %u\n",
                  SPIFFS.totalBytes(),
                  SPIFFS.usedBytes(),
                  SPIFFS.totalBytes() - SPIFFS.usedBytes());
}

void listarArquivos() {
  Serial.println("\nArquivos ---------------------------------");
  File dir = SPIFFS.open("/");
  File file = dir.openNextFile();
  while (file) {
    Serial.printf(" %s - %u bytes\n", file.name(), file.size());
    file = dir.openNextFile();
  }
}

void gravarArquivoTeste() {
  Serial.println("\nGravando arquivo de teste -------------------------------");
  File file = SPIFFS.open("/teste.txt", "a");
  file.println("Linha 1");
  file.printf("Linha 2");
  file.close();
}

void lerArquivoTeste() {
  Serial.println("\nLendo o arquivo de rede -------------------------------");
  File file = SPIFFS.open("/rede.txt", "r");
  Serial.printf("Nome: %s - %u bytes\n", file.name(), file.size());
  while (file.available())
  {
    Serial.println(file.readStringUntil('\n'));
  }
  file.close();
  
}

void excluirArquivoTeste() {
  Serial.println("\nExcluindo Arquivo ----------------------------");
  if (SPIFFS.remove("/teste.txt")) {
    Serial.println("Arquivo '/teste.txt' excluído");
  } else {
    Serial.println("Exclusão '/teste.txt' falhou");
  }
}

void salvarConfigWifi(String rede, String senha) {
  Serial.println("\n--- Excluindo configuracao rede anterior para receber a nova ---");

  if (SPIFFS.remove("/rede.txt")) {
    Serial.println("Arquivo '/rede.txt' excluído");
  } else {
    Serial.println("Exclusão '/rede.txt' falhou. Arquivo não existe ou houve uma falha momentanea.");
  }

  Serial.println("\n--- Salvando credenciais da rede wifi ---");
  File file = SPIFFS.open("/rede.txt", "a");
  file.println(rede);
  file.print(senha);
}

void carregarConfigWifi() {
  Serial.println("\n--- Carregando config de rede e efetuando conexao ---");
  File file = SPIFFS.open("/rede.txt", "r");
  Serial.printf("Nome: %s - %u bytes\n", file.name(), file.size());

  int i = 1;
  while (file.available())
  {
    String linha = file.readStringUntil('\n');
    linha.trim();
    
    if (i == 1){
      Serial.println("REDE: " + linha);
      ssid = linha.c_str();
    }
    if (i == 2){
      Serial.println("SENHA: " + linha);
      password = linha.c_str();
    }
    if (i > 2){
      break;
    }

    i++;
  }

  file.close();

  wifiSetup();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  irsend.begin();

  SerialBT.begin("ESP32 Gian Teste");
  Serial.println("ESP32 INICIADO");

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS: erro ao iniciar");
  } 

  informacoesTamanhoSPIFFS();
  listarArquivos();
  //lerArquivoTeste();
  //excluirArquivoTeste();

}

void loop() {

  salvarConfigWifi("Fobos", "pw@56789");
  lerArquivoTeste();
  carregarConfigWifi();
  delay(100000);

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