#ifndef NOTE_H
#define NOTE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "imu/imu.h" // 引入已定义的IMU数据结构

// 低八度音符 (3) - 仅保留7个自然音
#define NOTE_C3  131  // 低音do
#define NOTE_D3  147  // 低音re
#define NOTE_E3  165  // 低音mi
#define NOTE_F3  175  // 低音fa
#define NOTE_G3  196  // 低音sol
#define NOTE_A3  220  // 低音la
#define NOTE_B3  247  // 低音si

// 中八度音符 (4) - 仅保留7个自然音
#define NOTE_C4  262  // 中央C (中音do)
#define NOTE_D4  294  // 中音re
#define NOTE_E4  330  // 中音mi
#define NOTE_F4  349  // 中音fa
#define NOTE_G4  392  // 中音sol
#define NOTE_A4  440  // 标准音高A (中音la)
#define NOTE_B4  494  // 中音si

// 高八度音符 (5) - 仅保留7个自然音
#define NOTE_C5  523  // 高音do
#define NOTE_D5  587  // 高音re
#define NOTE_E5  659  // 高音mi
#define NOTE_F5  698  // 高音fa
#define NOTE_G5  784  // 高音sol
#define NOTE_A5  880  // 高音la
#define NOTE_B5  988  // 高音si

// 休止符
#define NOTE_REST 0  // 休止符 (无声)

// 基本音符时值 - 简化版
#define BEAT_UNIT 300  // 基本拍子单位 (300ms)
#define W      1200    // 全音符 (4拍)
#define H       600    // 二分音符 (2拍)
#define Q       300    // 四分音符 (1拍)
#define E       150    // 八分音符 (1/2拍)

// IMU数据映射到音符和持续时间，并返回JSON格式字符串
String mapIMUToMusicJSON(const IMUData& imu, const String& existingJson = "[]");

#endif



