#include <ros/ros.h>
#include <serial_send_gps/gps.h>
#include <GeographicLib/UTMUPS.hpp>
#include <geometry_msgs/PoseStamped.h>
#include <tf2/LinearMath/Quaternion.h>
#include <mutex>
#include <cmath>

// 全局变量
std::mutex target_mutex;
serial_send_gps::gps target_gps;  // 目标点经纬度
bool has_target = false;            // 是否接收到目标点

// 地球常数
const double EARTH_RADIUS = 6378137.0;  // 单位：米
const double DEG_TO_RAD = M_PI / 180.0;

// 订阅目标点回调函数
void targetCallback(const serial_send_gps::gps::ConstPtr& msg) {
    std::lock_guard<std::mutex> lock(target_mutex);
    target_gps = *msg;
    has_target = true;
    ROS_INFO("Target set: lat=%.6f, lon=%.6f", msg->latitude, msg->latitude);
}

// 经纬度转UTM坐标
void latlonToUTM(double lat1, double lon1, double& x, double& y) {
    int zone;
    bool northp;
    GeographicLib::UTMUPS::Forward(lat1, lon1, zone, northp, x, y);
}

// 计算LOS角（弧度）
double calculateLOSAngle(double current_x, double current_y, 
                        double target_x, double target_y) {
        // 东向增量：目标东坐标 - 当前东坐标
        double dx = target_x - current_x;
        // 北向增量：当前北坐标 - 目标北坐标（修正方向）
        double dy = current_y - target_y;  // 注意此处调整为 current_y - target_y
        // 数学坐标系角度（正东为0°，逆时针）
         double math_angle = atan2(dy, dx);
    
        // 转换为导航坐标系（正北为0°，顺时针）
        double nav_angle = (M_PI/2 + math_angle);
        if (nav_angle < 0) nav_angle += 2*M_PI;
    
        return nav_angle;
}

// 主逻辑处理
void processData(const serial_send_gps::gps::ConstPtr& current_gps) {
    std::lock_guard<std::mutex> lock(target_mutex);
    if (!has_target) {
        ROS_WARN_THROTTLE(1, "Waiting for target position...");
        return;
    }

    // 转换实时位置和目标位置到UTM
    double current_x, current_y, target_x, target_y;
    latlonToUTM(current_gps->latitude, current_gps->latitude, current_x, current_y);
    latlonToUTM(target_gps.latitude, target_gps.latitude, target_x, target_y);

    // 计算LOS角
    double los_angle = calculateLOSAngle(current_x, current_y, target_x, target_y);

    // 计算航向误差（假设当前航向为0，需根据实际传感器数据调整）
    double heading_error = los_angle;  // 实际应用中需替换为当前航向
    
    // 转换为度数并归一化到0~360°
    double los_angle_deg = los_angle * 180.0 / M_PI;
    los_angle_deg = fmod(los_angle_deg, 360.0);
    if (los_angle_deg < 0) los_angle_deg += 360.0;
    if (std::abs(los_angle_deg - 360.0) < 1e-6) {
        los_angle_deg = 0.0;
    }
    
    // 发布控制指令（示例：发布期望航向角）
    geometry_msgs::PoseStamped cmd_msg;
    tf2::Quaternion q;
    q.setRPY(0, 0, los_angle);  // 直接使用修正后的角度
    cmd_msg.pose.orientation.x = q.x();
    cmd_msg.pose.orientation.y = q.y();
    cmd_msg.pose.orientation.z = q.z();
    cmd_msg.pose.orientation.w = q.w();

    static ros::Publisher cmd_pub = 
        ros::NodeHandle().advertise<geometry_msgs::PoseStamped>("/cmd_heading", 10);
    cmd_pub.publish(cmd_msg);

    ROS_INFO_THROTTLE(1, "LOS angle: %.2f°, Error: %.2f°", 
                     los_angle * 180/M_PI, heading_error * 180/M_PI);
    // ROS_INFO("lat:%f", current_gps->lat);
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "los_guidance_node");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");

    // 订阅目标点和实时位置
    ros::Subscriber target_sub = nh.subscribe("/target_gps", 1, targetCallback);
    ros::Subscriber current_sub = nh.subscribe("/current_gps", 1, processData);

    ros::spin();
    return 0;
}