#ifndef HOME_UI_H
#define HOME_UI_H

#include <M5Unified.h>
#include <WiFi.h>
#include <time.h>

// 全局判断变量，用于控制清屏和重绘
extern bool homeUIRedrawNeeded;

// 显示主界面UI
void displayHomeUI();

// 初始化时间（非阻塞方式）
void initTimeAsync();

#endif
