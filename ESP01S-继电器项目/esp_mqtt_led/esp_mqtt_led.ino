#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "iot_msg.h"

// ========== 请修改以下配置 ==========
const char* ssid = "123";        // WiFi名称
const char* password = "123";    // WiFi密码
// 设备id "改我"
#define DEVICE_ID 123

// mqtt 域名配置 "改我"
#define MQTT_HOST "mqtt.strandiot.top"
#define MQTT_PORT 1883

#define MQTT_USERNAME "client_123"
#define MQTT_PASSWORD "123"


// 设备订阅的前缀
#define SUB_TOPIC_PRE "serverMsg/123/"
// 设备上行topic的前缀
#define PUB_TOPIC_PRE "clientMsg/123/"

// 使用字符串化拼接
#define STRINGIFY(x) #x
#define EXPAND_STRINGIFY(x) STRINGIFY(x)

// 需要接受，发送的完整topic
#define SUB_DEVICE_TOPIC SUB_TOPIC_PRE EXPAND_STRINGIFY(DEVICE_ID)
#define PUB_DEVICE_TOPIC PUB_TOPIC_PRE EXPAND_STRINGIFY(DEVICE_ID)

// 主题定义
const char* light_command_topic = SUB_DEVICE_TOPIC;   // 接收控制命令
const char* light_state_topic = PUB_DEVICE_TOPIC;      // 上报状态



// ==================================

#define RELAY_PIN 0    // ESP-01S 继电器模块固定用 GPIO0 控制

WiFiClient espClient;
PubSubClient client(espClient);

// 当前继电器状态（HIGH=断开，LOW=吸合）
bool relayState = HIGH;
// 延迟开关参数
struct RelayDelayAction_LJ delayAction;
// 延迟开关状态标记
bool delayActionPending = false;
unsigned long delayStartTime = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // 初始断开继电器
  
  setup_wifi();
  
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("正在连接 WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi 已连接");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("正在连接 MQTT 服务器...");
    
    // 生成随机客户端ID
    String clientId = "ESP01S-";
    clientId += String(random(0xffff), HEX);
    
    // 带用户名密码的连接
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("已连接");
      // 订阅控制主题
      client.subscribe(light_command_topic);
      // 上报当前状态
      publishState();
    } else {
      Serial.print("连接失败, rc=");
      Serial.print(client.state());
      Serial.println(" 5秒后重试...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("收到消息 [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // 把 payload 转成字符串
  memcpy(&net_packet.command, payload, 8);
    // Set payload to point to data starting from byte 8
    net_packet.payload = payload + 8;
    switch (net_packet.command) {
    case 16: {
            uint8_t switchStatus ;
            memcpy(&switchStatus, net_packet.payload, 1);
            if(switchStatus == 1){
                digitalWrite(RELAY_PIN, LOW);   // 继电器吸合 → 灯亮
                relayState = LOW;
                Serial.println("灯已打开");
                publishState();
            }else if (switchStatus == 0){
                digitalWrite(RELAY_PIN, HIGH);  // 继电器断开 → 灯灭
                relayState = HIGH;
                Serial.println("灯已关闭");
                publishState();
            }
            break;
    }
    case 17:{
       memcpy(&delayAction, net_packet.payload, sizeof(delayAction));

       Serial.printf("延迟开关 status=%d,delaySec=%d,timeSec=%d \n", delayAction.status, delayAction.delaySeconds, delayAction.createTime);
       if (delayAction.status == 1) {
         delayActionPending = true;
         delayStartTime = millis();
         Serial.println("延迟开关已启动");
       } else {
         delayActionPending = false;
         Serial.println("延迟开关已取消");
       }
       iot_protocol_send_data(17, (uint8_t*)&delayAction, sizeof(delayAction));
       break;
    }
    default:
        break;
    }
   
}
// ========== 新增：协议发送函数 ==========
void iot_protocol_send_data(uint64_t command, uint8_t* payload, uint16_t length) {
    // 定义发送缓冲区（根据你的实际环境调整大小）
    static uint8_t send_data[256];
    
    // 1. 清空数组
    memset(send_data, 0, sizeof(send_data));
    
    // 2. 复制 command（8字节）到数组
    uint16_t offset = 0;
    memcpy(send_data + offset, &command, 8);
    offset += 8;
    
    // 3. 将 payload 内容追加到数组后
    if (payload != NULL && length > 0) {
        memcpy(send_data + offset, payload, length);
        offset += length;
    }
    
    // 4. 追加 \r\n（可选，根据协议需求）
    send_data[offset++] = '\r';
    send_data[offset++] = '\n';
    
    printf("send topic=%s, data length=%d\n", PUB_DEVICE_TOPIC, offset);
    
    // 5. 发送数据（注意：需要根据你的 MQTT 客户端调整）
    // 方案A：如果你的 iot_net_send 是 MQTT publish 的封装
    // iot_net_send((uint8_t*)PUB_DEVICE_TOPIC, send_data, offset);
    
    // 方案B：如果直接用 PubSubClient 库（取消注释下面这行）
    client.publish(PUB_DEVICE_TOPIC, send_data, offset);
}

// ========== 修改：上报状态的函数 ==========
void publishState() {
    uint8_t switchStatus;  // 1字节：0=关，1=开
    
    if (relayState == LOW) {
        switchStatus = 1;   // 继电器吸合 = 灯亮
    } else {
        switchStatus = 0;   // 继电器断开 = 灯关
    }
    
    // 准备要发送的数据：
    // 格式：[8字节command] + [1字节switchStatus]
    uint8_t payload = switchStatus;          // 1字节状态
    
    // 调用协议发送函数
    iot_protocol_send_data(16, &payload, 1);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (delayActionPending) {
    unsigned long elapsedTime = (millis() - delayStartTime) / 1000;
    if (elapsedTime >= delayAction.delaySeconds) {
      delayActionPending = false;
      if (delayAction.action == 1) {
        digitalWrite(RELAY_PIN, LOW);
        relayState = LOW;
        Serial.println("延迟时间到，执行开灯动作");
      } else {
        digitalWrite(RELAY_PIN, HIGH);
        relayState = HIGH;
        Serial.println("延迟时间到，执行关灯动作");
      }
      publishState();
    }
  }
}