#ifndef NOTE_H
#define NOTE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "imu/imu.h" // 引入已定义的IMU数据结构


// 低八度音符 (2)
#define NOTE_C2  65   // 低低音do
#define NOTE_CS2 69   // 低低音do升
#define NOTE_D2  73   // 低低音re
#define NOTE_DS2 78   // 低低音re升
#define NOTE_E2  82   // 低低音mi
#define NOTE_F2  87   // 低低音fa
#define NOTE_FS2 93   // 低低音fa升
#define NOTE_G2  98   // 低低音sol
#define NOTE_GS2 104  // 低低音sol升
#define NOTE_A2  110  // 低低音la
#define NOTE_AS2 117  // 低低音la升
#define NOTE_B2  123  // 低低音si

// 低八度音符 (3)
#define NOTE_C3  131  // 低音do
#define NOTE_CS3 139  // 低音do升
#define NOTE_D3  147  // 低音re
#define NOTE_DS3 156  // 低音re升
#define NOTE_E3  165  // 低音mi
#define NOTE_F3  175  // 低音fa
#define NOTE_FS3 185  // 低音fa升
#define NOTE_G3  196  // 低音sol
#define NOTE_GS3 208  // 低音sol升
#define NOTE_A3  220  // 低音la
#define NOTE_AS3 233  // 低音la升
#define NOTE_B3  247  // 低音si

// 中八度音符 (4)
#define NOTE_C4  262  // 中央C (中音do)
#define NOTE_CS4 277  // 中音do升
#define NOTE_D4  294  // 中音re
#define NOTE_DS4 311  // 中音re升
#define NOTE_E4  330  // 中音mi
#define NOTE_F4  349  // 中音fa
#define NOTE_FS4 370  // 中音fa升
#define NOTE_G4  392  // 中音sol
#define NOTE_GS4 415  // 中音sol升
#define NOTE_A4  440  // 标准音高A (中音la)
#define NOTE_AS4 466  // 中音la升
#define NOTE_B4  494  // 中音si

// 高八度音符 (5)
#define NOTE_C5  523  // 高音do
#define NOTE_CS5 554  // 高音do升
#define NOTE_D5  587  // 高音re
#define NOTE_DS5 622  // 高音re升
#define NOTE_E5  659  // 高音mi
#define NOTE_F5  698  // 高音fa
#define NOTE_FS5 740  // 高音fa升
#define NOTE_G5  784  // 高音sol
#define NOTE_GS5 831  // 高音sol升
#define NOTE_A5  880  // 高音la
#define NOTE_AS5 932  // 高音la升
#define NOTE_B5  988  // 高音si

// 高八度音符 (6)
#define NOTE_C6  1047  // 高高音do
#define NOTE_CS6 1109  // 高高音do升
#define NOTE_D6  1175  // 高高音re
#define NOTE_DS6 1245  // 高高音re升
#define NOTE_E6  1319  // 高高音mi
#define NOTE_F6  1397  // 高高音fa
#define NOTE_FS6 1480  // 高高音fa升
#define NOTE_G6  1568  // 高高音sol
#define NOTE_GS6 1661  // 高高音sol升
#define NOTE_A6  1760  // 高高音la
#define NOTE_AS6 1865  // 高高音la升
#define NOTE_B6  1976  // 高高音si


// 休止符
#define NOTE_REST 0  // 休止符 (无声)


// 基本音符时值
#define W      2000    // 全音符 (2秒)
#define H       1000    // 二分音符 (1秒)
#define Q    500     // 四分音符 (0.5秒)
#define E     250     // 八分音符 (0.25秒)
#define S  125     // 十六分音符 (0.125秒)

// 带附点的音符
#define DH     1500    // 附点二分音符 (1.5秒)
#define DQ  750     // 附点四分音符 (0.75秒)
#define DE   375     // 附点八分音符 (0.375秒)

// 休止符
#define R      500     // 标准休止符 (四分休止符)
#define LR     1000    // 长休止符 (半休止符)
#define SR     250     // 短休止符 (八分休止符)





// IMU数据映射到音符和持续时间，并返回JSON格式字符串
String mapIMUToMusicJSON(const IMUData& imu, const String& existingJson = "[]");

#endif



