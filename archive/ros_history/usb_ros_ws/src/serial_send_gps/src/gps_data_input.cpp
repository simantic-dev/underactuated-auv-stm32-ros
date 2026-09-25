/*
    需求: 循环发布人的信息

*/

#include "ros/ros.h"
#include "serial_send_gps/gps.h"

int main(int argc, char *argv[])
{
    setlocale(LC_ALL,"");

    //1.初始化 ROS 节点
    ros::init(argc,argv,"talker_person");

    //2.创建 ROS 句柄
    ros::NodeHandle nh;

    //3.创建发布者对象
    ros::Publisher pub = nh.advertise<serial_send_gps::gps>("/current_gps",1000);

    //4.组织被发布的消息，编写发布逻辑并发布消息
    serial_send_gps::gps p;
    p.lat = 39.884202;
    p.lon = 116.398674;
    p.speed = 27.114;

    ros::Rate r(1);
    while (ros::ok())
    {
        pub.publish(p);
        p.lat += 0.0005;
        // p.longitude += 3.1415926535897932384626 * 0.00001;
        ROS_INFO("经度:%f,纬度:%f", p.lat, p.lon);

        r.sleep();
        ros::spinOnce();
    }



    return 0;
}