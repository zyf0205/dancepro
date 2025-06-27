#include "http/http.h"
#include <map> // 添加map头文件
#include <M5Unified.h>
// 创建HTTP服务器实例，端口80
WebServer server(80);

// 存储回调函数
std::map<String, DataCallback> dataCallbacks;

// 服务器状态跟踪
static bool serverRunning = false;
static unsigned long serverStartTime = 0;
static unsigned long lastRequestTime = 0;
static uint32_t totalRequestCount = 0;
static uint16_t errorCount = 0;

// 存储最新的音符映射数据
static String latestNoteMapData = "[]";
static unsigned long noteMapUpdateTime = 0;
static SemaphoreHandle_t noteMapMutex = NULL;

// 初始化HTTP服务器
void setupHTTPServer() {
    // 创建互斥锁
    if (noteMapMutex == NULL) {
        noteMapMutex = xSemaphoreCreateMutex();
    }
    
    // 设置CORS头部，允许跨域访问
    server.enableCORS(true); // 使用WebServer内置的CORS支持，替代DefaultHeaders
    
    // 设置路由
    
    // 1. 根路径 - 返回简单的HTML页面
    server.on("/", HTTP_GET, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        
        String html = "<!DOCTYPE html><html><head><title>ESP32 API</title>";
        html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>body{font-family:Arial;margin:20px;text-align:center;}";
        html += "div{margin:10px;padding:10px;background:#f8f8f8;border-radius:5px;}";
        html += "h1{color:#0066cc;}</style></head><body>";
        html += "<h1>ESP32 HTTP Server</h1>";
        html += "<div><p>Status: Online</p>";
        html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
        html += "<p>Available endpoints:</p>";
        html += "<p>/api/status - Get device status</p>";
        html += "<p>/api/data - Send/receive data</p>";
        html += "<p>/api/server - Get server status</p>";
        html += "<p>/api/notes - Get musical notes data</p></div>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    });
    
    // 2. 状态API - 返回设备状态信息
    server.on("/api/status", HTTP_GET, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        
        JsonDocument doc;
        doc["status"] = "online";
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
    
    // 3. 服务器状态API - 返回HTTP服务器状态
    server.on("/api/server", HTTP_GET, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        
        HTTPServerStatus status = getHTTPServerStatus();
        
        JsonDocument doc;
        doc["running"] = status.isRunning;
        doc["uptime"] = status.uptime / 1000; // 转换为秒
        doc["last_request"] = (millis() - status.lastRequest) / 1000; // 多少秒前
        doc["request_count"] = status.requestCount;
        doc["error_count"] = status.errorCount;
        doc["client_connected"] = status.clientConnected;
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
    
    // 4. 数据API - 发送和接收数据
    server.on("/api/data", HTTP_POST, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        
        // 检查是否有数据
        if (server.hasArg("plain")) {
            String jsonStr = server.arg("plain");
            
            // 解析JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonStr);
            
            if (error) {
                // JSON解析错误
                errorCount++;
                server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
                return;
            }
            
            // 检查是否有action字段
            if (!doc["action"].isNull()) { // 修改为使用isNull()
                // 获取action
                String action = doc["action"].as<String>();
                
                // 查找对应的回调函数
                if (dataCallbacks.find(action) != dataCallbacks.end()) {
                    // 调用回调函数
                    dataCallbacks[action](doc);
                    
                    // 返回成功响应
                    server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Data received\"}");
                } else {
                    // 未知action
                    errorCount++;
                    server.send(404, "application/json", "{\"error\":\"Unknown action\"}");
                }
            } else {
                // 没有action字段
                errorCount++;
                server.send(400, "application/json", "{\"error\":\"Missing 'action' field\"}");
                return;
            }
        } else {
            // 没有数据
            errorCount++;
            server.send(400, "application/json", "{\"error\":\"No data provided\"}");
        }
    });
    
    // 5. 音符数据API - 获取最新的音符数据
    server.on("/api/notes", HTTP_GET, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        
        if (xSemaphoreTake(noteMapMutex, portMAX_DELAY)) {
            // 构建响应
            // String response = "{\"notes\":";
            // response += latestNoteMapData;
            // response += ",\"timestamp\":";
            // response += noteMapUpdateTime;
            // response += ",\"update_time\":\"";
            // response += String(noteMapUpdateTime);
            // response += "\"}";
            String response = latestNoteMapData;
            xSemaphoreGive(noteMapMutex);
            server.send(200, "application/json", response);
        } else {
            server.send(500, "application/json", "{\"error\":\"Failed to access note data\"}");
        }
    });
    
    // 6. OPTIONS请求处理 (用于CORS预检)
    server.on("/api/data", HTTP_OPTIONS, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        server.send(200);
    });
    
    server.on("/api/notes", HTTP_OPTIONS, []() {
        lastRequestTime = millis();
        totalRequestCount++;
        server.send(200);
    });
    
    // 7. 404处理
    server.onNotFound([]() {
        lastRequestTime = millis();
        totalRequestCount++;
        errorCount++;
        server.send(404, "application/json", "{\"error\":\"Not found\"}");
    });
    
    // 启动服务器
    server.begin();
    serverRunning = true;
    serverStartTime = millis();
    Serial.println("[HTTP] Server started on port 80");
    Serial.print("[HTTP] Access at http://");
    Serial.println(WiFi.localIP().toString());
}

// 处理HTTP请求
void handleHTTPRequests() {
    if (serverRunning) {
        server.handleClient();
    }
}

// 设置数据回调函数
void setDataReceiveCallback(const String& endpoint, DataCallback callback) {
    dataCallbacks[endpoint] = callback;
}

// 发送数据到客户端
bool sendData(const JsonDocument& data) {
    // 这个函数可以用于主动向连接的客户端发送数据
    // 由于HTTP是请求-响应模式，这里实现为返回最后一次请求的响应
    
    if (!serverRunning || !server.client().connected()) {
        return false;
    }
    
    String response;
    serializeJson(data, response);
    server.send(200, "application/json", response);
    return true;
}

// 获取HTTP服务器状态
HTTPServerStatus getHTTPServerStatus() {
    HTTPServerStatus status;
    
    status.isRunning = serverRunning;
    status.uptime = serverRunning ? (millis() - serverStartTime) : 0;
    status.lastRequest = lastRequestTime;
    status.requestCount = totalRequestCount;
    status.errorCount = errorCount;
    status.clientConnected = server.client().connected();
    
    return status;
}

// 重启HTTP服务器
bool restartHTTPServer() {
    if (serverRunning) {
        server.stop();
        serverRunning = false;
        delay(500); // 给服务器一些时间关闭
    }
    
    // 重新启动服务器
    setupHTTPServer();
    
    return serverRunning;
}

// 上传并替换音符数据
bool uploadAndReplaceNoteData(const String& jsonData) {
    if (noteMapMutex == NULL) {
        noteMapMutex = xSemaphoreCreateMutex();
    }
    
    if (xSemaphoreTake(noteMapMutex, portMAX_DELAY)) {
        latestNoteMapData = jsonData;
        noteMapUpdateTime = millis();
        
        M5.Log.printf("[HTTP] 音符数据已更新，大小: %d 字节\n", jsonData.length());
        xSemaphoreGive(noteMapMutex);
        return true;
    }
    
    M5.Log.println("[HTTP] 无法获取互斥锁，音符数据更新失败");
    return false;
}
