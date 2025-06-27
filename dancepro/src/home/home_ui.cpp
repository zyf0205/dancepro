#include "home/home_ui.h"

// Color definitions
#define COLOR_BG            0x0000  // Black background
#define COLOR_TEXT          0xFFFF  // White text
#define COLOR_TITLE         0x07FF  // Cyan title
#define COLOR_ACCENT        0xF81F  // Magenta accent
#define COLOR_BATTERY_LOW   0xF800  // Red for low battery
#define COLOR_BATTERY_MID   0xFD20  // Orange for medium battery
#define COLOR_BATTERY_HIGH  0x07E0  // Green for high battery
#define COLOR_STATUS_BG     0x18E3  // Dark gray background for status bar
#define COLOR_NOTE1         0xF81F  // 品红
#define COLOR_NOTE2         0x07FF  // 青色
#define COLOR_NOTE3         0xFFE0  // 黄色
#define COLOR_NOTE4         0x07E0  // 绿色
#define COLOR_NOTE5         0x001F  // 蓝色

// 全局判断变量，用于控制清屏和重绘
bool homeUIRedrawNeeded = true;

// 静态变量用于动画和状态跟踪
static unsigned long lastAnimationTime = 0;
static unsigned long lastTimeUpdateTime = 0;
static int animationFrame = 0;
static bool timeInitialized = false;

// 用于保持时间的变量
static time_t lastSyncedTime = 0;      // 上次同步的UNIX时间戳
static unsigned long lastSyncedMillis = 0;  // 上次同步时的millis()值
static String cachedTimeStr = "00:00";  // 缓存的时间字符串

// 状态栏高度
#define STATUS_BAR_HEIGHT 20

// 动画区域参数
#define ANIM_MAX_RADIUS 50  // 动画最大半径
#define ANIM_CENTER_Y ((M5.Display.height() + STATUS_BAR_HEIGHT) / 2 + 10)  // 动画中心Y坐标，下移一点

// NTP服务器设置
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;  // 东八区 (UTC+8)
const int   daylightOffset_sec = 0;

// 前向声明辅助函数
void drawSimpleMusicNote(int centerX, int centerY, int frame);
void drawCircleToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int x0, int y0, int radius, uint16_t color);
void drawRectToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int x, int y, int w, int h, uint16_t color);
void drawLineToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int x0, int y0, int x1, int y1, uint16_t color);
void drawMusicNoteToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int centerX, int centerY, float scale, uint16_t color);
void drawMusicNoteAnimation(int frame);

// 非阻塞方式初始化时间 - 完全不阻塞UI线程
void initTimeAsync() {
  static bool ntpConfigured = false;
  
  // 只配置一次NTP
  if (!ntpConfigured && WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    ntpConfigured = true;
  }
  
  // 尝试获取时间但不阻塞
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1)) { // 最多等待1ms
      // 保存当前的UNIX时间戳和millis()值
      time(&lastSyncedTime);
      lastSyncedMillis = millis();
      timeInitialized = true;
    }
  }
}

// 获取格式化的时间字符串 - 完全不阻塞
String getFormattedTime() {
  static unsigned long lastTimeCheck = 0;
  unsigned long currentMillis = millis();
  
  // 每500ms更新一次时间字符串
  if (currentMillis - lastTimeCheck > 500) {
    lastTimeCheck = currentMillis;
    
    // 如果之前同步过时间
    if (lastSyncedTime > 0) {
      // 计算从上次同步后经过的秒数
      unsigned long elapsedSeconds = (currentMillis - lastSyncedMillis) / 1000;
      
      // 计算当前的UNIX时间戳
      time_t currentTime = lastSyncedTime + elapsedSeconds;
      
      // 转换为本地时间
      struct tm timeinfo;
      localtime_r(&currentTime, &timeinfo);
      
      // 格式化时间字符串
      char timeString[6]; // HH:MM + null terminator
      sprintf(timeString, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      cachedTimeStr = String(timeString);
    } else if (!timeInitialized) {
      // 如果从未同步过时间，显示设备运行时间
      unsigned long uptime = currentMillis / 1000;
      int hours = (uptime / 3600) % 24;
      int minutes = (uptime / 60) % 60;
      char uptimeStr[6];
      sprintf(uptimeStr, "%02d:%02d", hours, minutes);
      cachedTimeStr = String(uptimeStr);
    }
  }
  
  return cachedTimeStr;
}

// 绘制状态栏（时间和电池）
void drawStatusBar() {
  // 绘制状态栏背景
  M5.Display.fillRect(0, 0, M5.Display.width(), STATUS_BAR_HEIGHT, COLOR_STATUS_BG);
  
  // 绘制时间
  M5.Display.setTextColor(COLOR_TEXT);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(5, 6);
  M5.Display.print(getFormattedTime());
  
  // 绘制电池状态
  float batteryLevel = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();
  
  // 电池外框
  int battX = M5.Display.width() - 30;
  int battY = 5;
  int battWidth = 20;
  int battHeight = 10;
  
  // 绘制电池外框
  M5.Display.drawRect(battX, battY, battWidth, battHeight, COLOR_TEXT);
  M5.Display.drawRect(battX + battWidth, battY + 2, 2, battHeight - 4, COLOR_TEXT);
  
  // 确定电池颜色
  uint16_t batteryColor;
  if (batteryLevel < 20) {
    batteryColor = COLOR_BATTERY_LOW;
  } else if (batteryLevel < 50) {
    batteryColor = COLOR_BATTERY_MID;
  } else {
    batteryColor = COLOR_BATTERY_HIGH;
  }
  
  // 绘制电池填充
  int fillWidth = (battWidth - 2) * batteryLevel / 100.0;
  M5.Display.fillRect(battX + 1, battY + 1, fillWidth, battHeight - 2, batteryColor);
  
  // 如果正在充电，绘制闪电图标
  if (isCharging) {
    M5.Display.fillTriangle(
      battX + battWidth/2 - 2, battY + 2,
      battX + battWidth/2 + 2, battY + battHeight/2,
      battX + battWidth/2 - 1, battY + battHeight/2,
      COLOR_TEXT
    );
    M5.Display.fillTriangle(
      battX + battWidth/2 + 2, battY + battHeight - 2,
      battX + battWidth/2 - 2, battY + battHeight/2,
      battX + battWidth/2 + 1, battY + battHeight/2,
      COLOR_TEXT
    );
  }
  
  // 显示电池百分比
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT);
  M5.Display.setCursor(battX - 30, battY + 2);
  M5.Display.printf("%d%%", (int)batteryLevel);
}

// 绘制音符律动动画 - 优化版本，防止屏闪
void drawMusicNoteAnimation(int frame) {
  int centerX = M5.Display.width() / 2;
  int centerY = ANIM_CENTER_Y;
  
  // 创建双缓冲区，减少屏闪
  static uint16_t* animBuffer = NULL;
  static int lastBufferWidth = 0;
  static int lastBufferHeight = 0;
  
  // 计算动画区域大小
  int animWidth = M5.Display.width();
  int animHeight = M5.Display.height() - STATUS_BAR_HEIGHT;
  int animStartY = STATUS_BAR_HEIGHT;
  
  // 如果缓冲区未初始化或大小改变，重新分配
  if (animBuffer == NULL || lastBufferWidth != animWidth || lastBufferHeight != animHeight) {
    if (animBuffer != NULL) {
      free(animBuffer);
    }
    animBuffer = (uint16_t*)malloc(animWidth * animHeight * sizeof(uint16_t));
    lastBufferWidth = animWidth;
    lastBufferHeight = animHeight;
    
    // 如果内存分配失败，直接在屏幕上绘制
    if (animBuffer == NULL) {
      // 清除整个动画区域
      M5.Display.fillRect(0, STATUS_BAR_HEIGHT, M5.Display.width(), 
                         M5.Display.height() - STATUS_BAR_HEIGHT, COLOR_BG);
      
      // 简化版动画，减少复杂度
      drawSimpleMusicNote(centerX, centerY, frame);
      return;
    }
  }
  
  // 填充缓冲区背景色
  for (int i = 0; i < animWidth * animHeight; i++) {
    animBuffer[i] = COLOR_BG;
  }
  
  // 动态音符颜色
  uint16_t noteColor;
  switch ((frame / 30) % 6) {
    case 0: noteColor = COLOR_NOTE1; break;
    case 1: noteColor = COLOR_NOTE2; break;
    case 2: noteColor = COLOR_NOTE3; break;
    case 3: noteColor = COLOR_NOTE4; break;
    case 4: noteColor = COLOR_NOTE5; break;
    case 5: noteColor = COLOR_BATTERY_LOW; break;
    default: noteColor = COLOR_TEXT; break;
  }
  
  // 音符大小随动画帧变化 - 使用正弦函数实现平滑律动
  float scale = 1.0 + 0.15 * sin(frame * 0.05);
  
  // 绘制中央音符
  drawMusicNoteToBuffer(animBuffer, animWidth, animHeight, centerX, centerY - animStartY, scale, noteColor);
  
  // 绘制音波
  for (int i = 0; i < 3; i++) {
    int waveRadius = (20 + i * 15) * scale;
    float waveOffset = frame * 0.05 + i * 1.0;
    
    for (int angle = 0; angle < 360; angle += 30) {
      float radian = (angle + waveOffset * 10) * PI / 180.0;
      int x1 = centerX + waveRadius * cos(radian);
      int y1 = (centerY + waveRadius * sin(radian)) - animStartY;
      int x2 = centerX + (waveRadius + 5) * cos(radian);
      int y2 = (centerY + (waveRadius + 5) * sin(radian)) - animStartY;
      
      // 确保点在缓冲区内
      if (x1 >= 0 && x1 < animWidth && y1 >= 0 && y1 < animHeight &&
          x2 >= 0 && x2 < animWidth && y2 >= 0 && y2 < animHeight) {
        drawLineToBuffer(animBuffer, animWidth, animHeight, x1, y1, x2, y2, noteColor);
      }
    }
  }
  
  // 绘制小音符装饰
  int decorSize = 5 * scale;
  float decorAngle = frame * 0.03;
  
  for (int i = 0; i < 5; i++) {
    float angle = decorAngle + i * (2 * PI / 5);
    int radius = 35 * scale;
    int x = centerX + radius * cos(angle);
    int y = (centerY + radius * sin(angle)) - animStartY;
    
    // 确保在缓冲区范围内
    if (x >= decorSize && x < animWidth - decorSize && 
        y >= decorSize && y < animHeight - decorSize) {
      
      // 交替绘制不同形状的装饰
      if (i % 2 == 0) {
        // 小音符
        drawCircleToBuffer(animBuffer, animWidth, animHeight, x, y, decorSize, noteColor);
        
        // 小音符杆
        int miniStemLength = 8 * scale;
        if (y - miniStemLength >= 0) {
          drawRectToBuffer(animBuffer, animWidth, animHeight, 
                        x + decorSize - 1, y - miniStemLength, 
                        1, miniStemLength, noteColor);
        }
      } else {
        // 星形装饰
        for (int j = 0; j < 8; j++) {
          float starAngle = j * PI / 4;
          int x1 = x + decorSize * cos(starAngle);
          int y1 = y + decorSize * sin(starAngle);
          drawLineToBuffer(animBuffer, animWidth, animHeight, x, y, x1, y1, noteColor);
        }
      }
    }
  }
  
  // 添加随机飘动的小点
  int numPoints = 10;
  for (int i = 0; i < numPoints; i++) {
    float pointAngle = (frame * 0.02 + i * 36) * PI / 180.0;
    float distance = 20 + 20 * sin(frame * 0.03 + i);
    int x = centerX + distance * cos(pointAngle);
    int y = (centerY + distance * sin(pointAngle)) - animStartY;
    
    if (x >= 0 && x < animWidth && y >= 0 && y < animHeight) {
      animBuffer[y * animWidth + x] = noteColor;
    }
  }
  
  // 将缓冲区内容一次性绘制到屏幕上
  M5.Display.pushImage(0, STATUS_BAR_HEIGHT, animWidth, animHeight, animBuffer);
}

// 绘制音符到缓冲区
void drawMusicNoteToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int centerX, int centerY, float scale, uint16_t color) {
  // 绘制音符头部
  int noteHeadSize = 12 * scale;
  drawCircleToBuffer(buffer, bufWidth, bufHeight, 
                    centerX, centerY, noteHeadSize, color);
  
  // 绘制音符杆
  int stemLength = 30 * scale;
  int stemWidth = 3 * scale;
  drawRectToBuffer(buffer, bufWidth, bufHeight,
                  centerX + noteHeadSize - stemWidth, centerY - stemLength, 
                  stemWidth, stemLength, color);
  
  // 绘制音符旗帜
  int flagWidth = 15 * scale;
  int flagHeight = 8 * scale;
  
  // 第一个旗帜
  drawRectToBuffer(buffer, bufWidth, bufHeight,
                  centerX + noteHeadSize - stemWidth, centerY - stemLength, 
                  flagWidth, flagHeight, color);
  
  // 第二个旗帜
  drawRectToBuffer(buffer, bufWidth, bufHeight,
                  centerX + noteHeadSize - stemWidth, centerY - stemLength + 10 * scale, 
                  flagWidth, flagHeight, color);
}

// 简化版音符绘制，用于内存不足时
void drawSimpleMusicNote(int centerX, int centerY, int frame) {
  // 音符颜色
  uint16_t noteColor;
  switch ((frame / 30) % 6) {
    case 0: noteColor = COLOR_NOTE1; break;
    case 1: noteColor = COLOR_NOTE2; break;
    case 2: noteColor = COLOR_NOTE3; break;
    case 3: noteColor = COLOR_NOTE4; break;
    case 4: noteColor = COLOR_NOTE5; break;
    case 5: noteColor = COLOR_BATTERY_LOW; break;
    default: noteColor = COLOR_TEXT; break;
  }
  
  // 音符大小
  float scale = 1.0 + 0.15 * sin(frame * 0.05);
  int noteSize = 15 * scale;
  
  // 简单的音符
  M5.Display.fillCircle(centerX, centerY, noteSize, noteColor);
  M5.Display.fillRect(centerX + noteSize - 2, centerY - 30 * scale, 3 * scale, 30 * scale, noteColor);
  M5.Display.fillRect(centerX + noteSize + 1, centerY - 30 * scale, 10 * scale, 8 * scale, noteColor);
  
  // 简单的音波
  int waveRadius = 30 * scale;
  for (int angle = 0; angle < 360; angle += 45) {
    float radian = (angle + frame * 0.5) * PI / 180.0;
    int x1 = centerX + waveRadius * cos(radian);
    int y1 = centerY + waveRadius * sin(radian);
    int x2 = centerX + (waveRadius + 10 * scale) * cos(radian);
    int y2 = centerY + (waveRadius + 10 * scale) * sin(radian);
    M5.Display.drawLine(x1, y1, x2, y2, noteColor);
  }
}

// 辅助函数：在缓冲区中绘制圆形
void drawCircleToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int x0, int y0, int radius, uint16_t color) {
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x*x + y*y <= radius*radius) {
        int drawX = x0 + x;
        int drawY = y0 + y;
        if (drawX >= 0 && drawX < bufWidth && drawY >= 0 && drawY < bufHeight) {
          buffer[drawY * bufWidth + drawX] = color;
        }
      }
    }
  }
}

// 辅助函数：在缓冲区中绘制矩形
void drawRectToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int x, int y, int w, int h, uint16_t color) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      int drawX = x + i;
      int drawY = y + j;
      if (drawX >= 0 && drawX < bufWidth && drawY >= 0 && drawY < bufHeight) {
        buffer[drawY * bufWidth + drawX] = color;
      }
    }
  }
}

// 辅助函数：在缓冲区中绘制线段 (简化版Bresenham算法)
void drawLineToBuffer(uint16_t* buffer, int bufWidth, int bufHeight, int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  while (true) {
    if (x0 >= 0 && x0 < bufWidth && y0 >= 0 && y0 < bufHeight) {
      buffer[y0 * bufWidth + x0] = color;
    }
    
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}

// 显示主界面UI
void displayHomeUI() {
  unsigned long currentTime = millis();
  
  // 尝试异步初始化时间 (完全非阻塞)
  initTimeAsync();
  
  // 检测是否需要重绘
  bool updateTime = (currentTime - lastTimeUpdateTime >= 1000); // 每秒更新时间
  bool updateAnimation = (currentTime - lastAnimationTime >= 100); // 每100ms更新动画
  
  if (homeUIRedrawNeeded) {
    // 清屏
    M5.Display.fillScreen(COLOR_BG);
    
    // 重置重绘标志
    homeUIRedrawNeeded = false;
    
    // 强制立即更新时间和动画
    updateTime = true;
    updateAnimation = true;
  }
  
  // 更新时间和状态栏
  if (updateTime) {
    drawStatusBar();
    lastTimeUpdateTime = currentTime;
  }
  
  // 更新动画 - 使用优化的绘制方法
  if (updateAnimation) {
    animationFrame = (animationFrame + 1) % 120;  // 使用更长的循环周期
    drawMusicNoteAnimation(animationFrame);
    lastAnimationTime = currentTime;
  }
}
