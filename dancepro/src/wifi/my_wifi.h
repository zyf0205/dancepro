#ifndef _MY_WIFI_H
#define _MY_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <M5Unified.h>

// WiFi状态枚举
enum WiFiStatus {
    WIFI_INIT,        // 初始化
    WIFI_AP_MODE,     // AP模式(热点)
    WIFI_CONNECTING,  // 正在连接
    WIFI_CONNECTED,   // 已连接
    WIFI_FAILED       // 连接失败
};

// 初始化WiFi - 在setup中调用一次
void setupWiFi();

// 监控WiFi状态 - 在loop中定期调用
void monitorWiFi();

// 重置WiFi设置并进入AP模式
void resetWiFi();

// 获取当前WiFi状态
WiFiStatus getWiFiStatus();

// 获取IP地址
String getLocalIP();

// AP热点名称 - 供UI使用
extern const char* AP_NAME;

#endif
