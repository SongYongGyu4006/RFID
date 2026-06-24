#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// 내장 OLED 및 I2C 공유 핀 설정 (AGENTS.md 규칙)
#define OLED_SDA 14 // D5 (GPIO 14)
#define OLED_SCL 12 // D6 (GPIO 12)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 와이파이 설정 정보
const char* ssid     = "...";
const char* password = "01024235756";

// 대한민국 시간 설정 (UTC+9)
const long gmtOffset_sec = 9 * 3600;
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  
  // D5, D6 핀으로 I2C 시작
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
  
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // OLED 연결 애니메이션 (점 찍기)
    display.print(F("."));
    display.display();
    
    counter++;
    if (counter > 30) { // 약 15초간 접속 실패 시 재시도
      display.clearDisplay();
      display.setCursor(0, 10);
      display.println(F("Failed to connect."));
      display.println(F("Retrying..."));
      display.display();
      delay(2000);
      ESP.restart();
    }
  }
  
  Serial.println(F("\nWiFi Connected!"));
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(F("WiFi Connected!"));
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.println(F("Syncing Time..."));
  display.display();
  
  // NTP 서버로부터 대한민국 시간 동기화 설정 (ESP8266 내장 SDK 시간 동기화 사용)
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.windows.com");
  
  // 최초 시간 동기화 대기 (정상 동기화될 때까지 대기)
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { // 1970년도 초기값에서 벗어날 때까지
    delay(500);
    now = time(nullptr);
  }
  
  Serial.println(F("Time Synced!"));
}

void loop() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  // OLED 버퍼 지우기
  display.clearDisplay();
  
  // 1. 상단 WiFi 연결 표시 (헤더 바 형태)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("WiFi: OK"));
  
  // RSSI(신호 세기) 계산 및 우측 상단 표시
  long rssi = WiFi.RSSI();
  display.setCursor(90, 0);
  display.print(rssi);
  display.print(F("dBm"));
  
  // 헤더 가로선
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
  
  // 2. 중앙에 시간 표기 (HH:MM:SS) - 크게 표기 (크기 2)
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  display.setTextSize(2);
  // 가운데 정렬 계산 (가로 128픽셀 기준 폰트 크기 2의 가로 길이는 글자당 12픽셀, 총 8글자 = 96픽셀)
  display.setCursor((SCREEN_WIDTH - 96) / 2, 22); 
  display.print(timeStr);
  
  // 3. 하단에 날짜 표기 (YYYY-MM-DD / 요일)
  char dateStr[25];
  const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  sprintf(dateStr, "%04d-%02d-%02d  %s", 
          timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, days[timeinfo->tm_wday]);
  
  display.setTextSize(1);
  // 가운데 정렬 (글자당 6픽셀, 총 16글자 = 96픽셀)
  display.setCursor((SCREEN_WIDTH - 96) / 2, 48);
  display.print(dateStr);
  
  // 최종 버퍼 화면 출력
  display.display();
  
  delay(1000); // 1초 대기
}
