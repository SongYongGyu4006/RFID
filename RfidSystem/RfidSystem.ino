#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// 내장 OLED 및 I2C 공유 핀 설정 (AGENTS.md 규칙)
#define OLED_SDA 14 // D5 (GPIO 14)
#define OLED_SCL 12 // D6 (GPIO 12)

// PN532 I2C 더미 핀 설정 (물리적으로 연결하지 않아도 됨)
#define PN532_IRQ   5  // D1 (GPIO 5) - 더미
#define PN532_RESET 4  // D2 (GPIO 4) - 더미

// 부저 핀 설정 (D7 - GPIO 13)
#define BUZZER_PIN  13 // D7 (GPIO 13)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// 와이파이 설정 정보
const char* ssid     = "...";
const char* password = "01024235756";

// API 서버 정보
const char* serverUrl = "http://43.200.11.185/api_scan.php";

// 대한민국 시간 설정 (UTC+9)
const long gmtOffset_sec = 9 * 3600;
const int daylightOffset_sec = 0;

// 부저 알림음 정의
void playBootSound() {
  tone(BUZZER_PIN, 523, 150); // C5
  delay(180);
  tone(BUZZER_PIN, 659, 150); // E5
  delay(180);
  tone(BUZZER_PIN, 784, 250); // G5
  delay(300);
}

void playScanSound() {
  tone(BUZZER_PIN, 2093, 80); // C7 (단일 짧은 비프 - 카드 인식 확인용)
  delay(100);
}

void playAccessGrantedSound() {
  tone(BUZZER_PIN, 1047, 100); // C6
  delay(120);
  tone(BUZZER_PIN, 1318, 100); // E6
  delay(120);
  tone(BUZZER_PIN, 1568, 200); // G6
  delay(200);
}

void playAccessDeniedSound() {
  tone(BUZZER_PIN, 150, 400); // 낮은 경고음
  delay(450);
}

void setup() {
  Serial.begin(115200);
  
  // 부저 핀 설정 및 부팅음
  pinMode(BUZZER_PIN, OUTPUT);
  playBootSound();
  
  // D5, D6 핀으로 I2C 버스 시작
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // OLED 디스플레이 초기화
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 OLED 초기화 실패"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("Connecting to WiFi..."));
  display.print(F("SSID: "));
  display.println(ssid);
  display.display();
  
  // 와이파이 연결 시작
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(F("."));
    display.display();
  }
  
  Serial.println(F("\nWiFi Connected!"));
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(F("WiFi Connected!"));
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.println(F("Initializing PN532..."));
  display.display();
  
  // PN532 RFID 리더기 초기화
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println(F("PN532 보드를 찾을 수 없습니다."));
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(F("Error: PN532 not found"));
    display.display();
    for(;;);
  }
  
  // RFID 카드를 읽기 위한 SAM 환경 설정
  nfc.SAMConfig();
  
  // NTP 시간 설정
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.windows.com");
  
  // 최초 시간 동기화 대기
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    now = time(nullptr);
  }
  
  Serial.println(F("System Ready!"));
  
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(F("System Ready"));
  display.println(F("Scan card/tag..."));
  display.display();
}

// 헬퍼: UID 바이트 배열을 16진수 문자열로 변환
String getUidString(uint8_t *uid, uint8_t uidLength) {
  String uidStr = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
    if (i < uidLength - 1) uidStr += " ";
  }
  uidStr.toUpperCase();
  return uidStr;
}

// 헬퍼: JSON 응답에서 문자열 추출 (경량 파서)
String extractFromJson(String json, String key) {
  String target = "\"" + key + "\":";
  int index = json.indexOf(target);
  if (index == -1) return "";
  
  int start = index + target.length();
  // 값 타입 체크 (문자열인지 불리언/숫자인지)
  if (json.charAt(start) == '"') {
    start++;
    int end = json.indexOf("\"", start);
    if (end != -1) return json.substring(start, end);
  } else {
    // 불리언/숫자형
    int end = json.indexOf(",", start);
    if (end == -1) end = json.indexOf("}", start);
    if (end != -1) return json.substring(start, end);
  }
  return "";
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  // 1. 카드 태깅 감지 (대기 시간 250ms로 제한하여 시계 업데이트 주기 확보)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 250);
  
  if (success) {
    // 카드 감지 성공음 재생
    playScanSound();
    
    String uidStr = getUidString(uid, uidLength);
    Serial.print(F("Card Scanned: "));
    Serial.println(uidStr);
    
    // OLED 화면 표시
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 5);
    display.println(F("SCANNING CARD..."));
    display.drawFastHLine(0, 15, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(0, 25);
    display.println(F("UID:"));
    display.setTextSize(2);
    display.println(uidStr);
    display.display();
    
    // 2. AWS 서버로 출입 정보 전송 (HTTP POST)
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;
      
      http.begin(client, serverUrl);
      http.addHeader("Content-Type", "application/json");
      
      String jsonPayload = "{\"uid\":\"" + uidStr + "\"}";
      int httpResponseCode = http.POST(jsonPayload);
      
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 5);
      
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Response: " + response);
        
        // JSON 응답 파싱
        String accessVal = extractFromJson(response, "access");
        String name = extractFromJson(response, "name");
        String role = extractFromJson(response, "role");
        
        bool isAllowed = (accessVal == "true" || accessVal == "1");
        
        if (isAllowed) {
          // 승인됨
          playAccessGrantedSound();
          
          display.println(F("ACCESS GRANTED"));
          display.drawFastHLine(0, 15, SCREEN_WIDTH, SSD1306_WHITE);
          display.setCursor(0, 25);
          display.setTextSize(2);
          display.println(name);
          display.setTextSize(1);
          display.setCursor(0, 48);
          display.println(role);
        } else {
          // 거부됨
          playAccessDeniedSound();
          
          display.println(F("ACCESS DENIED"));
          display.drawFastHLine(0, 15, SCREEN_WIDTH, SSD1306_WHITE);
          display.setCursor(0, 25);
          display.setTextSize(2);
          display.println(F("REJECTED"));
          display.setTextSize(1);
          display.setCursor(0, 48);
          display.println(F("Unregistered Card"));
        }
      } else {
        // 서버 연결 오류
        playAccessDeniedSound();
        display.println(F("CONNECTION ERROR"));
        display.drawFastHLine(0, 15, SCREEN_WIDTH, SSD1306_WHITE);
        display.setCursor(0, 25);
        display.println(F("Server Unreachable"));
        Serial.println("Error on HTTP request");
      }
      
      display.display();
      http.end();
    } else {
      // WiFi 연결이 끊겼을 때
      playAccessDeniedSound();
      display.clearDisplay();
      display.setCursor(0, 10);
      display.println(F("WiFi Offline"));
      display.println(F("Cannot verify card"));
      display.display();
    }
    
    // 화면 결과 표시 유지용 대기
    delay(2000);
  }
  
  // 3. 평상시 상태: OLED 시계 화면 표시
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  display.clearDisplay();
  
  // 상단 WiFi 및 수신강도 정보 바
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("WiFi: "));
  display.print(ssid);
  
  long rssi = WiFi.RSSI();
  display.setCursor(95, 0);
  display.print(rssi);
  display.print(F("dB"));
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
  
  // 중앙 시간 표시 (HH:MM:SS)
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  display.setTextSize(2);
  display.setCursor((SCREEN_WIDTH - 96) / 2, 20); 
  display.print(timeStr);
  
  // 하단 날짜 표시 (YYYY-MM-DD)
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
  display.setTextSize(1);
  display.setCursor((SCREEN_WIDTH - 60) / 2, 48);
  display.print(dateStr);
  
  // 하단 가이드 텍스트
  display.setCursor(0, 56);
  display.print(F(">> Ready to scan card"));
  
  display.display();
}
