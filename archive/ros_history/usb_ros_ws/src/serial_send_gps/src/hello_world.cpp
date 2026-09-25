/*
    需求: 循环发布人的信息，形成圆形轨迹
*/

#include "ros/ros.h"
#include "serial_send_gps/gps.h"
#include <cmath>  // 添加数学库支持

int main(int argc, char *argv[])
{
    setlocale(LC_ALL,"");

    // 1. 初始化 ROS 节点
    ros::init(argc,argv,"talker_person");

    // 2. 创建 ROS 句柄
    ros::NodeHandle nh;

    // 3. 创建发布者对象
    ros::Publisher pub = nh.advertise<serial_send_gps::gps>("/current_gps", 1000);

    // 4. 定义圆形轨迹参数
    const double lat_center = 39.884202;  // 圆心纬度
    const double lon_center = 116.398674; // 圆心经度
    const double radius = 0.0005;         // 半径（单位：度，约55米）
    double theta = 0.0;                   // 初始角度（弧度）
    const double angular_speed = 0.1;     // 角速度（弧度/周期）

    serial_send_gps::gps p;
    p.speed = 27.114;  // 速度字段可根据需要调整

    ros::Rate r(10);  // 提高频率至10Hz，使轨迹更平滑
    while (ros::ok())
    {
        // 计算新坐标（平面近似）
        p.lat = lat_center + radius * cos(theta);
        p.lon = lon_center + radius * sin(theta);
        
        // 更新角度
        theta += angular_speed;
        if (theta > 2 * M_PI) theta -= 2 * M_PI;  // 保持角度在0~2π范围内

        // 发布消息
        pub.publish(p);
        ROS_INFO("经度:%.6f, 纬度:%.6f", p.lon, p.lat);  // 格式化输出更高精度

        r.sleep();
        ros::spinOnce();
    }

    return 0;
}

// /*
//     需求: 循环发布人的信息

// */

// #include "ros/ros.h"
// #include "serial_node/gps_data.h"

// int main(int argc, char *argv[])
// {
//     setlocale(LC_ALL,"");

//     //1.初始化 ROS 节点
//     ros::init(argc,argv,"talker_person");

//     //2.创建 ROS 句柄
//     ros::NodeHandle nh;

//     //3.创建发布者对象
//     ros::Publisher pub = nh.advertise<serial_node::gps_data>("/gps/fix",1000);

//     //4.组织被发布的消息，编写发布逻辑并发布消息
//     serial_node::gps_data p;
//     p.latitude = 39.884202;
//     p.longitude = 116.398674;
//     p.speed = 27.114;

//     ros::Rate r(1);
//     while (ros::ok())
//     {
//         pub.publish(p);
//         p.latitude += 0.0005;
//         // p.longitude += 3.1415926535897932384626 * 0.00001;
//         ROS_INFO("纬度:%f,经度:%f", p.latitude, p.longitude);

//         r.sleep();
//         ros::spinOnce();
//     }



//     return 0;
// }