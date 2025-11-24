#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>

#define JOY_X         32
#define JOY_Y         33
#define TRIGGER_PIN   18
#define RELOAD_PIN    19
#define VIBRATION_PIN 25

const char* ssid = "GunController";
const char* password = "12345678";  // No password for open AP

WiFiServer server(80);

MPU6050 gyro;

int ammo = 10;
const int MAX_AMMO = 10;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  gyro.initialize();

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(RELOAD_PIN, INPUT_PULLUP);
  pinMode(VIBRATION_PIN, OUTPUT);

  // Start WiFi Access Point
  Serial.println("Starting WiFi AP...");
  WiFi.softAP(ssid, password);
  delay(100);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  Serial.print("SSID: ");
  Serial.print(ssid);
  Serial.print("Password: ");
  Serial.print(password);

  server.begin();
  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();
  
  int joyX = analogRead(JOY_X);
  int joyY = analogRead(JOY_Y);

  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  gyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  bool triggerPressed = digitalRead(TRIGGER_PIN) == LOW;
  bool reloadPressed = digitalRead(RELOAD_PIN) == LOW;

  if (triggerPressed && ammo > 0) {
    fireGun();
  }

  if (reloadPressed) {
    reloadWeapon();
  }

  // Format data as JSON for easier parsing
  String data = "{\"joyX\":" + String(joyX) + 
                ",\"joyY\":" + String(joyY) + 
                ",\"gx\":" + String(gx) + 
                ",\"gy\":" + String(gy) + 
                ",\"gz\":" + String(gz) +
                ",\"trigger\":" + String(triggerPressed ? 1 : 0) + 
                ",\"reload\":" + String(reloadPressed ? 1 : 0) + 
                ",\"ammo\":" + String(ammo) + "}";

  if (client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println(data);
    client.stop();
  }

  Serial.println(data);
  delay(50);
}

void fireGun() {
  digitalWrite(VIBRATION_PIN, HIGH);
  delay(200);
  digitalWrite(VIBRATION_PIN, LOW);
  ammo--;
}

void reloadWeapon() {
  ammo = MAX_AMMO;
}
