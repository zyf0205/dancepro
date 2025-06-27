#ifndef HTTP_H
#define HTTP_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <map>

// HTTP服务器状态结构体
struct HTTPServerStatus {
    bool isRunning;          // 服务器是否运行
    unsigned long uptime;    // 服务器运行时间(毫秒)
    unsigned long lastRequest; // 上次请求时间(毫秒)
    uint32_t requestCount;   // 请求计数
    uint16_t errorCount;     // 错误计数
    bool clientConnected;    // 是否有客户端连接
};

// 初始化HTTP服务器
void setupHTTPServer();

// 处理HTTP请求 (在loop中调用)
void handleHTTPRequests();

// 设置数据回调函数 - 当收到数据时调用
typedef std::function<void(const JsonDocument&)> DataCallback;
void setDataReceiveCallback(const String& endpoint, DataCallback callback);

// 发送数据到客户端
bool sendData(const JsonDocument& data);

// 获取HTTP服务器状态
HTTPServerStatus getHTTPServerStatus();

// 重启HTTP服务器
bool restartHTTPServer();

// 上传并替换音符数据 - 完全替换现有数据
bool uploadAndReplaceNoteData(const String& jsonData);

#endif
