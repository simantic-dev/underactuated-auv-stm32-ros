#include <ros/ros.h>
#include <serial/serial.h>
#include <sstream>
#include <iomanip>

int main(int argc, char** argv){
    ros::init(argc, argv, "stm32_sender");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");
    serial::Serial ser;
    try {
        ser.setPort("/dev/ttyUSB0");        // 串口设备
        ser.setBaudrate(115200);            // 波特率
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    } catch (serial::IOException& e) {
        ROS_ERROR("无法打开串口！");
        return -1;
    }

    if (ser.isOpen()) {
        ROS_INFO("串口已开启: %s", ser.getPort().c_str());
    } else {
        ROS_ERROR("串口未开启");
        return -1;
    }

    ros::Rate loop_rate(1);
    while(ros::ok()){
        // 示例数据：0xAA 0xBB 0x01 0x02
        uint8_t data[] = {0xAA, 0xBB, 0x04, 0x02};
        
        size_t bytes_written = ser.write(data, sizeof(data));
        
        // 将数据转换为十六进制字符串
        std::stringstream ss;
        for (size_t i = 0; i < sizeof(data); ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
        }
        
        ROS_INFO("发送 %zu 字节: %s", bytes_written, ss.str().c_str());
        ROS_INFO("尝试发送数据: %02X %02X %02X %02X", data[0], data[1], data[2], data[3]);

        loop_rate.sleep();
    }
    ser.close();
    return 0;
}