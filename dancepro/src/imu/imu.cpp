#include "imu/imu.h"
#include <math.h>
#include <M5Unified.h>
// 全局变量实例
IMUData ImuData = {0};


// 初始化IMU传感器
bool initIMU() {
  // 检查IMU是否可用
  if (!M5.Imu.begin()) {
    M5.Log.println("IMU initialization failed!");
    return false;
  }
  // 初始化数据结构
  memset(&ImuData, 0, sizeof(IMUData));
  // 首次更新IMU数据
  return updateIMUData(ImuData);
}

// 更新IMU数据函数，从传感器读取数据并转换为姿态角
bool updateIMUData(IMUData& data) {
  // 使用M5.Imu.update()更新传感器数据
  bool updated = M5.Imu.update();
  if (updated) {
    // 获取IMU原始数据
    auto imuData = M5.Imu.getImuData();
    // 1. 保存原始传感器数据
    data.accX = imuData.accel.x;
    data.accY = imuData.accel.y;
    data.accZ = imuData.accel.z;
    
    data.gyroX = imuData.gyro.x;
    data.gyroY = imuData.gyro.y;
    data.gyroZ = imuData.gyro.z;
    
    data.magX = imuData.mag.x;
    data.magY = imuData.mag.y;
    data.magZ = imuData.mag.z;
    
    // 2. 计算姿态角 (转换为欧拉角)
    // 计算 roll (沿X轴旋转)
    data.roll = atan2(data.accY, data.accZ) * 180.0 / M_PI;
    // 计算 pitch (沿Y轴旋转)
    data.pitch = atan2(-data.accX, sqrt(data.accY * data.accY + data.accZ * data.accZ)) * 180.0 / M_PI;
    // 计算 yaw (沿Z轴旋转)
    // 使用磁力计数据，并根据倾斜角度进行校正
    float magX_comp, magY_comp;
    // 根据pitch和roll角度补偿磁力计读数
    magX_comp = data.magX * cos(data.pitch * M_PI / 180.0) + 
                data.magZ * sin(data.pitch * M_PI / 180.0);
                
    magY_comp = data.magX * sin(data.roll * M_PI / 180.0) * sin(data.pitch * M_PI / 180.0) +
                data.magY * cos(data.roll * M_PI / 180.0) - 
                data.magZ * sin(data.roll * M_PI / 180.0) * cos(data.pitch * M_PI / 180.0);
    // 计算补偿后的偏航角
    data.yaw = atan2(magY_comp, magX_comp) * 180.0 / M_PI;
    // 将偏航角转换为0-360度范围
    if (data.yaw < 0) {
      data.yaw += 360.0;
    }
    // 更新全局数据
    ImuData = data;
  }
  return updated;
}