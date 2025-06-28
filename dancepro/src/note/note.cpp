#include "note/note.h"
#include <math.h>
#include <ArduinoJson.h>

// 存储状态和结构信息
static unsigned long lastNoteTime = 0;
static int lastNote = -1;
static int currentMeasure = 0;  // 当前小节
static int currentBeat = 0;     // 当前拍子位置
static int currentSection = 0;  // 当前段落 (A, B, C, 尾声)
static bool isPlaying = false;  // 是否正在播放

// 定义更广泛的音符范围，确保使用更多中高音区
// 中音区音阶
const int MID_SCALE[] = {
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4
};

// 高音区音阶
const int HIGH_SCALE[] = {
    NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5
};

// 定义和弦 - 增加中高音区和弦
const int C_CHORD[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};    // C和弦
const int G_CHORD[] = {NOTE_G3, NOTE_B3, NOTE_D4, NOTE_G4};    // G和弦
const int AM_CHORD[] = {NOTE_A3, NOTE_C4, NOTE_E4, NOTE_A4};   // Am和弦
const int F_CHORD[] = {NOTE_F3, NOTE_A3, NOTE_C4, NOTE_F4};    // F和弦

// 高八度和弦 - 用于B段和C段
const int C_CHORD_HIGH[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5};    // C和弦高八度
const int G_CHORD_HIGH[] = {NOTE_G3, NOTE_B3, NOTE_D4, NOTE_G4, NOTE_B4};    // G和弦高八度
const int AM_CHORD_HIGH[] = {NOTE_A3, NOTE_C4, NOTE_E4, NOTE_A4, NOTE_C5};   // Am和弦高八度
const int F_CHORD_HIGH[] = {NOTE_F3, NOTE_A3, NOTE_C4, NOTE_F4, NOTE_A4};    // F和弦高八度

// 定义各段落的和弦进行 (每小节一个和弦)
const int* A_SECTION_CHORDS[] = {
    C_CHORD, G_CHORD, AM_CHORD, F_CHORD,
    C_CHORD, G_CHORD, AM_CHORD, F_CHORD,
    C_CHORD, G_CHORD, AM_CHORD, F_CHORD,
    C_CHORD, G_CHORD, AM_CHORD, F_CHORD
};

const int* B_SECTION_CHORDS[] = {
    F_CHORD_HIGH, C_CHORD_HIGH, G_CHORD_HIGH, AM_CHORD_HIGH,
    F_CHORD_HIGH, C_CHORD_HIGH, G_CHORD_HIGH, AM_CHORD_HIGH,
    F_CHORD_HIGH, C_CHORD_HIGH, G_CHORD_HIGH, AM_CHORD_HIGH,
    F_CHORD_HIGH, C_CHORD_HIGH, G_CHORD_HIGH, AM_CHORD_HIGH
};

const int* C_SECTION_CHORDS[] = {
    C_CHORD_HIGH, AM_CHORD_HIGH, F_CHORD_HIGH, G_CHORD_HIGH,
    C_CHORD_HIGH, AM_CHORD_HIGH, F_CHORD_HIGH, G_CHORD_HIGH
};

const int* OUTRO_CHORDS[] = {
    F_CHORD_HIGH, G_CHORD_HIGH, AM_CHORD_HIGH, C_CHORD_HIGH,
    F_CHORD_HIGH, G_CHORD_HIGH, AM_CHORD_HIGH, C_CHORD_HIGH
};

// 定义4/4拍基本节奏单位 (单位: ms)
const int BEAT_UNIT = 300;  // 基本单位为300ms

// 基本4/4拍节奏 (每拍一个音符)
const int RHYTHM_BASIC[] = {
    BEAT_UNIT, BEAT_UNIT, BEAT_UNIT, BEAT_UNIT
};

// 带重拍的4/4拍节奏 (第一拍加重)
const int RHYTHM_ACCENTED[] = {
    BEAT_UNIT*5/3, BEAT_UNIT, BEAT_UNIT, BEAT_UNIT
};

// 切分音节奏
const int RHYTHM_SYNCOPATION[] = {
    BEAT_UNIT/2, BEAT_UNIT/2, BEAT_UNIT, BEAT_UNIT/2, BEAT_UNIT/2, BEAT_UNIT
};

// 三连音节奏
const int RHYTHM_TRIPLET[] = {
    BEAT_UNIT, BEAT_UNIT*2/3, BEAT_UNIT*2/3, BEAT_UNIT*2/3, BEAT_UNIT
};

// 段落结束节奏 (带长音)
const int RHYTHM_SECTION_END[] = {
    BEAT_UNIT, BEAT_UNIT, BEAT_UNIT, BEAT_UNIT*5/3
};

// 尾声渐慢节奏
const int RHYTHM_ENDING[] = {
    BEAT_UNIT, BEAT_UNIT, BEAT_UNIT*2, BEAT_UNIT*3
};

// 各节奏模式的长度
const int RHYTHM_BASIC_LENGTH = 4;
const int RHYTHM_ACCENTED_LENGTH = 4;
const int RHYTHM_SYNCOPATION_LENGTH = 6;
const int RHYTHM_TRIPLET_LENGTH = 5;
const int RHYTHM_SECTION_END_LENGTH = 4;
const int RHYTHM_ENDING_LENGTH = 4;

// 音乐结构定义 - 每个小节使用的节奏类型
// 0=基本, 1=带重拍, 2=切分音, 3=三连音, 4=段落结束, 5=尾声
const int A_SECTION_RHYTHMS[] = {
    0, 0, 0, 1,  // 1-4小节
    0, 0, 2, 2,  // 5-8小节 (7-8使用切分音)
    0, 0, 0, 1,  // 9-12小节
    0, 0, 0, 4   // 13-16小节 (16小节用段落结束节奏)
};

const int B_SECTION_RHYTHMS[] = {
    1, 0, 0, 3,  // 17-20小节 (17带重拍，20使用三连音)
    3, 0, 2, 2,  // 21-24小节 (21三连音，23-24切分音)
    2, 2, 0, 1,  // 25-28小节 (25-26切分音，28带重拍)
    0, 0, 0, 4   // 29-32小节 (32小节用段落结束节奏)
};

const int C_SECTION_RHYTHMS[] = {
    1, 0, 3, 0,  // 33-36小节 (33带重拍，35三连音)
    0, 3, 0, 4   // 37-40小节 (38三连音，40段落结束)
};

const int OUTRO_RHYTHMS[] = {
    1, 3, 0, 0,  // 41-44小节 (41带重拍，42三连音)
    0, 0, 0, 5   // 45-48小节 (48使用尾声节奏)
};

// 休止符位置 (小节号，从0开始计数)
const int REST_MEASURES[] = {7, 15, 23, 31, 35, 39};
const int REST_MEASURES_COUNT = 6;

// IMU数据映射到音符和持续时间，并将结果添加到现有JSON字符串中
String mapIMUToMusicJSON(const IMUData& imu, const String& existingJson) {
    // 计算设备状态
    float tiltAngle = atan2(sqrt(imu.accX*imu.accX + imu.accY*imu.accY), imu.accZ) * 180.0 / PI;
    if (tiltAngle > 90) tiltAngle = 180 - tiltAngle;
    
    float direction = atan2(imu.magY, imu.magX) * 180.0 / PI;
    if (direction < 0) direction += 360;
    
    float gyroTotal = sqrt(imu.gyroX*imu.gyroX + imu.gyroY*imu.gyroY + imu.gyroZ*imu.gyroZ);
    
    // 记录当前时间
    unsigned long currentTime = millis();
    
    // 初始化或重置音乐播放
    if (!isPlaying || currentTime - lastNoteTime > 10000) {  // 10秒无操作重置
        currentMeasure = 0;
        currentBeat = 0;
        currentSection = 0;
        isPlaying = true;
        lastNoteTime = currentTime;
    }
    
    // 控制音符生成速度 - 保持音符密集度
    if (currentTime - lastNoteTime < 250) {  // 平均每250ms一个音符
        return existingJson;
    }
    
    // 确定当前所在的音乐段落
    int totalMeasures = currentMeasure;
    if (totalMeasures < 16) {
        currentSection = 0;  // A段 (1-16小节)
    } else if (totalMeasures < 32) {
        currentSection = 1;  // B段 (17-32小节)
    } else if (totalMeasures < 40) {
        currentSection = 2;  // C段 (33-40小节)
    } else {
        currentSection = 3;  // 尾声 (41-48小节)
    }
    
    // 根据当前段落选择和弦和节奏
    const int* currentChord;
    int rhythmType;
    
    switch (currentSection) {
        case 0:  // A段
            currentChord = A_SECTION_CHORDS[currentMeasure % 16];
            rhythmType = A_SECTION_RHYTHMS[currentMeasure % 16];
            break;
        case 1:  // B段
            currentChord = B_SECTION_CHORDS[(currentMeasure - 16) % 16];
            rhythmType = B_SECTION_RHYTHMS[(currentMeasure - 16) % 16];
            break;
        case 2:  // C段
            currentChord = C_SECTION_CHORDS[(currentMeasure - 32) % 8];
            rhythmType = C_SECTION_RHYTHMS[(currentMeasure - 32) % 8];
            break;
        case 3:  // 尾声
        default:
            currentChord = OUTRO_CHORDS[(currentMeasure - 40) % 8];
            rhythmType = OUTRO_RHYTHMS[(currentMeasure - 40) % 8];
            break;
    }
    
    // 选择当前节奏
    const int* currentRhythm;
    int rhythmLength;
    
    switch (rhythmType) {
        case 1:  // 带重拍的节奏
            currentRhythm = RHYTHM_ACCENTED;
            rhythmLength = RHYTHM_ACCENTED_LENGTH;
            break;
        case 2:  // 切分音节奏
            currentRhythm = RHYTHM_SYNCOPATION;
            rhythmLength = RHYTHM_SYNCOPATION_LENGTH;
            break;
        case 3:  // 三连音节奏
            currentRhythm = RHYTHM_TRIPLET;
            rhythmLength = RHYTHM_TRIPLET_LENGTH;
            break;
        case 4:  // 段落结束节奏
            currentRhythm = RHYTHM_SECTION_END;
            rhythmLength = RHYTHM_SECTION_END_LENGTH;
            break;
        case 5:  // 尾声节奏
            currentRhythm = RHYTHM_ENDING;
            rhythmLength = RHYTHM_ENDING_LENGTH;
            break;
        case 0:  // 基本节奏
        default:
            currentRhythm = RHYTHM_BASIC;
            rhythmLength = RHYTHM_BASIC_LENGTH;
            break;
    }
    
    // 获取当前节奏的持续时间
    int duration = currentRhythm[currentBeat % rhythmLength];
    
    // 确定音符
    int note;
    
    // 检查是否是休止符位置
    bool isRestMeasure = false;
    for (int i = 0; i < REST_MEASURES_COUNT; i++) {
        if (currentMeasure == REST_MEASURES[i]) {
            isRestMeasure = true;
            break;
        }
    }
    
    if (isRestMeasure && currentBeat == 0) {
        // 在指定的小节第一拍插入休止符
        note = NOTE_REST;
        duration = BEAT_UNIT;  // 休止符持续一个基本单位
    } else {
        // 根据当前段落和和弦选择音符
        
        // 根据段落和位置选择音符范围
        int chordSize = (currentSection == 0) ? 4 : 5;  // A段使用4音和弦，其他段落使用5音和弦
        
        // 从和弦中选择音符，确保使用更多中高音区
        int chordIndex;
        
        // 根据段落和拍子位置选择和弦音符索引
        switch (currentSection) {
            case 0:  // A段 - 基础舞蹈主题
                // 在A段中，逐渐引入高音
                if (currentMeasure < 8) {
                    // 前半部分主要使用中低音区
                    chordIndex = currentBeat % 3;  // 使用和弦的前3个音
                } else {
                    // 后半部分增加高音使用
                    chordIndex = currentBeat % 4;  // 使用和弦的所有4个音
                }
                break;
                
            case 1:  // B段 - 增加音高变化
                // B段大量使用中高音区
                if (currentBeat == 0) {
                    // 第一拍使用和弦根音
                    chordIndex = 0;
                } else if (currentBeat == 2) {
                    // 第三拍使用高音区
                    chordIndex = 3 + (currentBeat % 2);  // 使用和弦的第4或第5个音
                } else {
                    // 其他拍位置随机选择
                    chordIndex = random(chordSize);
                }
                break;
                
            case 2:  // C段 - 加入休止符创造停顿
                // C段使用更多高音区
                if (currentBeat == 0) {
                    // 第一拍使用高八度
                    chordIndex = min(3, chordSize-1);  // 使用和弦的高音部分
                } else {
                    // 其他拍位置在整个和弦范围内选择
                    chordIndex = random(chordSize);
                }
                break;
                
            case 3:  // 尾声 - 渐强结束
                // 尾声部分使用全音域
                if (currentBeat == 0) {
                    // 第一拍使用和弦根音
                    chordIndex = 0;
                } else if (currentMeasure >= 44) {
                    // 最后几小节使用高音
                    chordIndex = min(currentBeat + 1, chordSize-1);
                } else {
                    // 其他位置在整个和弦范围内选择
                    chordIndex = random(chordSize);
                }
                break;
                
            default:
                chordIndex = currentBeat % chordSize;
                break;
        }
        
        // 获取基础音符
        note = currentChord[chordIndex];
        
        // 根据段落特性进行音符调整
        switch (currentSection) {
            case 0:  // A段 - 基础舞蹈主题
                // 每8个音符出现一次500ms长音 (重拍标记)
                if ((currentMeasure % 4 == 3) && (currentBeat == 0)) {
                    note = currentChord[min(3, chordSize-1)];  // 使用和弦高音
                    duration = BEAT_UNIT * 5/3;  // 稍微延长
                }
                break;
                
            case 1:  // B段 - 增加音高变化
                // 在切分音节奏中使用更高的音符
                if (rhythmType == 2 && currentBeat % 2 == 1) {
                    // 在切分音的弱拍部分使用高音区
                    note = HIGH_SCALE[random(7)];
                }
                
                // 第24和32小节使用长音标记段落
                if ((currentMeasure == 23 || currentMeasure == 31) && currentBeat == 3) {
                    note = currentChord[chordSize-1];  // 使用和弦最高音
                    duration = BEAT_UNIT * 5/3;  // 延长音符
                }
                break;
                
            case 2:  // C段 - 加入休止符创造停顿
                // 使用更高八度的音符增加变化
                if (rhythmType == 3) {  // 三连音节奏
                    // 在三连音中使用高音区音阶音符
                    if (currentBeat >= 1) {  // 三连音的后几个音符
                        note = HIGH_SCALE[random(7)];
                    }
                }
                break;
                
            case 3:  // 尾声 - 渐强结束
                // 最后一个小节使用渐慢节奏
                if (currentMeasure == 47) {
                    if (currentBeat == 3) {
                        note = NOTE_C5;  // 高音C结束
                        duration = 1000;  // 长音
                    }
                }
                break;
        }
    }
    
    // 根据IMU数据动态调整音符和持续时间
    
    // 根据倾斜角度调整音高
    if (tiltAngle > 60 && note != NOTE_REST) {
        // 大幅倾斜，使用中音区
        if (note >= NOTE_C5) note = MID_SCALE[random(7)];
    } else if (tiltAngle < 20 && note != NOTE_REST) {
        // 接近水平，使用高音区
        if (note <= NOTE_B4) note = HIGH_SCALE[random(7)];
    }
    
    // 根据陀螺仪数据调整持续时间 - 适应舞蹈动作
    if (gyroTotal > 200) {
        // 快速旋转，适合三连音
        if (duration > BEAT_UNIT) {
            // 将长音符分解为更短的音符
            duration = BEAT_UNIT * 2/3;
        }
    } else if (gyroTotal < 50 && currentSection == 3) {
        // 缓慢移动且在尾声部分，延长音符
        duration = duration * 1.2;
    }
    
    // 避免连续相同音符
    if (note == lastNote && note != NOTE_REST) {
        // 根据当前段落选择替代音符
        switch (currentSection) {
            case 0:  // A段
                note = MID_SCALE[random(7)];  // 使用中音区音阶
                break;
            case 1:  // B段
            case 2:  // C段
                note = HIGH_SCALE[random(7)];  // 使用高音区音阶
                break;
            case 3:  // 尾声
                // 在尾声中交替使用中音区和高音区
                if (random(2) == 0) {
                    note = MID_SCALE[random(7)];
                } else {
                    note = HIGH_SCALE[random(7)];
                }
                break;
        }
    }
    
    lastNote = note;
    lastNoteTime = currentTime;
    
    // 更新小节和拍子计数
    currentBeat++;
    if (currentBeat >= rhythmLength) {
        currentBeat = 0;
        currentMeasure++;
        
        // 如果超过48小节，重新开始
        if (currentMeasure >= 48) {
            currentMeasure = 0;
            currentSection = 0;
        }
    }
    
    // 创建JSON对象
    String newEntry = "{\"n\":" + String(note) + ",\"t\":" + String(duration) + "}";
    
    // 添加到现有JSON字符串
    String resultJson;
    
    if (existingJson.length() == 0 || existingJson == "[]") {
        resultJson = "[" + newEntry + "]";
    } else {
        resultJson = existingJson;
        if (resultJson.endsWith("]")) {
            resultJson = resultJson.substring(0, resultJson.length() - 1);
            if (resultJson.endsWith("}")) {
                resultJson += "," + newEntry + "]";
            } else {
                resultJson += newEntry + "]";
            }
        } else {
            resultJson = "[" + newEntry + "]";
        }
    }
    
    return resultJson;
}