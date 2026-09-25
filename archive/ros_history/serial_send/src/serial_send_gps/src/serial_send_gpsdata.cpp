#include <ros/ros.h>
#include <serial/serial.h>
#include <serial_send_gps/gps.h>
serial::Serial ser;

// GPS数据回调函数
void gpsCallback(const serial_send_gps::gps::ConstPtr& msg) {
    // 1. 提取数据
    float lat = msg->latitude;
    float lon = msg->longitude;
    float los = msg->los;
    uint32_t timestamp = msg->timestamp;

    // 2. 构造数据帧
    uint8_t header[] = {0xAA, 0x55};
    uint8_t length = 4 * 4;  // 4个float/uint32_t，每个4字节
    uint8_t checksum = 0;

    std::vector<uint8_t> packet;
    packet.insert(packet.end(), header, header + 2);
    packet.push_back(length);

    // 3. 序列化数据
    auto serializeFloat = [](float f, std::vector<uint8_t>& buf) {
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&f);
        for (int i = 0; i < 4; i++) buf.push_back(bytes[i]);
    };

    serializeFloat(lat, packet);
    serializeFloat(lon, packet);
    serializeFloat(los, packet);
    uint8_t* time_bytes = reinterpret_cast<uint8_t*>(&timestamp);
    packet.insert(packet.end(), time_bytes, time_bytes + 4);

    // 4. 计算校验和（累加和取低8位）
    for (auto b : packet) checksum += b;
    packet.push_back(checksum & 0xFF);

    // 5. 发送数据
    ser.write(packet);
    ROS_INFO("Sent GPS data: lat=%.6f, lon=%.6f, los=%.2f, timestamp=%u", lat, lon, los, timestamp);
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "gps_sender");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");
    // 初始化串口
    try {
        ser.setPort("/dev/ttyUSB0");  // 替换为实际设备名称
        ser.setBaudrate(115200);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    } catch (serial::IOException& e) {
        ROS_ERROR("无法打开串口！");
        return -1;
    }

    if (ser.isOpen()) {
        ROS_INFO("串口已开启");
    }

    // 订阅GPS话题
    ros::Subscriber sub = nh.subscribe("/los", 1, gpsCallback);
    ros::spin();
    return 0;
}