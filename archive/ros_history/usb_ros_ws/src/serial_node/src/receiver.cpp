#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "serial/serial.h"
#include "serial_node/test.h"
#define sBUFFER_SIZE 1024
#define rBUFFER_SIZE 1024
unsigned char s_buffer[sBUFFER_SIZE];
unsigned char r_buffer[rBUFFER_SIZE];
serial::Serial ser;
typedef union
{
	float data;
	unsigned char data8[4];
} data_u;
data_u position_x;
data_u position_y;
data_u position_z;
data_u roll;
data_u pitch;
data_u yaw;
data_u status_bit;
int main(int argc, char** argv)
{
	ros::init(argc, argv, "serial_node");
	ros::NodeHandle nh;
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
		ROS_INFO_STREAM("Failed to open port ");
		return -1;
	}
	ROS_INFO_STREAM("Succeed to open port" );

	int cnt = 0;
	ros::Rate loop_rate(500);
	ros::Publisher imu_pub = nh.advertise<serial_node::test>("gps", 1000);
	while(ros::ok())
	{
		serial_node::test msg;
		if(ser.available())
		{
			size_t bytes_read = ser.read(r_buffer, ser.available());
			int state = 0;
			unsigned char buffer[28] = {0};
			for(int i = 0; i < bytes_read && i < rBUFFER_SIZE; i++)
			{
				if(state == 0 && r_buffer[i] == 0xAA)
				{
					state++;
				}
				else if(state == 1 && r_buffer[i] == 0xBB)
				{
					state++;
				}
				else if(state >= 2 && state < 30)
				{
					buffer[state-2]=r_buffer[i];
					state ++;
					if(state == 30)
					{
						for(int k = 0; k < 4; k++)
						{
							position_x.data8[k] = buffer[k];
							position_y.data8[k] = buffer[4 + k];
							position_z.data8[k] = buffer[8 + k];
							roll.data8[k] = buffer[12 + k];
							pitch.data8[k] = buffer[16 + k];
							yaw.data8[k] = buffer[20 + k];
							status_bit.data8[k] = buffer[24 + k];
						}
						ROS_INFO("%f, %f, %f, %f, %f, %f, %f, %d", position_x.data, position_y.data, position_z.data, roll.data, pitch.data, yaw.data, status_bit.data, cnt);
						state = 0;
					}
				}
				else state = 0;
			}
		}
		msg.position_x = position_x.data;
		msg.position_y = position_y.data;
		msg.position_z = position_z.data;
		msg.roll = roll.data;
		msg.pitch = pitch.data;
		msg.yaw = yaw.data;
		msg.status_bit = status_bit.data;
		imu_pub.publish(msg);
		loop_rate.sleep();
		cnt++;
	}
	ser.close();
	return 0;
}
