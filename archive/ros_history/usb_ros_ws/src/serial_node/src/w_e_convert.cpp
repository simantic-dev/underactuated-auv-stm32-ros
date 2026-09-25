/*
    需求: 循环发布人的信息

*/

#include "ros/ros.h"
#include "serial_node/gps_data.h"

int main(int argc, char *argv[])
{
    setlocale(LC_ALL,"");

    //1.初始化 ROS 节点
    ros::init(argc,argv,"talker_person");

    //2.创建 ROS 句柄
    ros::NodeHandle nh;

    //3.创建发布者对象
    ros::Publisher pub = nh.advertise<serial_node::gps_data>("gps",1000);

    //4.组织被发布的消息，编写发布逻辑并发布消息
    serial_node::gps_data p;
    p.rec_latitude_deg = 35.0;
    p.rec_latitude_min = 12.5;
    p.rec_longitude_deg = 120.0;
    p.rec_longitude_min = 11.5;

    ros::Rate r(1);
    while (ros::ok())
    {
        pub.publish(p);
        p.rec_latitude_deg += 0.00001;
        p.rec_latitude_min += 0.0001;
        p.rec_longitude_deg += 0.00005;
        p.rec_longitude_min -= 0.0001;

        ROS_INFO("经度:%f,经分:%f", p.rec_latitude_deg, p.rec_latitude_min);
        ROS_INFO("纬度:%f,纬分:%f", p.rec_longitude_deg, p.rec_longitude_min);
        r.sleep();
        ros::spinOnce();
    }



    return 0;
}