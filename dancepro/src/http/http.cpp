#include "http/http.h"
#include <map> // 添加map头文件
#include <M5Unified.h>
// 创建HTTP服务器实例，端口80
WebServer server(80);

// 存储回调函数，stl map容器存储回调函数，key为String，value为DataCallback
std::map<String, DataCallback> dataCallbacks;

static bool serverRunning = false;        // 服务器是否运行
static unsigned long lastRequestTime = 0; // 最后一次请求时间
static uint16_t errorCount = 0;           // 错误计数

// 存储最新的音符映射数据
static String latestNoteMapData = "[]";
// 互斥锁,用于保护音符映射数据
static SemaphoreHandle_t noteMapMutex = NULL;

// 初始化HTTP服务器
void setupHTTPServer()
{
    // 创建互斥锁，防止多个线程同时访问音符映射数据导致json损坏
    if (noteMapMutex == NULL)
    {
        noteMapMutex = xSemaphoreCreateMutex();
    }
    // 设置CORS头部，允许跨域访问
    server.enableCORS(true);
    // 数据API - 发送和接收数据
    server.on("/api/data", HTTP_POST, []()
              {
        lastRequestTime = millis();
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
            if (!doc["action"].isNull()) {
                // 获取action
                String action = doc["action"].as<String>();
                
                // 查找对应的回调函数
                if (dataCallbacks.find(action) != dataCallbacks.end()) {
                    // 调用回调函数
                    dataCallbacks[action](doc);
                    
                    // 返回成功响应
                    server.send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    // 未知action
                    errorCount++;
                    server.send(404, "application/json", "{\"error\":\"Unknown action\"}");
                }
            } else {
                // 没有action字段
                errorCount++;
                server.send(400, "application/json", "{\"error\":\"Missing 'action' field\"}");
            }
        } else {
            // 没有数据
            errorCount++;
            server.send(400, "application/json", "{\"error\":\"No data provided\"}");
        } });

    // 音符数据API - 获取最新的音符数据
    server.on("/api/notes", HTTP_GET, []()
              {
        lastRequestTime = millis();
        
        if (xSemaphoreTake(noteMapMutex, portMAX_DELAY)) {
            String response = latestNoteMapData;
            xSemaphoreGive(noteMapMutex);
            server.send(200, "application/json", response);
        } else {
            server.send(500, "application/json", "{\"error\":\"Failed to access note data\"}");
        } });

    // CORS预检请求处理
    server.on("/api/data", HTTP_OPTIONS, []()
              { server.send(200); });

    server.on("/api/notes", HTTP_OPTIONS, []()
              { server.send(200); });

    // 404处理
    server.onNotFound([]()
                      {
        errorCount++;
        server.send(404, "application/json", "{\"error\":\"Not found\"}"); });

    // 启动服务器
    server.begin();
    serverRunning = true;
    M5.Log.println("[HTTP] 服务器已启动，端口80");
    M5.Log.print("[HTTP] 访问地址 http://");
    M5.Log.println(WiFi.localIP().toString().c_str());
}

// 处理HTTP请求
void handleHTTPRequests()
{
    if (serverRunning)
    {
        server.handleClient();
    }
}

// 设置数据回调函数
void setDataReceiveCallback(const String &endpoint, DataCallback callback)
{
    dataCallbacks[endpoint] = callback;
}

// 发送数据到客户端
bool sendData(const JsonDocument &data)
{
    // 这个函数可以用于主动向连接的客户端发送数据
    // 由于HTTP是请求-响应模式，这里实现为返回最后一次请求的响应

    if (!serverRunning || !server.client().connected())
    {
        return false;
    }

    String response;
    serializeJson(data, response);
    server.send(200, "application/json", response);
    return true;
}

// 获取HTTP服务器状态
HTTPServerStatus getHTTPServerStatus()
{
    HTTPServerStatus status;

    status.isRunning = serverRunning;
    status.lastRequest = lastRequestTime;
    status.errorCount = errorCount;
    status.clientConnected = server.client().connected();

    return status;
}

// 重启HTTP服务器
bool restartHTTPServer()
{
    if (serverRunning)
    {
        server.stop();
        serverRunning = false;
        delay(500); // 给服务器一些时间关闭
    }

    // 重新启动服务器
    setupHTTPServer();

    return serverRunning;
}

// 上传并替换音符数据
bool uploadAndReplaceNoteData(const String &jsonData)
{
    if (noteMapMutex == NULL)
    {
        noteMapMutex = xSemaphoreCreateMutex();
    }

    // 检查数据大小
    if (jsonData.length() > 20000)
    { // 设置合理的大小限制
        M5.Log.println("[HTTP] 音符数据过大，无法上传");
        return false;
    }

    if (xSemaphoreTake(noteMapMutex, 1000 / portTICK_PERIOD_MS))
    { // 添加超时
        // 先释放旧数据内存
        latestNoteMapData = "";
        // 确保有足够内存后再复制
        latestNoteMapData = jsonData;

        M5.Log.printf("[HTTP] 音符数据已更新，大小: %d 字节\n", jsonData.length());
        xSemaphoreGive(noteMapMutex);
        return true;
    }

    M5.Log.println("[HTTP] 无法获取互斥锁，音符数据更新失败");
    return false;
}
