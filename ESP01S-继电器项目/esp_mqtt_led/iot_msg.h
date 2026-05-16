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

// 继电器开关
#pragma pack (1)
struct RelayStatus_LJ{
    UINT8 switchStatus; // 0:关 1：开
};
#pragma pack()


#pragma pack (1)
struct RelayDelayAction_LJ{
    UINT8 status; // 是否启用延迟开关
    UINT32 delaySeconds; // 延迟开关秒数
    UINT64 createTime; // 下发指令时间戳（s）
    UINT8 action; // 0:关灯 1:开灯
};
#pragma pack()