#include <ros/ros.h>
#include <serial_send_gps/gps.h>
#include <GeographicLib/UTMUPS.hpp>
#include <geometry_msgs/PoseStamped.h>
#include <tf2/LinearMath/Quaternion.h>
#include <mutex>
#include <cmath>
#include <queue>

// 全局变量
std::mutex data_mutex;                          //数据互斥锁
std::queue<serial_send_gps::gps> target_queue;  // 目标点队列
serial_send_gps::gps current_target;            // 当前目标点
bool has_target = false;
const double ARRIVAL_THRESHOLD = 5.0;           // 到达判定阈值（米）
double latest_los_deg = 0.0;                    // 存储最新的LOS角度

// 地球常数
const double EARTH_RADIUS = 6378137.0;  // 单位：米
const double DEG_TO_RAD = M_PI / 180.0;

// 全局发布者
ros::Publisher cmd_pub;     // 控制指令发布者
ros::Publisher los_pub;     // LOS角发布者（使用自定义消息）

// 订阅目标点回调函数
void targetCallback(const serial_send_gps::gps::ConstPtr& msg) {
    std::lock_guard<std::mutex> lock(data_mutex);
    target_queue.push(*msg);
    ROS_INFO("Target added to queue: lat=%.6f, lon=%.6f", msg->latitude, msg->latitude);
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
    double dx = target_x - current_x;
    double dy = current_y - target_y;
    double math_angle = atan2(dy, dx);
    double nav_angle = (M_PI/2 - math_angle);
    if (nav_angle < 0) nav_angle += 2*M_PI;
    return nav_angle;
}

// 定时发布LOS数据回调函数
void timerCallback(const ros::TimerEvent& event) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    // 创建并发布LOS消息
    serial_send_gps::gps los_msg;
    los_msg.los = latest_los_deg;
    los_pub.publish(los_msg);
    ROS_DEBUG_THROTTLE(1, "Published LOS: %.2f°", latest_los_deg);
}

// 主逻辑处理
void processData(const serial_send_gps::gps::ConstPtr& current_gps) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    // 初始化当前目标
    if (!has_target && !target_queue.empty()) {
        current_target = target_queue.front();
        target_queue.pop();
        has_target = true;
        ROS_INFO("New target set: lat=%.6f, lon=%.6f", 
                current_target.latitude, current_target.latitude);
    }

    if (!has_target) {
        ROS_WARN_THROTTLE(1, "Waiting for target...");
        return;
    }

    // 坐标转换
    double current_x, current_y, target_x, target_y;
    latlonToUTM(current_gps->latitude, current_gps->longitude, current_x, current_y);
    latlonToUTM(current_target.latitude, current_target.longitude, target_x, target_y);

    // 到达检测
    double dx = target_x - current_x;
    double dy = target_y - current_y;
    double distance = sqrt(dx*dx + dy*dy);

    if (distance < ARRIVAL_THRESHOLD) {
        ROS_INFO("Reached target: lat=%.6f, lon=%.6f", 
                current_target.latitude, current_target.longitude);
        
        if (!target_queue.empty()) {
            current_target = target_queue.front();
            target_queue.pop();
            ROS_INFO("Next target: lat=%.6f, lon=%.6f", 
                    current_target.latitude, current_target.longitude);
        } else {
            has_target = false;
            ROS_INFO("All targets completed.");
            return;
        }
    }

    // 计算LOS角
    double los_angle = calculateLOSAngle(current_x, current_y, target_x, target_y);

    // 更新全局LOS角度（不再直接发布）
    latest_los_deg = los_angle * 180/M_PI;
    latest_los_deg = fmod(latest_los_deg, 360.0);
    if (latest_los_deg < 0) latest_los_deg += 360.0;

    // 转换为四元数
    geometry_msgs::PoseStamped cmd_msg;
    tf2::Quaternion q;
    q.setRPY(0, 0, los_angle);
    cmd_msg.pose.orientation.x = q.x();
    cmd_msg.pose.orientation.y = q.y();
    cmd_msg.pose.orientation.z = q.z();
    cmd_msg.pose.orientation.w = q.w();
    cmd_pub.publish(cmd_msg);

    // // 发布控制指令
    // static ros::Publisher cmd_pub = 
    //     ros::NodeHandle().advertise<geometry_msgs::PoseStamped>("/cmd_heading", 10);
    // cmd_pub.publish(cmd_msg);

    // 日志输出
    double los_deg = los_angle * 180/M_PI;
    los_deg = fmod(los_deg, 360.0);
    if (los_deg < 0) los_deg += 360.0;
    
    ROS_INFO_THROTTLE(1, "Target: %.6f,%.6f | LOS: %.2f° | Dist: %.2fm", 
                     current_target.latitude, current_target.longitude, los_deg, distance);
    
    // // 发布到/los话题（使用自定义消息）
    // serial_send_gps::gps los_msg;
    // los_msg.los = los_deg;  // 直接填充los字段
    // los_pub.publish(los_msg);
        
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "los_guidance_node");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");
    
    // 初始化发布者
    cmd_pub = nh.advertise<geometry_msgs::PoseStamped>("/cmd_heading", 1);
    los_pub = nh.advertise<serial_send_gps::gps>("/los", 1);  // 使用自定义消息类型

    // 新增：创建定时器（0.5秒周期）
    ros::Timer timer = nh.createTimer(
        ros::Duration(1), 
        timerCallback
    );

    ros::Subscriber target_sub = nh.subscribe("/target_gps", 1, targetCallback);
    ros::Subscriber current_sub = nh.subscribe("/current_gps", 1, processData);

    ros::spin();
    return 0;
}