#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// 내장 OLED 및 I2C 공유 핀 설정
#define OLED_SDA 14 // D5 (GPIO 14)
#define OLED_SCL 12 // D6 (GPIO 12)

// PN532 I2C 더미 핀 설정 (물리적으로 연결하지 않아도 됨)
#define PN532_IRQ   5  // D1 (GPIO 5) - 더미
#define PN532_RESET 4  // D2 (GPIO 4) - 더미

// 부저 핀 설정 (D7 - GPIO 13)
#define BUZZER_PIN  13 // D7 (GPIO 13)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// 부팅 시 재생되는 멜로디 (도-미-솔 상승음)
void playBootSound() {
  tone(BUZZER_PIN, 523, 150); // C5 (523Hz)
  delay(180);
  tone(BUZZER_PIN, 659, 150); // E5 (659Hz)
  delay(180);
  tone(BUZZER_PIN, 784, 250); // G5 (784Hz)
  delay(300);
}

// 카드 스캔 성공 시 재생되는 멜로디 (삐빅- 경쾌한 이중음)
void playScanSound() {
  tone(BUZZER_PIN, 2093, 80); // C7 (2093Hz)
  delay(100);
  tone(BUZZER_PIN, 2093, 80); // C7 (2093Hz)
  delay(100);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // 시리얼 모니터 대기
  
  Serial.println(F("\n--- PN532 RFID/NFC I2C Test ---"));
  
  // 부저 핀 설정 및 부팅음 출력
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
  display.println(F("Initializing..."));
  display.display();
  
  // PN532 RFID 리더기 초기화
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println(F("PN532 보드를 찾을 수 없습니다. 배선 및 I2C 스위치 설정을 확인하세요."));
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(F("Error: PN532 not found"));
    display.println(F("Check DIP Switches"));
    display.println(F("SEL0: ON / SEL1: OFF"));
    display.display();
    for(;;); // 대기
  }
  
  // 펌웨어 버전 정보 표시
  Serial.print(F("Found chip PN5")); 
  Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print(F("Firmware ver. ")); 
  Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print(F(".")); 
  Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // RFID 카드를 읽기 위한 SAM(Secure Access Module) 환경 설정
  nfc.SAMConfig();
  
  Serial.println(F("Waiting for an ISO14443A Card/Tag..."));
  
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(F("RFID Test Ready"));
  display.println(F("Scan card/tag..."));
  display.display();
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // UID를 저장할 버퍼
  uint8_t uidLength;                        // UID의 길이 (4 또는 7 바이트)
    
  // ISO14443A 타입의 카드 감지 시도 (타임아웃 500ms 부여하여 대기 상태 방지)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 500);
  
  if (success) {
    // 카드 인식 성공 시 부저 알림음 연주
    playScanSound();
    
    Serial.println(F("\nFound an ISO14443A card"));
    Serial.print(F("  UID Length: ")); Serial.print(uidLength, DEC); Serial.println(F(" bytes"));
    Serial.print(F("  UID Value: "));
    nfc.PrintHex(uid, uidLength);
    Serial.println();
    
    // OLED 디스플레이 출력
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("CARD DETECTED!"));
    display.println(F("---------------------"));
    display.print(F("Length: "));
    display.print(uidLength);
    display.println(F(" bytes"));
    display.println(F("UID:"));
    
    // UID를 16진수 문자열로 OLED에 출력
    display.setTextSize(2); // UID는 크게 표시
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) display.print(F("0"));
      display.print(uid[i], HEX);
      if (i < uidLength - 1) display.print(F(" "));
    }
    display.display();
    
    // 카드 인식 지연 및 화면 표시 유지용 대기 (성공음 딜레이 감안하여 1.8초)
    delay(1800);
    
    // 화면 원상 복귀
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.println(F("RFID Test Ready"));
    display.println(F("Scan card/tag..."));
    display.display();
  } else {
    // 카드 인식 없을 시 시리얼에 하트비트 형태의 도트 출력 (작동 중임 확인용)
    Serial.print(".");
  }
}
