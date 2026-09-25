#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "serial/serial.h"
#include "serial_node/gps_data.h"
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
unsigned char rec_latitude_deg;
data_u rec_latitude_min;
unsigned char rec_longitude_deg;
data_u rec_longitude_min;
data_u rec_heading_angle;
unsigned char rec_gps_state_bit;
unsigned char rec_RTK_bit;
unsigned char rec_latitude_bit;
unsigned char rec_longitude_bit;
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
	ros::Rate r(1);
	ros::Publisher imu_pub = nh.advertise<serial_node::gps_data>("gps", 1000);
	while(ros::ok())
	{
		serial_node::gps_data msg;
		if(ser.available())
		{
			size_t bytes_read = ser.read(r_buffer, ser.available());
			int state = 0;
			unsigned char buffer[18] = {0};
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
				else if(state >= 2 && state < 20)
				{
					buffer[state-2]=r_buffer[i];
					state ++;
					if(state == 20)
					{
						rec_gps_state_bit = buffer[0];
						rec_RTK_bit = buffer[1];
						rec_latitude_bit = buffer[2];
						rec_latitude_deg = buffer[3];
						rec_longitude_bit = buffer[8];
						rec_longitude_deg = buffer[9];
						for(int k = 0; k < 4; k++)
						{
							rec_latitude_min.data8[k] = buffer[4 + k];
							rec_longitude_min.data8[k] = buffer[10 + k];
							rec_heading_angle.data8[k] = buffer[14 + k];
						}
						ROS_INFO("%d, %d, %d, %d, %f, %d, %d, %f, %f, %d", rec_gps_state_bit, rec_RTK_bit, rec_latitude_bit, rec_latitude_deg, rec_latitude_min.data, rec_longitude_bit, rec_longitude_deg, rec_longitude_min.data, rec_heading_angle.data, cnt);
						state = 0;
					}
				}
				else state = 0;
			}
		}

		msg.rec_gps_state_bit = rec_gps_state_bit;
		msg.rec_RTK_bit = rec_RTK_bit;
		msg.rec_latitude_bit = rec_latitude_bit;
		msg.rec_latitude_deg = rec_latitude_deg;
		msg.rec_latitude_min = rec_latitude_min.data;
		msg.rec_longitude_bit = rec_longitude_bit;
		msg.rec_longitude_deg = rec_longitude_deg;
		msg.rec_longitude_min = rec_longitude_min.data;
		msg.rec_heading_angle = rec_heading_angle.data;
		imu_pub.publish(msg);

		r.sleep();
		cnt++;
	}
	ser.close();
	return 0;
}
