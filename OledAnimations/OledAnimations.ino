#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// 내장 OLED I2C 핀 설정 (AGENTS.md 규칙 준수)
#define OLED_SDA 14 // D5 (GPIO 14)
#define OLED_SCL 12 // D6 (GPIO 12)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 애니메이션 관리를 위한 변수들
unsigned long lastTime = 0;
int currentMode = 0;
const int modeDuration = 8000; // 각 애니메이션 지속 시간 (8초)

// 1. Starfield (우주 공간 워프) 변수
#define NUM_STARS 35
struct Star {
  float x, y, z;
};
Star stars[NUM_STARS];
const float starSpeed = 2.0;

// 2. 3D Rotating Cube 변수
struct Point3D {
  float x, y, z;
};
Point3D cubeVertices[8] = {
  {-20, -20, -20}, {20, -20, -20}, {20, 20, -20}, {-20, 20, -20},
  {-20, -20, 20},  {20, -20, 20},  {20, 20, 20},  {-20, 20, 20}
};
int cubeEdges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0}, // 앞면
  {4, 5}, {5, 6}, {6, 7}, {7, 4}, // 뒷면
  {0, 4}, {1, 5}, {2, 6}, {3, 7}  // 연결선
};
float angleX = 0;
float angleY = 0;
float angleZ = 0;

// 3. Plexus (입자 연결 망) 변수
#define NUM_PARTICLES 16
struct Particle {
  float x, y;
  float dx, dy;
};
Particle particles[NUM_PARTICLES];
const float maxDistance = 25.0;

// 4. DNA Helix 변수
float dnaPhase = 0;

// 5. Morphing String Art 변수
struct Node {
  float x, y;
  float dx, dy;
};
Node nodes[4];

// 초기화 도우미 함수들
void initStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-64, 64);
    stars[i].y = random(-32, 32);
    stars[i].z = random(10, 128);
  }
}

void initParticles() {
  for (int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x = random(0, SCREEN_WIDTH);
    particles[i].y = random(0, SCREEN_HEIGHT);
    particles[i].dx = (random(5, 15) / 10.0) * (random(0, 2) == 0 ? 1 : -1);
    particles[i].dy = (random(5, 15) / 10.0) * (random(0, 2) == 0 ? 1 : -1);
  }
}

void initNodes() {
  for (int i = 0; i < 4; i++) {
    nodes[i].x = random(0, SCREEN_WIDTH);
    nodes[i].y = random(0, SCREEN_HEIGHT);
    nodes[i].dx = (random(10, 25) / 10.0) * (random(0, 2) == 0 ? 1 : -1);
    nodes[i].dy = (random(10, 25) / 10.0) * (random(0, 2) == 0 ? 1 : -1);
  }
}

void setup() {
  Serial.begin(115200);
  
  // 내장 OLED 전용 I2C 시작
  Wire.begin(OLED_SDA, OLED_SCL);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed"));
    for (;;);
  }
  
  display.clearDisplay();
  display.display();
  
  // 랜덤 시드 설정
  randomSeed(analogRead(0));
  
  // 각 애니메이션 데이터 초기화
  initStars();
  initParticles();
  initNodes();
  
  lastTime = millis();
}

// 1. Starfield (점 기반 우주 여행)
void drawStarfield() {
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].z -= starSpeed;
    if (stars[i].z <= 0) {
      stars[i].x = random(-64, 64);
      stars[i].y = random(-32, 32);
      stars[i].z = 128;
    }
    
    // 3D 좌표를 2D 화면 좌표로 투영
    int px = (stars[i].x * 64) / stars[i].z + centerX;
    int py = (stars[i].y * 64) / stars[i].z + centerY;
    
    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
      display.drawPixel(px, py, SSD1306_WHITE);
    }
  }
}

// 2. 3D Rotating Cube (선 기반 3차원 회전 큐브)
void drawRotatingCube() {
  Point3D rotated[8];
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  
  // 회전각 업데이트
  angleX += 0.03;
  angleY += 0.04;
  angleZ += 0.02;
  
  // 3D 회전 계산 및 2D 투영
  for (int i = 0; i < 8; i++) {
    float x = cubeVertices[i].x;
    float y = cubeVertices[i].y;
    float z = cubeVertices[i].z;
    
    // X축 회전
    float y1 = y * cos(angleX) - z * sin(angleX);
    float z1 = y * sin(angleX) + z * cos(angleX);
    
    // Y축 회전
    float x2 = x * cos(angleY) - z1 * sin(angleY);
    float z2 = x * sin(angleY) + z1 * cos(angleY);
    
    // Z축 회전
    float x3 = x2 * cos(angleZ) - y1 * sin(angleZ);
    float y3 = x2 * sin(angleZ) + y1 * cos(angleZ);
    
    // 2D 원근 투영 (Perspective projection)
    float distance = 60;
    float sz = 1.0 / (1.0 - z2 / distance);
    rotated[i].x = x3 * sz + centerX;
    rotated[i].y = y3 * sz + centerY;
  }
  
  // 엣지(선) 그리기
  for (int i = 0; i < 12; i++) {
    int startIdx = cubeEdges[i][0];
    int endIdx = cubeEdges[i][1];
    display.drawLine(rotated[startIdx].x, rotated[startIdx].y, 
                     rotated[endIdx].x, rotated[endIdx].y, SSD1306_WHITE);
  }
}

// 3. Plexus (점과 선의 연결 유기적 망)
void drawPlexus() {
  // 입자 이동 및 그리기
  for (int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x += particles[i].dx;
    particles[i].y += particles[i].dy;
    
    // 벽 충돌 감지
    if (particles[i].x <= 0 || particles[i].x >= SCREEN_WIDTH) particles[i].dx *= -1;
    if (particles[i].y <= 0 || particles[i].y >= SCREEN_HEIGHT) particles[i].dy *= -1;
    
    display.drawPixel(particles[i].x, particles[i].y, SSD1306_WHITE);
  }
  
  // 근접한 입자 간에 선 연결
  for (int i = 0; i < NUM_PARTICLES; i++) {
    for (int j = i + 1; j < NUM_PARTICLES; j++) {
      float dist = sqrt(pow(particles[i].x - particles[j].x, 2) + 
                        pow(particles[i].y - particles[j].y, 2));
      if (dist < maxDistance) {
        display.drawLine(particles[i].x, particles[i].y, 
                         particles[j].x, particles[j].y, SSD1306_WHITE);
      }
    }
  }
}

// 4. DNA Helix (이중 나선 회전 애니메이션)
void drawDNAHelix() {
  int centerY = SCREEN_HEIGHT / 2;
  dnaPhase += 0.1;
  
  for (int x = 4; x < SCREEN_WIDTH; x += 8) {
    float angle = x * 0.08 + dnaPhase;
    int y1 = centerY + sin(angle) * 20;
    int y2 = centerY - sin(angle) * 20;
    
    // 두 나선의 메인 포인트
    display.drawPixel(x, y1, SSD1306_WHITE);
    display.drawPixel(x, y2, SSD1306_WHITE);
    
    // 두 포인트를 잇는 수직 기둥 (선)
    display.drawLine(x, y1, x, y2, SSD1306_WHITE);
  }
}

// 5. Morphing String Art (네 모서리를 회전하는 스트링 아트 선형 기하학)
void drawStringArt() {
  // 노드들 이동
  for (int i = 0; i < 4; i++) {
    nodes[i].x += nodes[i].dx;
    nodes[i].y += nodes[i].dy;
    
    if (nodes[i].x <= 0 || nodes[i].x >= SCREEN_WIDTH) nodes[i].dx *= -1;
    if (nodes[i].y <= 0 || nodes[i].y >= SCREEN_HEIGHT) nodes[i].dy *= -1;
  }
  
  // 노드들을 연결하는 보간 선들을 여러 개 촘촘히 그려 기하학적 형태 형성
  int steps = 10;
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / steps;
    
    // Node 0 -> Node 1 보간점
    float xa = nodes[0].x + t * (nodes[1].x - nodes[0].x);
    float ya = nodes[0].y + t * (nodes[1].y - nodes[0].y);
    
    // Node 2 -> Node 3 보간점
    float xb = nodes[2].x + t * (nodes[3].x - nodes[2].x);
    float yb = nodes[2].y + t * (nodes[3].y - nodes[2].y);
    
    display.drawLine(xa, ya, xb, yb, SSD1306_WHITE);
  }
}

void loop() {
  display.clearDisplay();
  
  // 현재 모드 실행
  switch (currentMode) {
    case 0:
      drawStarfield();
      break;
    case 1:
      drawRotatingCube();
      break;
    case 2:
      drawPlexus();
      break;
    case 3:
      drawDNAHelix();
      break;
    case 4:
      drawStringArt();
      break;
  }
  
  display.display();
  
  // 일정 시간마다 애니메이션 전환
  if (millis() - lastTime > modeDuration) {
    currentMode = (currentMode + 1) % 5;
    lastTime = millis();
    
    // 모드 전환 시 데이터 초기화
    if (currentMode == 0) initStars();
    if (currentMode == 2) initParticles();
    if (currentMode == 4) initNodes();
  }
  
  delay(15); // 프레임 레이트 제한 (~60fps)
}
