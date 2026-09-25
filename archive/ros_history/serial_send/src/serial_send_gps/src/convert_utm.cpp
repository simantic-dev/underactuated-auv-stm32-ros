#include <ros/ros.h>
#include <mutex>
#include <cmath>

// 假设自定义消息类型为gps_msgs/GPSData
#include <serial_send_gps/gps.h>

// 全局变量
std::mutex data_mutex;
bool is_initialized = false;

// 地球常数
const double EARTH_RADIUS = 6378137.0;  // 单位：米
const double DEG_TO_RAD = M_PI / 180.0;

// 初始位置存储
struct {
    double latitude;
    double longitude;
    double altitude;
} origin;

// 经纬度转平面坐标
void convert_to_relative(double lat, double lon, double alt,
                        double& x, double& y, double& z) 
{
    // 转换为弧度
    double lat_rad = (lat - origin.latitude) * DEG_TO_RAD;
    double lon_rad = (lon - origin.longitude) * DEG_TO_RAD;
    
    // 使用简化投影公式
    x = lat_rad * EARTH_RADIUS;          // 南北方向
    y = lon_rad * EARTH_RADIUS * cos(origin.latitude * DEG_TO_RAD); // 东西方向
    z = alt - origin.altitude;           // 高度差
}

// 回调函数
void gpsCallback(const serial_send_gps::gps::ConstPtr& msg) {
    std::lock_guard<std::mutex> lock(data_mutex);

    // 设置初始原点
        origin.latitude = 35.7875;
        origin.longitude = 120.0273;
        origin.altitude = 1;

    // 计算相对坐标
    double x, y, z;
    convert_to_relative(msg->latitude, msg->longitude, msg->speed, x, y, z);

    // 输出相对坐标
    ROS_INFO("Relative position: X:%.2fm Y:%.2fm", x, y);
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "gps_coordinate_converter");
    ros::NodeHandle nh;

    // 订阅自定义GPS话题
    ros::Subscriber sub = nh.subscribe("/gps_target", 10, gpsCallback);

    // 主循环
    ros::Rate r(10);  // 10Hz
    while (ros::ok()) {
        ros::spinOnce();
        r.sleep();
    }

    return 0;
}