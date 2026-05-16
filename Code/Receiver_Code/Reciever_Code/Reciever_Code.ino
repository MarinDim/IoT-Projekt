#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include "time.h"
#include "esp_sntp.h"
#include <WiFiClientSecure.h>

WebServer server(80);

#define MAX_POINTS 100
#define buzz 4

float werte[MAX_POINTS];
unsigned long zeit[MAX_POINTS];

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";

const char* webhook="https://discordapp.com/api/webhooks/1504861498692866120/FSqzfR1dMzNJts7BotexuINVgoxDeecL-PoU1rrs06DmSczc3RoOdSQrZVrkfZ2Hkhaa";

int indexWert=0;
int discordZeit=0;

typedef struct struct_message {
  float nachricht;
  String led_farbe;
} struct_message;

struct_message myData;

//Um den Timestamp zu bekommen
String printLocalTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "Fehler";
  }

  char zeit[30];

  strftime(zeit, sizeof(zeit), "%Y-%m-%d %H:%M:%S", &timeinfo);

  return String(zeit);
}


void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}


//Um Werte im Graphen zu speichern
void addValue(float value) {
  werte[indexWert]=value;
  zeit[indexWert]=millis();

  indexWert++;

  if (indexWert>=MAX_POINTS) {
    indexWert=0;
  }
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));

  addValue(myData.nachricht);
  if(false){
    Serial.print("Distanz: ");
    Serial.println(myData.nachricht);
  }
}


//Webseite
void graphZeichnen() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
<canvas id="chart"></canvas>

<script>
const ctx = document.getElementById('chart');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [{
      label: 'Entfernung',
      data: []
    }]
  },
  options: {
    scales: {
      y: {
        min:0,
        max :500
      }
    }
  }
});

async function updateChart() {
  const res = await fetch('/api/data');
  const d = await res.json();

  chart.data.labels.push(d.timestamp);
  chart.data.datasets[0].data.push(d.entfernung);

  if (chart.data.labels.length > 20) {
    chart.data.labels.shift();
    chart.data.datasets[0].data.shift();
  }

  chart.update();
}

setInterval(updateChart, 1000);
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}


void setup() {
  Serial.begin(115200);
  pinMode(buzz, OUTPUT);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin("Magenta995041", "745hrga7dh5j");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //IP addresse ausgeben
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Fehler");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  //Webserver
  server.on("/", graphZeichnen);
  

  server.on("/api/data", [](){
  String json="{";
  json+="\"timestamp\":\""+printLocalTime() +"\",";
  json+="\"entfernung\":"+String(myData.nachricht)+ ",";
  json+="\"farbe\":\""+String(myData.led_farbe)+"\"";
  json+="}";
  server.send(200, "application/json", json);
});

  server.begin();
}
void discordNachricht(String nachricht){
  if(WiFi.status()==WL_CONNECTED){
    WiFiClientSecure client;
    client.setInsecure();


    HTTPClient http;

    http.begin(client, webhook);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"content\": \"Entfernung: " + nachricht + " cm\"}";

    int httpAntwort=http.POST(json);

    Serial.print("\nHTTP Anwort: " + String(http.POST(json)));
    http.end();
  }
}


void loop() {
  if(millis()-discordZeit>10000){//Nur jede 10 Sekunden
    discordNachricht(String(myData.nachricht));
    discordZeit=millis();
  }
  if(myData.nachricht<50){
    tone(buzz, 1000);
  }
  else{
    noTone(buzz);
  }
  
  server.handleClient();
}