#include <iostream>
#include <string>
#include <sstream>
using namespace std;

#include "ros/ros.h"
#include "std_msgs/String.h"
#include "serial/serial.h"
#include "serial_node/gps.h"

#define sBUFFER_SIZE 1024
#define rBUFFER_SIZE 1024
unsigned char s_buffer[sBUFFER_SIZE];
unsigned char r_buffer[rBUFFER_SIZE];

serial::Serial ser;
typedef union 
{
    float data;
    unsigned char data8[4];
}
data_u;
data_u latitude;
data_u longitude;
data_u angle;

int main(int argc,char** argv)
{
    ros::init(argc,argv,"serial_node");
    ros::NodeHandle nh;
    //打开串口
    try
    {
        ser.setPort("/dev/ttyUSB0");
        ser.setBaudrate(115200);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    }
    catch(serial::IOException &e)
    {
        ROS_INFO_STREAM("Failed to open port");
        return -1;
    }
    ROS_INFO_STREAM("Succeed to open port");

    int cnt = 0;
    ros::Rate r(1);
    ros::Publisher imu_pub = nh.advertise<serial_node::gps>("imu",1000);
    while (ros::ok())
    {
        serial_node::gps msg;
        if (ser.available())
        {
            //读取串口数据
            size_t bytes_read = ser.read(r_buffer,ser.available());
            //使用状态机处理读取到的数据，通信协议与STM32端一致
            int state = 0;
            unsigned char buffer[12] = {0};
            for(int i = 0; i < bytes_read && i < rBUFFER_SIZE; i++)
            {
                if (state == 0 && r_buffer[i] == 0xAA)
                {
                    state++;
                }
                else if (state == 1 && r_buffer[i] == 0xBB)
                {
                    state++;
                }
                else if (state >= 2 && state < 18)
                {
                    buffer[state-2]=r_buffer[i];
                    state++;
                    if (state == 18)
                    {
                        for (int k = 0; k < 4; k++)
                        {
                            latitude.data8[k] = buffer[k];
                            longitude.data8[k] = buffer[4+k];
                            angle.data8[k] = buffer[8+k];
                        }
                        ROS_INFO("%f,%f,%f,%d",latitude.data,longitude.data,angle.data,cnt);
                        state = 0;
                    }
                }
                else state = 0;
            }
        }
        //发布接受到的imu数据
        msg.latitude = latitude.data;
        msg.longitude = longitude.data;
        msg.angle = angle.data;
        imu_pub.publish(msg);
        ros::spinOnce();
        r.sleep();
        cnt++;
    }
    //关闭串口
    ser.close();
    return 0;
}


