#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
const char* ssid = "OPPO A96";        
const char* password = "d9desk2t";     
#define BUTTON_PIN 14    
#define LED_PIN 2      
#define SDA_PIN 16      
#define SCL_PIN 17     

WebServer server(80);
WebSocketsClient webSocket;
bool wifiConnected = false;
bool mpuReady = false;
bool useDummyGPS = true; 
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);
// Button variables
int buttonCount = 0;
bool lastButtonState = HIGH;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 50;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 3000;
// const char* websocket_host = "192.168.101.7";
// const uint16_t websocket_port = 3001;        
// const char* websocket_path = "/";       
const char* websocket_host = "caremates-websocket.codebloop.my.id";
const uint16_t websocket_port = 443;              // default HTTPS/WSS port
const char* websocket_path = "/";                 

//Atur-atur 
const char* serverUrl = "wss://web-sockets-caremates-production.up.railway.app/esp32";


// Sensor data
struct SensorData {
    float accelX = 0, accelY = 0, accelZ = 0;
    float gyroX = 0, gyroY = 0, gyroZ = 0;
    float temperature = 0;
    bool buttonPressed = false;
    int buttonCount = 0;
    unsigned long timestamp = 0;
    float lastLat = 0.0;
    float lastLon = 0.0;
} sensorData;
void sendSensorDataViaHTTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected, can't send data");
        return;
    }

    HTTPClient http;
    http.begin(serverUrl);  
    http.addHeader("Content-Type", "application/json");
    WiFiClientSecure client;
    client.setInsecure(); //
    StaticJsonDocument<512> doc;

    doc["accelX"] = sensorData.accelX;
    doc["accelY"] = sensorData.accelY;
    doc["accelZ"] = sensorData.accelZ;
    doc["gyroX"] = sensorData.gyroX;
    doc["gyroY"] = sensorData.gyroY;
    doc["gyroZ"] = sensorData.gyroZ;
    doc["temperature"] = sensorData.temperature;
    doc["buttonPressed"] = sensorData.buttonPressed;
    doc["buttonCount"] = sensorData.buttonCount;
    doc["timestamp"] = sensorData.timestamp;
    doc["lastLat"] = sensorData.lastLat;
    doc["lastLon"] = sensorData.lastLon;

    String jsonStr;
    serializeJson(doc, jsonStr);

    int httpResponseCode = http.POST(jsonStr);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("HTTP POST Response code: %d\n", httpResponseCode);
        Serial.println("Response: " + response);
    } else {
        Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}

void setupWebSocket() {
    const char* websocket_host = "caremates-websocket.codebloop.my.id";  
    const int websocket_port = 443;                 
    const char* websocket_path = "/";  
    webSocket.beginSSL(websocket_host, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected!");
      break;
    case WStype_TEXT:
      Serial.printf("Received: %s\n", payload);
      break;
  }
}
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("==========================================");
    Serial.println("🚀 ESP32 Simple IoT System");
    Serial.println("==========================================");
    // Setup pins
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("📌 Hardware Setup:");
    Serial.printf("   Button: GPIO %d → GND\n", BUTTON_PIN);
    Serial.printf("   MPU6050 SDA: GPIO %d\n", SDA_PIN);
    Serial.printf("   MPU6050 SCL: GPIO %d\n", SCL_PIN);
    
    // Initialize MPU6050
    initMPU6050();
    if (!useDummyGPS) {
        SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
    }
    // Connect WiFi
    connectWiFi();
    // Setup Web Server
    setupWebSocket();
    // webSocket.begin(websocket_host, websocket_port, websocket_path);
    // webSocket.onEvent(webSocketEvent);
    // webSocket.setReconnectInterval(5000);
    
    Serial.println("==========================================");
    Serial.println("✅ System Ready!");
    if (wifiConnected) {
        Serial.println("🌐 Web Dashboard: http://" + WiFi.localIP().toString());
    }
    Serial.println("🔘 Press button to test!");
    Serial.println("==========================================");
}

void initMPU6050() {
    Serial.println("\n📊 Initializing MPU6050...");
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    byte address = 0x68;
    // Wake up MPU6050
    Wire.beginTransmission(address);
    Wire.write(0x6B); // Power management
    Wire.write(0x00); // Wake up
    byte error = Wire.endTransmission();
    if (error == 0) {
        delay(100);
        // Test reading
        Wire.beginTransmission(address);
        Wire.write(0x3B); // Accelerometer data
        Wire.endTransmission();
        
        Wire.requestFrom(address, (byte)6);
        if (Wire.available() >= 6) {
            mpuReady = true;
            Serial.println("✅ MPU6050 Ready!");
        } else {
            Serial.println("❌ MPU6050 read test failed");
        }
    } else {
        Serial.println("❌ MPU6050 not responding");
    }
}
void connectWiFi() {
    Serial.println("\n📡 Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("\n✅ WiFi Connected!");
        Serial.println("📍 IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n❌ WiFi Failed!");
        digitalWrite(LED_PIN, LOW);
    }
}

void readMPU6050() {
    if (!mpuReady) return;
    
    byte address = 0x68;
    
    // Read accelerometer, temperature, and gyroscope
    Wire.beginTransmission(address);
    Wire.write(0x3B); // Starting register
    Wire.endTransmission();
    
    Wire.requestFrom(address, (byte)14);
    
    if (Wire.available() >= 14) {
        // Read raw data
        int16_t ax = (Wire.read() << 8) | Wire.read();
        int16_t ay = (Wire.read() << 8) | Wire.read();
        int16_t az = (Wire.read() << 8) | Wire.read();
        int16_t temp = (Wire.read() << 8) | Wire.read();
        int16_t gx = (Wire.read() << 8) | Wire.read();
        int16_t gy = (Wire.read() << 8) | Wire.read();
        int16_t gz = (Wire.read() << 8) | Wire.read();
        
        // Convert to real units
        sensorData.accelX = ax / 16384.0 * 9.81; // Convert to m/s²
        sensorData.accelY = ay / 16384.0 * 9.81;
        sensorData.accelZ = az / 16384.0 * 9.81;
        
        sensorData.gyroX = gx / 131.0; // Convert to °/s
        sensorData.gyroY = gy / 131.0;
        sensorData.gyroZ = gz / 131.0;
        
        sensorData.temperature = (temp / 340.0) + 36.53; // Convert to °C
        sensorData.timestamp = millis();
    }
}

void checkButton() {
    bool currentState = digitalRead(BUTTON_PIN);
    sensorData.buttonPressed = (currentState == LOW);
    
    // Detect button press events (with debouncing)
    if (currentState != lastButtonState) {
        if (millis() - lastButtonPress > debounceDelay) {
            if (currentState == LOW) {
                // Button pressed
                buttonCount++;
                sensorData.buttonCount = buttonCount;
                lastButtonPress = millis();
                
                Serial.printf("🔘 BUTTON PRESSED! Count: %d\n", buttonCount);
                Serial.println("   📊 Current sensor data:");
                if (mpuReady) {
                    Serial.printf("   🏃 Accel: X=%.2f Y=%.2f Z=%.2f m/s²\n", 
                                 sensorData.accelX, sensorData.accelY, sensorData.accelZ);
                    Serial.printf("   🌡️ Temp: %.1f°C\n", sensorData.temperature);
                    
                } else {
                    Serial.println("   ⚠️ MPU6050 not available");
                }
                // LED feedback
                for (int i = 0; i < 3; i++) {
                    digitalWrite(LED_PIN, LOW);
                    delay(50);
                    digitalWrite(LED_PIN, HIGH);
                    delay(50);
                }
                
            }
        }
        lastButtonState = currentState;
    }
}
void sendSensorDataViaWebSocket() {
    if (!webSocket.isConnected()) return;

    StaticJsonDocument<512> doc;

    doc["accelX"] = sensorData.accelX;
    doc["accelY"] = sensorData.accelY;
    doc["accelZ"] = sensorData.accelZ;
    doc["gyroX"] = sensorData.gyroX;
    doc["gyroY"] = sensorData.gyroY;
    doc["gyroZ"] = sensorData.gyroZ;
    doc["temperature"] = sensorData.temperature;
    doc["buttonPressed"] = sensorData.buttonPressed;
    doc["buttonCount"] = sensorData.buttonCount;
    doc["timestamp"] = sensorData.timestamp;
    doc["lastLat"] = sensorData.lastLat;
    doc["lastLon"] = sensorData.lastLon;

    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
}


void loop() {
    webSocket.loop();
    if (wifiConnected) {
        server.handleClient();
    }
    readMPU6050();
    checkButton();
    updateGPS();
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus >= 5000) {
        sendSensorDataViaWebSocket();
        // sendSensorDataViaHTTP();
        lastStatus = millis();
        Serial.println("💓 System Status:");
        Serial.printf("   ⏱️ Uptime: %lu seconds\n", millis() / 1000);
        Serial.printf("   🔘 Button: %s (Count: %d)\n", 
                     sensorData.buttonPressed ? "PRESSED" : "Released", buttonCount);
        
        if (mpuReady) {
            Serial.printf("   📊 Accel: %.2f, %.2f, %.2f m/s²\n", 
                         sensorData.accelX, sensorData.accelY, sensorData.accelZ);
        }
        
        if (wifiConnected) {
            Serial.println("   🌐 WiFi: Connected");
        }
    }
    
    delay(50);
}

void updateGPS() {
  if (useDummyGPS) {
    float baseLat = -6.200000;
    float baseLon = 106.816666;
    float noiseLat = ((float)random(-500, 500)) / 100000.0;
    float noiseLon = ((float)random(-500, 500)) / 100000.0;
    float lat = baseLat + noiseLat;
    float lon = baseLon + noiseLon;
    sensorData.lastLat=lat;
    sensorData.lastLon=lon;
    char nmea[100];
    int latDeg = abs((int)lat);
    float latMin = (abs(lat) - latDeg) * 60.0;
    char ns = lat < 0 ? 'S' : 'N';
    int lonDeg = abs((int)lon);
    float lonMin = (abs(lon) - lonDeg) * 60.0;
    char ew = lon < 0 ? 'W' : 'E';

    // snprintf(nmea, sizeof(nmea),
    //          "$GPRMC,123519,A,%02d%07.4f,%c,%03d%07.4f,%c,000.5,054.7,191194,020.3,E*68",
    //          latDeg, latMin, ns, lonDeg, lonMin, ew);

    for (int i = 0; i < strlen(nmea); i++) {
      gps.encode(nmea[i]);
    }

    Serial.print("GPS");
    Serial.println(nmea);
  }
}
// void printLastLocation() {
//   if (lastLat != 0.0 && lastLon != 0.0) {
//     Serial.print("Last Known Location -> ");
//     Serial.print("Lat: "); Serial.print(lastLat, 6);
//     Serial.print(" | Lon: "); Serial.println(lastLon, 6);
//   }
// }
