typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t INT8;
typedef int8_t  INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;
#define    TRUE        1
#define    FALSE        0
typedef void* HANDLE;
typedef uint8_t BOOL;

// 协议定义的数据
struct NetPacket {
    UINT64  command; // 指令类型
    UINT8 * payload; // 指令数据
} net_packet;

#define LED_COUNT   144    // 修改为你灯带的实际灯珠数量


// Led灯带控制
#pragma pack (1)
struct ColorLedLine_LJ{
    UINT16 LightRate; // 亮度
    UINT8 Red[144]; // 所有灯珠 红色 色值 0~255
    UINT8 Green[144]; // 所有灯珠 绿色 色值 0~255
    UINT8 Blue[144]; // 所有灯灯珠 蓝色 色值 0~255
};
#pragma pack()
