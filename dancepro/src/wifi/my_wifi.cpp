#include "wifi/my_wifi.h"
#include <M5Unified.h>

// WiFi管理对象
WiFiManager wifiManager;

// 当前WiFi状态
WiFiStatus wifiStatus = WIFI_INIT;

// AP模式配置 - 全局可见
const char* AP_NAME = "ESP32_S3R"; // AP热点名称
const char* AP_PASSWORD = "";        // 无密码

// 连接超时时间(毫秒)
const unsigned long CONNECT_TIMEOUT = 15000;

// 连接尝试开始时间
unsigned long connectStartTime = 0;

// 获取WiFi状态
WiFiStatus getWiFiStatus() {
    return wifiStatus;
}

// 获取IP地址
String getLocalIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    } else {
        return WiFi.softAPIP().toString();
    }
}

// 启动配置门户（AP模式）
void startConfigPortal() {
    M5.Log.printf("[WiFi] 启动AP模式: %s\n", AP_NAME);
    
    wifiStatus = WIFI_AP_MODE;
    
    // 启动配置门户，阻塞直到配置完成或超时
    if (wifiManager.startConfigPortal(AP_NAME, AP_PASSWORD)) {
        // 用户完成配置
        M5.Log.printf("[WiFi] 配网成功，已连接到: %s\n", WiFi.SSID().c_str());
        M5.Log.printf("[WiFi] IP地址: %s\n", WiFi.localIP().toString().c_str());
        wifiStatus = WIFI_CONNECTED;
    } else {
        // 配置门户超时
        M5.Log.println("[WiFi] 配置门户超时，未能配网");
        wifiStatus = WIFI_FAILED;
    }
}

// 初始化WiFi配置 - 在setup中调用一次
void setupWiFi() {
    M5.Log.println("[WiFi] 初始化WiFi配置...");
    
    // 配置WiFiManager
    wifiManager.setDebugOutput(true);                // 启用调试输出
    wifiManager.setMinimumSignalQuality(30);         // 设置最小信号质量
    wifiManager.setRemoveDuplicateAPs(true);         // 移除重复AP
    wifiManager.setConfigPortalTimeout(180);         // 配置门户超时时间(秒)
    wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0)); // 设置AP静态IP
    
    // 设置设备名称
    String hostname = "ESP32-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);
    wifiManager.setHostname(hostname.c_str());
    
    // 注册WiFi保存回调
    wifiManager.setSaveConfigCallback([]() {
        M5.Log.println("[WiFi] 配网信息已保存到闪存，正在连接...");
        wifiStatus = WIFI_CONNECTING;
    });
    
    // 开始尝试连接
    M5.Log.println("[WiFi] 尝试连接已保存的WiFi...");
    wifiStatus = WIFI_CONNECTING;
    connectStartTime = millis();
    
    // 非阻塞方式开始连接
    WiFi.begin();

    // 在setupWiFi()函数中添加以下代码
    wifiManager.setCaptivePortalEnable(true);
    wifiManager.setAPCallback([](WiFiManager* wifiManager) {
        M5.Log.println("[WiFi] 进入AP模式，请连接到热点并访问 http://192.168.4.1");
    });
}

// 监控WiFi状态 - 在loop中定期调用
void monitorWiFi() {
    // 根据当前状态处理
    switch (wifiStatus) {
        case WIFI_CONNECTING:
            // 检查是否已连接
            if (WiFi.status() == WL_CONNECTED) {
                M5.Log.printf("[WiFi] 已连接到WiFi: %s\n", WiFi.SSID().c_str());
                M5.Log.printf("[WiFi] IP地址: %s\n", WiFi.localIP().toString().c_str());
                wifiStatus = WIFI_CONNECTED;
            }
            // 检查是否连接超时
            else if (millis() - connectStartTime > CONNECT_TIMEOUT) {
                M5.Log.println("[WiFi] 连接超时");
                wifiStatus = WIFI_FAILED;
            }
            break;
            
        case WIFI_CONNECTED:
            // 检查是否断开连接
            if (WiFi.status() != WL_CONNECTED) {
                M5.Log.println("[WiFi] 连接已断开");
                wifiStatus = WIFI_FAILED;
                
                // 尝试重新连接
                M5.Log.println("[WiFi] 尝试重新连接...");
                wifiStatus = WIFI_CONNECTING;
                connectStartTime = millis();
                WiFi.begin();
            }
            break;
            
        case WIFI_FAILED:
            // 间隔一段时间后自动尝试重新连接
            static unsigned long lastRetryTime = 0;
            if (millis() - lastRetryTime > 10000) { // 10秒后重试
                lastRetryTime = millis();
                M5.Log.println("[WiFi] 尝试重新连接...");
                wifiStatus = WIFI_CONNECTING;
                connectStartTime = millis();
                WiFi.begin();
            }
            break;
            
        default:
            break;
    }
}

// 重置WiFi设置并进入AP模式
void resetWiFi() {
    M5.Log.println("[WiFi] 重置WiFi设置..."); 
    
    // 清除保存的WiFi凭证
    wifiManager.resetSettings();
    
    // 确保captive portal功能开启
    wifiManager.setCaptivePortalEnable(true);
    
    M5.Log.println("[WiFi] WiFi凭证已清除，启动配置门户");
    M5.Log.println("[WiFi] 请连接到ESP32_S3R热点，然后访问http://192.168.4.1");
    
    // 启动AP模式配置门户
    startConfigPortal();
}
