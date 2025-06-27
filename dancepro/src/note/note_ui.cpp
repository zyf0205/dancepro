#include "note/note_ui.h"
#include "imu/imu.h"

// 简化的配色方案 - 仅使用三种主要颜色
#define COLOR_BG            0x0000  // 黑色背景
#define COLOR_TEXT          0xFFFF  // 白色文本
#define COLOR_PRIMARY       0x07FF  // 青色作为主题色
#define COLOR_ACCENT        0xF800  // 红色作为强调色（仅用于录制指示器）

// Static variables to prevent screen flicker
static bool prevIsRecording = false;
static unsigned long lastAnimationTime = 0;
static unsigned long lastBlinkTime = 0;  // 单独控制红点闪烁
static int animationFrame = 0;
static bool blinkState = false;  // 红点闪烁状态

// 状态栏高度
#define STATUS_BAR_HEIGHT 20

// 全局判断变量，用于控制清屏和重绘
bool noteUIRedrawNeeded = true;

// Display recording start UI
void displayNoteUI(bool isRecording, unsigned long recordTime, IMUData imuData) {
  unsigned long currentTime = millis();
  
  // 检测状态变化，需要完全重绘
  if (isRecording != prevIsRecording) {
    noteUIRedrawNeeded = true;
    prevIsRecording = isRecording;
  }
  
  // 开始界面
  if (!isRecording) {
    // 只在需要重绘时绘制静态元素
    if (noteUIRedrawNeeded) {
      // 清屏
      M5.Display.fillScreen(COLOR_BG);
      
      // 绘制标题栏
      M5.Display.fillRect(0, 0, M5.Display.width(), STATUS_BAR_HEIGHT, COLOR_BG);
      M5.Display.setTextColor(COLOR_PRIMARY);
      M5.Display.setTextSize(1);
      M5.Display.setCursor(5, 6);
      M5.Display.print("RECORD");
      
      // 绘制音符图标
      int noteX = M5.Display.width() - 15;
      int noteY = 10;
      M5.Display.fillCircle(noteX, noteY, 5, COLOR_PRIMARY);
      M5.Display.fillRect(noteX, noteY - 5, 2, 10, COLOR_PRIMARY);
      
      // // 绘制中央标题
      // M5.Display.setTextColor(COLOR_TEXT);
      // M5.Display.setTextSize(1);
      // M5.Display.setCursor((M5.Display.width() - 96) / 2, 35);
      // M5.Display.print(" MUSIC");
      
      // 绘制按钮
      int btnWidth = 90;
      int btnHeight = 30;
      int btnX = (M5.Display.width() - btnWidth) / 2;
      int btnY = 70;
      M5.Display.fillRoundRect(btnX, btnY, btnWidth, btnHeight, 8, COLOR_PRIMARY);
      
      // 按钮文本
      M5.Display.setTextColor(COLOR_BG);
      M5.Display.setCursor(btnX + 25, btnY + 10);
      M5.Display.print("START");
      
      // 重置重绘标志
      noteUIRedrawNeeded = false;
      lastAnimationTime = currentTime - 200; // 确保动画立即更新
    }
    
    // 动画元素（每150毫秒更新一次）
    if (currentTime - lastAnimationTime > 150) {
      // 更新动画帧
      animationFrame = (animationFrame + 1) % 12;
      
      // 按钮脉冲效果
      int btnWidth = 90;
      int btnHeight = 30;
      int btnX = (M5.Display.width() - btnWidth) / 2;
      int btnY = 70;
      
      // 清除按钮边框区域
      M5.Display.drawRoundRect(btnX - 3, btnY - 3, btnWidth + 6, btnHeight + 6, 10, COLOR_BG);
      M5.Display.drawRoundRect(btnX - 2, btnY - 2, btnWidth + 4, btnHeight + 4, 9, COLOR_BG);
      
      // 绘制新的脉冲（使用主题色的不同亮度）
      uint16_t pulseColor = (animationFrame % 2 == 0) ? COLOR_PRIMARY : COLOR_TEXT;
      M5.Display.drawRoundRect(btnX - 2, btnY - 2, btnWidth + 4, btnHeight + 4, 10, pulseColor);
      
      // 清除波形区域
      M5.Display.fillRect(0, 50, M5.Display.width(), 10, COLOR_BG);
      
      // 绘制动画波形
      for (int i = 0; i < M5.Display.width(); i += 5) {
        float offset = animationFrame * 0.5;
        int waveHeight = 5 + 5 * sin(i * 0.1 + offset);
        M5.Display.drawLine(i, 50, i, 50 + waveHeight, COLOR_PRIMARY);
      }
      
      lastAnimationTime = currentTime;
    }
  }
  // 录制中界面
  else {
    // 绘制静态元素
    if (noteUIRedrawNeeded) {
      // 清屏
      M5.Display.fillScreen(COLOR_BG);
      
      // 绘制顶部状态栏背景
      M5.Display.fillRect(0, 0, M5.Display.width(), STATUS_BAR_HEIGHT, COLOR_BG);
      
      // 绘制录制文本
      M5.Display.setTextColor(COLOR_PRIMARY);
      M5.Display.setTextSize(1);
      M5.Display.setCursor(5, 6);
      M5.Display.print("REC");
      
      // 绘制停止按钮
      int btnWidth = 90;
      int btnHeight = 30;
      int btnX = (M5.Display.width() - btnWidth) / 2;
      int btnY = 95;
      M5.Display.fillRoundRect(btnX, btnY, btnWidth, btnHeight, 8, COLOR_ACCENT);
      
      // 按钮文本
      M5.Display.setTextColor(COLOR_TEXT);
      M5.Display.setTextSize(1);
      M5.Display.setCursor(btnX + 30, btnY + 10);
      M5.Display.print("STOP");
      
      // 初始化闪烁状态
      blinkState = true;
      
      // 重置重绘标志
      noteUIRedrawNeeded = false;
      lastAnimationTime = currentTime - 100; // 确保动画立即更新
      lastBlinkTime = currentTime;
    }
    
    // 单独控制红点闪烁（降低频率到800毫秒）
    if (currentTime - lastBlinkTime > 800) {
      blinkState = !blinkState;
      
      // 清除红点区域
      M5.Display.fillCircle(30, 10, 5, COLOR_BG);
      
      // 绘制录制指示点
      if (blinkState) {
        M5.Display.fillCircle(30, 10, 5, COLOR_ACCENT);
      }
      
      lastBlinkTime = currentTime;
    }
    
    // 更新录制时间（每秒更新一次）
    if (currentTime - lastAnimationTime > 100) {
      // 更新录制时间显示
      unsigned long seconds = recordTime / 1000;
      unsigned long minutes = seconds / 60;
      seconds = seconds % 60;
      
      // 清除时间区域
      M5.Display.fillRect(M5.Display.width() - 45, 0, 45, STATUS_BAR_HEIGHT, COLOR_BG);
      
      // 绘制更新后的时间
      M5.Display.setTextColor(COLOR_TEXT);
      M5.Display.setTextSize(1);
      M5.Display.setCursor(M5.Display.width() - 45, 6);
      if (minutes < 10) M5.Display.print("0");
      M5.Display.print(minutes);
      M5.Display.print(":");
      if (seconds < 10) M5.Display.print("0");
      M5.Display.print(seconds);
      
      // 更新动画帧
      animationFrame = (animationFrame + 1) % 12;
      
      // 清除律动区域 - 下移到STOP按钮上方
      int rhythmAreaTop = STATUS_BAR_HEIGHT + 5;
      int rhythmAreaBottom = 90; // STOP按钮上方留一些空间
      M5.Display.fillRect(0, rhythmAreaTop, M5.Display.width(), rhythmAreaBottom - rhythmAreaTop, COLOR_BG);
      
      // 使用IMU数据绘制律动条
      int barCount = 12;
      int barWidth = 8;
      int barGap = 2;
      int totalWidth = barCount * (barWidth + barGap) - barGap;
      int startX = (M5.Display.width() - totalWidth) / 2;
      int baseY = 70; // 调整基准Y坐标，使律动条在STOP按钮上方
      
      // 将IMU数据映射到律动条高度
      float accMagnitude = sqrt(imuData.accX*imuData.accX + 
                               imuData.accY*imuData.accY + 
                               imuData.accZ*imuData.accZ);
      
      float gyroMagnitude = sqrt(imuData.gyroX*imuData.gyroX + 
                                imuData.gyroY*imuData.gyroY + 
                                imuData.gyroZ*imuData.gyroZ);
      
      // 绘制律动条
      for (int i = 0; i < barCount; i++) {
        // 使用不同的数据源计算高度
        float heightFactor = 0;
        
        // 使用加速度、陀螺仪和姿态角的组合
        if (i % 3 == 0) {
          // 基于加速度
          heightFactor = map(accMagnitude, 0.5, 3.0, 5, 35);
        } else if (i % 3 == 1) {
          // 基于陀螺仪
          heightFactor = map(gyroMagnitude, 0, 200, 5, 35);
        } else {
          // 基于姿态角
          heightFactor = map(abs(imuData.pitch) + abs(imuData.roll), 0, 90, 5, 35);
        }
        
        // 添加一些随机变化和波形效果
        float phase = (i * 30 + animationFrame * 10) * PI / 180.0;
        int barHeight = heightFactor + 5 * sin(phase);
        
        // 确保高度在合理范围内
        barHeight = constrain(barHeight, 5, 40);
        
        // 绘制律动条
        int barX = startX + i * (barWidth + barGap);
        M5.Display.fillRect(barX, baseY - barHeight, barWidth, barHeight, COLOR_PRIMARY);
      }
      
      lastAnimationTime = currentTime;
    }
  }
}

