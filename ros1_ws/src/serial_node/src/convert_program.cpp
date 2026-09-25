#include <ros/ros.h>
#include <serial_node/gps_data.h>  // 替换为自己的包名

ros::Publisher pub;

void convertCallback(const serial_node::gps_data::ConstPtr& msg) {
    // 转换公式：十进制 = 度 + 分/60
    serial_node::gps_data conver;
    conver.latitude = msg->rec_latitude_deg + (msg->rec_latitude_min / 60.0);
    conver.longitude = msg->rec_longitude_deg + (msg->rec_longitude_min / 60.0);

    pub.publish(conver);
    ROS_INFO("经度：%f, 纬度：%f", conver.latitude, conver.longitude);

}

int main(int argc, char** argv) {
    ros::init(argc, argv, "longitude_converter_cpp");
    ros::NodeHandle nh;
    setlocale(LC_ALL,"");
    // 创建发布/订阅器
    // pub = nh.advertise<std_msgs::Float32>("converted_lon", 10);
    pub = nh.advertise<serial_node::gps_data>("gps/fix", 1000);
    ros::Subscriber sub = nh.subscribe("gps", 10, convertCallback);
    ROS_INFO("C++经度转换节点已启动");
    
    ros::spin();
    return 0;
}