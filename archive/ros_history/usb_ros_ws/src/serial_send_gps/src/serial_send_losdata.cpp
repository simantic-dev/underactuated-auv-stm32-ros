#include <ros/ros.h>
#include <serial/serial.h>
#include <serial_send_gps/gps.h>
#include <fstream>  // 新增文件流头文件

serial::Serial ser;
std::ofstream dataFile;  // 全局文件流对象

// 实时控制帧协议参数
#pragma pack(push, 1) // 内存对齐
struct ControlFrame {
    uint8_t head[2] = {0xAA, 0xFF};
    uint8_t id = 0x41;
    uint8_t len = 14;      // 7个int16 = 14字节
    int16_t ctrl_rol = 0;  // 目标字段
    int16_t ctrl_pit = 0;
    int16_t ctrl_thr = 0;
    int16_t ctrl_yawdps = 0;
    int16_t ctrl_spd_x = 0;
    int16_t ctrl_spd_y = 0;
    int16_t ctrl_spd_z = 0;
    uint8_t sc = 0;        // 和校验
    uint8_t ac = 0;        // 附加校验（根据实际需求实现）
};
#pragma pack(pop)

void gpsCallback(const serial_send_gps::gps::ConstPtr& msg) {

    // 1. 数据记录到文件
    if (dataFile.is_open()) 
    {
        dataFile << ros::Time::now() << ","    // 记录ROS时间戳
                << msg->los << ","            // 记录LOS值
                << std::endl;
    }

    // 2. 初始化控制帧
    ControlFrame frame;
    // 3. 填充los到CTRL_ROL（需要缩放）
    float los = msg->los;
    // 协议要求：±90度放大100倍，这里假设los已经是角度值
    frame.ctrl_rol = static_cast<int16_t>(los * 100); // 转换为±9000
    
    // 4. 计算校验和（仅示例SC校验）
    frame.sc = 0;
    uint8_t* frame_bytes = reinterpret_cast<uint8_t*>(&frame);
    for (int i = 0; i < sizeof(frame) - 2; i++) { // 不包含SC和AC
        frame.sc += frame_bytes[i];
    }
    frame.ac = 0; // 附加校验根据实际协议补充
    
    // 5. 发送数据
    ROS_INFO("Raw frame bytes:");
for (int i = 0; i < sizeof(frame); i++) {
    ROS_INFO("%02X ", frame_bytes[i]);
}
    ser.write(frame_bytes, sizeof(frame));
    ROS_INFO("Sent CTRL_ROL: %.2f -> %d", los, frame.ctrl_rol);
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "ctrl_sender");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");

    // 初始化数据文件
    const std::string filename = "los_data.csv";
    dataFile.open(filename, std::ios::out | std::ios::app);
    if (!dataFile) {
        ROS_ERROR("Failed to create data file: %s", filename.c_str());
        return -1;
    }
    
    // 写入CSV表头
    dataFile << "timestamp,los_value\n";
    
    // 初始化串口（保持原有配置）
    try {
        ser.setPort("/dev/ttyACM0");
        ser.setBaudrate(115200);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    } catch (serial::IOException& e) {
        ROS_ERROR("Port open failed: %s", e.what());
        return -1;
    }

    if (ser.isOpen()) {
        ROS_INFO("Port opened");
    }

    // 订阅话题（假设仍然使用/los话题）
    ros::Subscriber sub = nh.subscribe("/los", 1, gpsCallback);

    // 添加定期flush机制
    ros::Rate rate(1);  // 每秒flush一次
    while (ros::ok()) 
    {
        ros::spinOnce();
        dataFile.flush();  // 定期刷新缓冲区
        rate.sleep();
    }

    // 关闭资源
    dataFile.close();
    if (ser.isOpen()) ser.close();
    return 0;
}