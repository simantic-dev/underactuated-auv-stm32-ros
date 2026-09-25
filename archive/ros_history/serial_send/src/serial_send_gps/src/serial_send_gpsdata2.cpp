#include <ros/ros.h>
#include <serial/serial.h>
#include <serial_send_gps/gps.h>
serial::Serial ser;

int16_t floatToInt16(float value, float scale) {
    return static_cast<int16_t>(value * scale);
}
// GPS数据回调函数
void gpsCallback(const serial_send_gps::gps::ConstPtr& msg) {
    // 1. 提取数据
    float ctrl_rol = msg->lat;    // 姿态角-横滚
    float ctrl_pit = msg->lon;    // 姿态角-俯仰
    float ctrl_thr = msg->los;    // 油门控制量
    float ctrl_yawdps = 0.0;      // 自旋角速度（示例值）
    float ctrl_spd_x = 0.0;       // 速度X（示例值）
    float ctrl_spd_y = 0.0;       // 速度Y（示例值）
    float ctrl_spd_z = 0.0;       // 速度Z（示例值）

    int16_t int_rol = floatToInt16(ctrl_rol, 100.0);   // 角度±90，放大100倍
    int16_t int_pit = floatToInt16(ctrl_pit, 100.0);
    int16_t int_thr = floatToInt16(ctrl_thr, 10.0);    // 0-1000(0.1%)
    int16_t int_yawdps = floatToInt16(ctrl_yawdps, 1.0);
    int16_t int_spd_x = static_cast<int16_t>(ctrl_spd_x);
    int16_t int_spd_y = static_cast<int16_t>(ctrl_spd_y);
    int16_t int_spd_z = static_cast<int16_t>(ctrl_spd_z);

    // 2. 构造数据帧
    std::vector<uint8_t> packet;
    packet = { 0xAA, 0xFF, 0x41, 14 };

    // 3. 序列化数据
    auto serializeInt16 = [&packet](int16_t val) {
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&val);
        packet.push_back(bytes[0]);
        packet.push_back(bytes[1]);
        };

    serializeInt16(int_rol);
    serializeInt16(int_pit);
    serializeInt16(int_thr);
    serializeInt16(int_yawdps);
    serializeInt16(int_spd_x);
    serializeInt16(int_spd_y);
    serializeInt16(int_spd_z);

    // 4. 计算校验和（累加和取低8位）
    uint8_t checksum = 0;
    for (uint8_t b : packet) checksum += b;
    packet.push_back(checksum & 0xFF);

    // 5. 发送数据
    ser.write(packet);
    ROS_INFO("Sent control frame: rol=%.2f, pit=%.2f, thr=%.1f", ctrl_rol, ctrl_pit, ctrl_thr);
}


int main(int argc, char** argv) {
    ros::init(argc, argv, "gps_sender");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");
    // 初始化串口
    try {
        ser.setPort("/dev/ttyACM0");  // 替换为实际设备名称
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