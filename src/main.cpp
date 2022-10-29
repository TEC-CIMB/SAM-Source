#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>

#define ARRSIZE 60
#define VOLT 2
#define DEVICE 1
#define BUTTON 4

const char *ssid = "SAM";
const char *password = "123456789";

WebServer server(80);
StaticJsonDocument<3000> jsonDocument;
char buffer[3000];

int volt[ARRSIZE];
int64_t tim = 0;
int64_t but = 0;
TaskHandle_t Task1;
TaskHandle_t Task2;

void push(int val, int *arreglo)
{
  for (int i = ARRSIZE - 2; i >= 0; i--)
  {
    arreglo[i + 1] = arreglo[i];
  }
  arreglo[0] = val;
}

void inicializarPines()
{
  pinMode(BUTTON, INPUT_PULLUP);
}

void aniadirArreglo(char *nombre, int *arreglo)
{
  StaticJsonDocument<2000> aux;
  JsonArray array = aux.to<JsonArray>();
  for (int i = 0; i < ARRSIZE; i++)
  {
    array.add(arreglo[i]);
  }
  jsonDocument[nombre] = array;
}

void leerVoltaje(int pin, int *arreglo)
{
  push(analogRead(pin), arreglo);
}

void getData()
{
  // Serial.println("Get data");
  jsonDocument.clear();
  jsonDocument["device"] = DEVICE;
  jsonDocument["time"] = but;
  aniadirArreglo((char *)"volt", volt);
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void sync()
{
  // esp wifi get tsf time
  tim = esp_wifi_get_tsf_time(WIFI_IF_STA) / 1000 - millis();
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  // Execute after post
  server.send(200, "application/json", "{}");
}

void wifi(void *pvParameters)
{

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(".");
  String name = "sam" + String(DEVICE);
  if (!MDNS.begin(name.c_str()))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  IPAddress IP = WiFi.localIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.enableCORS();
  server.on("/data", getData);
  server.on("/sync", HTTP_POST, sync);
  server.begin();
  for (;;)
  {
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void hardware(void *pvParameters)
{
  inicializarPines();
  for (;;)
  {
    leerVoltaje(VOLT, volt);
    if (digitalRead(BUTTON) == LOW)
    {
      but = tim + millis();
      Serial.print("Button pressed at: ");
      Serial.println(but);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  xTaskCreatePinnedToCore(
      wifi,   /* Task function. */
      "wifi", /* name of task. */
      10000,  /* Stack size of task */
      NULL,   /* parameter of the task */
      1,      /* priority of the task */
      &Task1, /* Task handle to keep track of created task */
      0);     /* pin task to core 0 */

  xTaskCreatePinnedToCore(
      hardware,   /* Task function. */
      "hardware", /* name of task. */
      10000,      /* Stack size of task */
      NULL,       /* parameter of the task */
      1,          /* priority of the task */
      &Task2,     /* Task handle to keep track of created task */
      0);         /* pin task to core 1 */
}

void loop()
{
}