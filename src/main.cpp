#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Wire.h>
#include <SPI.h>

#define ARRSIZE 100
#define DEVICE 0
#define SDA 6
#define SCL 7

const char *ssid = "SAM";
const char *password = "123456789";

WebServer server(80);
StaticJsonDocument<5000> jsonDocument;
char buffer[3000];

float x[ARRSIZE];
float y[ARRSIZE];
float z[ARRSIZE];

int64_t tim = 0;

TaskHandle_t Task1;
TaskHandle_t Task2;

Adafruit_BNO055 bno;

void push(float val, float *arreglo)
{
  for (int i = ARRSIZE - 2; i >= 0; i--)
  {
    arreglo[i + 1] = arreglo[i];
  }
  arreglo[0] = val;
}

void inicializar()
{
  Wire.setPins(SDA, SCL);
  Wire.begin();
  bno = Adafruit_BNO055();
  if (!bno.begin())
  {
    Serial.print("No BNO055 detected");
    while (1)
    {
      vTaskDelay(1000);
    }
  }
}

void aniadirArreglo(char *nombre, float *arreglo)
{
  StaticJsonDocument<2000> aux;
  JsonArray array = aux.to<JsonArray>();
  for (int i = 0; i < ARRSIZE; i++)
  {
    array.add(arreglo[i]);
  }
  jsonDocument[nombre] = array;
}

void leerIMU()
{
  sensors_event_t event;
  bno.getEvent(&event);
  push((float)event.orientation.x, x);
  push((float)event.orientation.y, y);
  push((float)event.orientation.z, z);
}

void getData()
{
  // Serial.println("Get data");
  jsonDocument.clear();
  jsonDocument["device"] = DEVICE;
  aniadirArreglo((char *)"x", x);
  aniadirArreglo((char *)"y", y);
  aniadirArreglo((char *)"z", z);
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
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void hardware(void *pvParameters)
{
  inicializar();
  for (;;)
  {
    leerIMU();
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