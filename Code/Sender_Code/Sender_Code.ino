#include <esp_now.h>
#include <WiFi.h>
#include "SSD1306Wire.h"
#include <Wire.h>

SSD1306Wire display(0x3C, 21, 22);//I2C Addresse, SDA, SCL

//MAC-Adresse des Empfängers
uint8_t broadcastAddress[] = {0x00, 0x70, 0x07, 0x83, 0x40, 0x30};

//Pins
#define TRIG 5
#define ECHO 19
#define GRUEN 18
#define BLAU 16
#define ROT 17

long dauer;
float entfernung;

//Struct für den Nachricht
typedef struct struct_message {
  float nachricht;
  String led_farbe;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  // WLAN in Station-Modus
  WiFi.mode(WIFI_STA);
  WiFi.begin("Magenta995041", "745hrga7dh5j");

  //ESP-NOW initialisieren
  if (esp_now_init()!=ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  //Peer hinzufügen
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT_PULLUP);
  pinMode(GRUEN, OUTPUT);
  pinMode(BLAU, OUTPUT);
  pinMode(ROT, OUTPUT);

  //LEDs aus
  digitalWrite(GRUEN, LOW);
  digitalWrite(BLAU, LOW);
  digitalWrite(ROT, LOW);

  Wire.begin(21, 22);
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.drawString(0, 0, "OLED OK");
  display.display();
  
  Serial.println("Setup complete");
}

void loop() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  dauer=pulseIn(ECHO, HIGH);

  //Entfernung in cm berechnen
  entfernung=dauer * 0.0343 / 2;

  if(entfernung>500){
    entfernung=500;
  }

  Serial.print("Entfernung: ");
  Serial.print(entfernung);
  Serial.println(" cm");
  Serial.println(myData.led_farbe);

  //Nachricht eingebem
  myData.nachricht = entfernung;



  //LEDs je nach Entfernung schalten
  if (entfernung<=20) {
    digitalWrite(GRUEN, LOW);
    digitalWrite(BLAU, LOW);
    digitalWrite(ROT, HIGH);
    myData.led_farbe="rot";
  }
  else if (entfernung>20 && entfernung <= 80) {
    digitalWrite(GRUEN, LOW);
    digitalWrite(BLAU, HIGH);
    digitalWrite(ROT, HIGH);
    myData.led_farbe="lila";
  } 
  else if (entfernung>80&&entfernung<=200){
    digitalWrite(GRUEN, HIGH);
    digitalWrite(BLAU, HIGH);
    digitalWrite(ROT, LOW);
    myData.led_farbe="türkis";
  }
  else if(entfernung>200){
    digitalWrite(GRUEN, HIGH);
    digitalWrite(BLAU, LOW);
    digitalWrite(ROT, LOW);
    myData.led_farbe="grün";
  }
  else{
    digitalWrite(GRUEN, HIGH);
    digitalWrite(BLAU, HIGH);
    digitalWrite(ROT, HIGH);
    myData.led_farbe="weiß";
  }

  esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));//Nachricht an Receiver versenden

  display.clear();

  display.drawString(0, 0, "Distanz: " +String(entfernung)+" cm");
  display.drawString(0, 20, "Farbe: " +String(myData.led_farbe));
  
  display.display();

  delay(500); //Sendeintervall

}