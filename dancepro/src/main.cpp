#include <M5Unified.h>
#include <Arduino.h>
#include "imu/imu.h"
#include "http/http.h"
#include "wifi/my_wifi.h"
#include "note/note.h"
#include "home/home_ui.h"
#include "wifi/wifi_ui.h"
#include "note/note_ui.h"

//句柄
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t imuTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t httpTaskHandle = NULL;
TaskHandle_t noteTaskHandle = NULL;

//变量
extern IMUData ImuData;// imu数据
int page = 0;// 页面
int lastPage=0;
bool canSwitchPage = true;// 是否可以切换页面
bool isRecording = false;//是否正在录制
unsigned long recordStartTime = 0;  // 记录开始时间
unsigned long currentRecordTime = 0;  // 当前录制时间
bool isTimeInitialized = false;//是否初始化时间
String musicJSON = "";//音乐json字符串
extern bool noteUIRedrawNeeded;//是否需要重新绘制各个ui界面
extern bool wifiUIRedrawNeeded;
extern bool homeUIRedrawNeeded;

//传入任务句柄,删除任务
void deleteTask(TaskHandle_t &taskHandle)
{
  if (taskHandle != NULL)
  {
    // 删除任务并释放其资源
    vTaskDelete(taskHandle);
    // 将任务句柄设置为 NULL，表示任务不存在
    taskHandle = NULL;
    M5.Log.println("Task deleted successfully");
  }
  else
  {
    M5.Log.println("Task already deleted or doesn't exist");
  }
}

//通过imu数据判断手腕动作,并切换页面
void imu_task(void *pvParameters)
{
  ImuData = {0};
  float prevRoll = 0;                     // 记录上一次的roll角度
  const float threshold = 20;             // 定义角度变化阈值，可根据需要调整
  unsigned long lastPageChangeTime = 0;   // 上次页面切换时间
  const unsigned long debounceTime = 500; // 防抖时间（毫秒）
  // 状态变量，用于跟踪手腕动作
  enum WristState
  {
    NEUTRAL,     // 中立状态
    FLIPPED_UP,  // 已向上翻转
    FLIPPED_DOWN // 已向下翻转
  };
  WristState wristState = NEUTRAL;
  for (;;)
  {
    if (canSwitchPage)
    {
      updateIMUData(ImuData);
      // 记录当前时间
      unsigned long currentTime = millis();
      // 计算roll角度变化量
      float rollChange = ImuData.roll - prevRoll;
      // 基于角度变化和当前状态判断手腕动作
      if (currentTime - lastPageChangeTime > debounceTime)
      {
        //正在录制，不进行页面切换
        if(isRecording){
          lastPageChangeTime = currentTime;
          continue;
        }
        // 检测手腕向上翻转
        if (rollChange > threshold && wristState == NEUTRAL)
        {
          // 手腕从中立状态向上翻转，页面加1
          lastPage = page;
          page = (page + 1) % 3;
          M5.Log.printf("手腕向上翻转，页面切换到: %d\n", page);
          wristState = FLIPPED_UP;
          lastPageChangeTime = currentTime;
        }
        // 检测手腕向下翻转
        else if (rollChange < -threshold && wristState == NEUTRAL)
        {
          // 手腕从中立状态向下翻转，页面减1
          lastPage = page;
          page = (page > 0) ? (page - 1) : 2;
          M5.Log.printf("手腕向下翻转，页面切换到: %d\n", page);
          wristState = FLIPPED_DOWN;
          lastPageChangeTime = currentTime;
        }
        // 检测手腕是否回到中立位置
        else if (abs(rollChange) < 5 && (wristState == FLIPPED_UP || wristState == FLIPPED_DOWN))
        {
          // 手腕回到中立位置，重置状态但不改变页面
          lastPage = page;
          M5.Log.println("手腕回到中立位置");
          wristState = NEUTRAL;
        }
      }
      // 更新上一次的roll角度
      prevRoll = ImuData.roll;
    }
    vTaskDelay(150);
  }
}

//wifi连接任务
void wifi_task(void *pvParameters)
{
  setupWiFi();
  for (;;)
  {
    if(getWiFiStatus()==WIFI_CONNECTED&&!isTimeInitialized){
      isTimeInitialized=true;
      initTimeAsync();
    }
    monitorWiFi();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//ui显示任务
void ui_task(void *pvParameters)
{
  for (;;) 
  {
    switch(page){
      case 0://主界面
        if(lastPage!=page){
          homeUIRedrawNeeded=true;
          lastPage=page;
        }
        displayHomeUI();
        break;
      case 1://音符录制界面
        if(lastPage!=page){
          noteUIRedrawNeeded=true;
          lastPage=page;
        }
        if(isRecording){
          
          currentRecordTime = millis() - recordStartTime;
        }
        displayNoteUI(isRecording,currentRecordTime,ImuData);
        break;
      case 2://wifi界面
        if(lastPage!=page){
          wifiUIRedrawNeeded=true;
          lastPage=page;
        }
        //M5.Display.clear();
        displayWiFiUI(getWiFiStatus());
        break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//音乐处理任务
void note_task(void *pvParameters) {
  for (;;) {
    musicJSON = mapIMUToMusicJSON(ImuData,musicJSON);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

//通过按钮进入页面功能
void button_task(void *pvParameters)
{
  for (;;)
  {
    M5.update(); // 必须首先调用，更新按钮状态
    if(page==0&&M5.BtnA.wasPressed()){
      M5.Log.println("进入页面0");
    }
    if(page==1&&M5.BtnA.wasPressed()){
      M5.Log.println("进入页面1");
      isRecording = !isRecording;
      recordStartTime = millis();
      if(isRecording){
        musicJSON = "[]";
        xTaskCreate(note_task, "NoteTask", 4096, NULL, 1, &noteTaskHandle);
      }else{
        vTaskDelete(noteTaskHandle);
        M5.Log.println(musicJSON.c_str());
        uploadAndReplaceNoteData(musicJSON);
      }
    }
    if(page==2&&M5.BtnA.wasPressed()){
      M5.Log.println("进入页面2");
      resetWiFi();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// http 注册数据回调函数
void registerCallbacks() {
  // 注册处理设置的回调
  setDataReceiveCallback("settings", [](const JsonDocument& data) {
    // 处理设置
    if (!data["brightness"].isNull()) { // 修改为使用isNull()
      int brightness = data["brightness"].as<int>();
      M5.Display.setBrightness(brightness);//范围0-255
      M5.Log.printf("亮度已设置为: %d\n", brightness);
      // 这里可以添加亮度控制代码
    }
    if (!data["restart"].isNull()) { // 修改为使用isNull()
      bool restart = data["restart"].as<bool>();
      if(restart){
        M5.Log.println("正在重启设备...");
        ESP.restart();
      }
    }
    if(!data["sleep"].isNull()){
      bool sleep = data["sleep"].as<bool>();
      if(sleep){
        M5.Log.println("进入休眠模式...");
        M5.Power.deepSleep();
      }
    }
  });
}

// HTTP服务器任务
void http_task(void *pvParameters) {
  // 等待WiFi连接
  while (WiFi.status() != WL_CONNECTED) {
    M5.Log.println("等待WiFi连接...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  M5.Log.println("wifi连接成功，HTTP服务器任务开始\n");
  // 初始化HTTP服务器
  setupHTTPServer();
  // 注册数据回调函数
  registerCallbacks();
  // 任务循环
  for (;;) {
    // 处理HTTP请求
    handleHTTPRequests();
    // 检查服务器状态
    static unsigned long lastCheckTime = 0;
    if (millis() - lastCheckTime > 30000) { // 每30秒检查一次
      lastCheckTime = millis();
      HTTPServerStatus status = getHTTPServerStatus();
      if (!status.isRunning) {
        M5.Log.println("HTTP服务器未运行，尝试重启...");
        restartHTTPServer();
      }
    }
    // 给其他任务运行的时间
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//开始任务,用于创建其他任务
void start_task(void *pvParameters)
{
  //按钮任务
  xTaskCreate(button_task, "ButtonTask", 4096, NULL, 2, &buttonTaskHandle);
  //imu任务
  xTaskCreate(imu_task, "IMUTask", 4096, NULL, 1, &imuTaskHandle);
  //ui任务
  xTaskCreate(ui_task, "UITask", 4096, NULL, 1, &uiTaskHandle);
  //wifi任务
  xTaskCreate(wifi_task, "WifiTask", 4096, NULL, 1, &wifiTaskHandle);
  //http任务
  xTaskCreate(http_task, "HttpTask", 4096, NULL, 1, &httpTaskHandle);

  vTaskDelete(NULL);
}

//主函数
void setup()
{
  // 正确配置M5Unified，启用串口输出
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200; // 设置波特率
  M5.begin(cfg);
  M5.Log.println("M5.Log测试");
  // 创建开始任务
  xTaskCreate(start_task, "StartTask", 4096, NULL, 1, NULL);
}


void loop()
{
}