#ifndef IMU_H
#define IMU_H

// 用于存储IMU数据和姿态角的结构体
struct IMUData {
  // 姿态角数据 (以度为单位)
  float pitch;  // 俯仰角
  float roll;   // 横滚角
  float yaw;    // 偏航角
  
  // 原始传感器数据
  float accX, accY, accZ;    // 加速度计数据 (g)
  float gyroX, gyroY, gyroZ; // 陀螺仪数据 (dps)
  float magX, magY, magZ;    // 磁力计数据 (uT)
};

// 初始化IMU传感器
bool initIMU();

// 更新IMU数据
bool updateIMUData(IMUData& data);


#endif