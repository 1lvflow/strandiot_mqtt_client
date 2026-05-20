#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "iot_msg.h"
#include <Adafruit_NeoPixel.h>

#define LED_PIN     4     // 对应硬件的 GPIO4 (D2)


// ========== 请修改以下配置 ==========
const char* ssid = "123";        // WiFi名称
const char* password = "123";    // WiFi密码
// 设备id "改我"
#define DEVICE_ID 123

// mqtt 域名配置 "改我"
#define MQTT_HOST "mqtt.strandiot.top"
#define MQTT_PORT 1883

#define MQTT_USERNAME "client_4"
#define MQTT_PASSWORD "123"


// 设备订阅的前缀
#define SUB_TOPIC_PRE "serverMsg/4/"
// 设备上行topic的前缀
#define PUB_TOPIC_PRE "clientMsg/4/"

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
 
WiFiClient espClient;
PubSubClient client(espClient);

// 设置MQTT消息缓冲区大小（需要大于 ColorLedLine_LJ 结构体大小 434 字节 + command 8字节 = 442字节）
const int MQTT_BUFFER_SIZE = 1024;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

static struct ColorLedLine_LJ colorLedLineConf;

void setup() {
  Serial.begin(115200);
  client.setBufferSize(MQTT_BUFFER_SIZE);

  setup_wifi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);

  strip.begin();           // 初始化LED灯带
  strip.show();            // 将所有LED初始化为关闭状态
  strip.setBrightness(50); // 设置亮度为50 (0-255)
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
    String clientId = "ESPCOLORLED-";
    clientId += String(random(0xffff), HEX);
    
    // 带用户名密码的连接
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("已连接");
      // 订阅控制主题
      Serial.print("订阅 topic: ");
      Serial.println(SUB_DEVICE_TOPIC);
      boolean subResult = client.subscribe(SUB_DEVICE_TOPIC);
      if (subResult) {
        Serial.println("订阅成功");
      } else {
        Serial.println("订阅失败");
      }
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
  Serial.print("] 长度: ");
  Serial.println(length);
  
  if (length < 8) {
    return;
  }
  
  memcpy(&net_packet.command, payload, 8);
  net_packet.payload = payload + 8;
  
  switch (net_packet.command) {
    case 18: {
      if (length - 8 < sizeof(colorLedLineConf)) {
        break;
      }
      
      memcpy(&colorLedLineConf, net_packet.payload, sizeof(colorLedLineConf));
      
      if(colorLedLineConf.LightRate > 255) {
        colorLedLineConf.LightRate = 255;
      }
      strip.setBrightness(colorLedLineConf.LightRate);

      for(int i = 0; i < LED_COUNT && i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(colorLedLineConf.Red[i], colorLedLineConf.Green[i], colorLedLineConf.Blue[i]));
      }
      strip.show();
      Serial.println("灯带颜色已更新");
      break;
    }
    case 17:{
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
  
}

void rainbow(uint8_t wait) {
  for(uint16_t firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { 
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}