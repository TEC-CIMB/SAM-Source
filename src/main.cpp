#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define ARRSIZE 10
#define VOLT 2

const char *ssid = "SAM";
const char *password = "123456789";

WebServer server(80);
StaticJsonDocument<3000> jsonDocument;
char buffer[3000];

int volt[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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
  pinMode(VOLT, INPUT_PULLUP);
}

void aniadirArreglo(char *nombre, int *arreglo)
{
  StaticJsonDocument<200> aux;
  JsonArray array = aux.to<JsonArray>();
  for (int i = 0; i < ARRSIZE; i++)
  {
    array.add(arreglo[i]);
  }
  jsonDocument[nombre] = array;
}

void leerVoltaje(int pin, int *arreglo)
{
  push(digitalRead(pin), arreglo);
}

void getData()
{
  Serial.println("Get data");
  jsonDocument.clear();
  aniadirArreglo((char *)"volt", volt);
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void post()
{
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
  IPAddress IP = WiFi.localIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.enableCORS();
  server.on("/data", getData);
  server.on("/servo", HTTP_POST, post);
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