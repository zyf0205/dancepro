#ifndef NOTE_UI_H
#define NOTE_UI_H

#include <M5Unified.h>
#include <M5GFX.h>
#include "imu/imu.h"  // 包含IMU数据结构

// 全局判断变量，用于控制清屏和重绘
extern bool noteUIRedrawNeeded;

// 显示音乐录制界面
// isRecording: 是否正在录制
// recordTime: 录制时间(毫秒)，仅在录制状态下使用
// imuData: IMU传感器数据，用于律动效果
void displayNoteUI(bool isRecording = false, unsigned long recordTime = 0, IMUData imuData = {0});

#endif
