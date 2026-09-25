#include <ros/ros.h>
#include <nav_msgs/Path.h>
#include <geometry_msgs/PoseStamped.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

// 地球常数
const double EARTH_RADIUS = 6378137.0;  // 单位：米
const double DEG_TO_RAD = M_PI / 180.0;

// 初始位置存储
struct {
    double latitude;
    double longitude;
    double altitude;
    bool initialized = false;
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

int main(int argc, char** argv) {
    ros::init(argc, argv, "gps_path_publisher");
    ros::NodeHandle nh;
    ros::Publisher path_pub = nh.advertise<nav_msgs::Path>("gps_path", 10);
    nav_msgs::Path path_msg;
    path_msg.header.frame_id = "map";

    // 读取GPS数据文件
    std::ifstream file("gps_data.txt");
    if (!file.is_open()) {
        ROS_ERROR("Failed to open GPS data file.");
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 跳过标题行
        if (line.find("timestamp") != std::string::npos) continue;

        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, ',')) fields.push_back(field);

        if (fields.size() < 5) continue;

        double fixsta = std::stod(fields[1]);
        if (fixsta < 0.5) continue;  // 仅处理有效定位数据

        double lat = std::stod(fields[2]);
        double lon = std::stod(fields[3]);
        double alt = std::stod(fields[4]);

        // 设置初始原点
        if (!origin.initialized) {
            origin.latitude = lat;
            origin.longitude = lon;
            origin.altitude = alt;
            origin.initialized = true;
            ROS_INFO("Origin set at: lat=%.6f, lon=%.6f", lat, lon);
            continue;
        }

        // 坐标转换
        double x, y, z;
        convert_to_relative(lat, lon, alt, x, y, z);

        geometry_msgs::PoseStamped pose;
        pose.header.frame_id = "map";
        pose.header.stamp = ros::Time::now();
        pose.pose.position.x = x;
        pose.pose.position.y = y;
        pose.pose.position.z = z;
        pose.pose.orientation.w = 1.0;

        path_msg.poses.push_back(pose);
    }
    file.close();

    ros::Rate rate(10);
    while (ros::ok()) {
        path_msg.header.stamp = ros::Time::now();
        path_pub.publish(path_msg);
        rate.sleep();
        ros::spinOnce();
    }

    return 0;
}
