#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "serial/serial.h"
#include "serial_send_gps/usb_gps.h"
#define sBUFFER_SIZE 1024
#define rBUFFER_SIZE 1024
unsigned char s_buffer[sBUFFER_SIZE];
unsigned char r_buffer[rBUFFER_SIZE];
serial::Serial ser;

/*typedef union
{
	float data;
	unsigned char data8[4];
} data_u;
int rec_latitude_deg;
data_u rec_latitude_min;
int rec_longitude_deg;
data_u rec_longitude_min;
data_u rec_heading_angle;
int rec_gps_state_bit;
int rec_RTK_bit;
int rec_latitude_bit;
int rec_longitude_bit;*/

int8_t _ROL[2];
int8_t _PIT[2];
int8_t _YAW[2];
uint8_t _FUSION_STA;

int8_t _ALT_FU[4];
int8_t _ALT_ADD[4];
uint8_t ALT_STA[4];

uint8_t _fixsta;
int8_t _lng[4];
int8_t _lat[4];
int8_t _alt[4];
float _trol, _tpit, _tyaw, _tlng, _tlat, _talt;
int32_t _exrol, _expit, _exyaw, _exlng, _exlat, _exalt;

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
	catch (serial::IOException& e)
	{
		ROS_INFO_STREAM("Failed to open port ");
		return -1;
	}
	ROS_INFO_STREAM("Succeed to open port");

	int cnt = 0;
	ros::Rate r(1);
	ros::Publisher imu_pub = nh.advertise<serial_send_gps::usb_gps>("usb", 1000);
	while (ros::ok())
	{
		serial_send_gps::usb_gps msg;
		if (ser.available())
		{
			size_t bytes_read = ser.read(r_buffer, ser.available());
			int state = 0;
			uint8_t _datalen = 0;
			unsigned char buffer[22] = { 0 };
			for (int i = 0; i < bytes_read && i < rBUFFER_SIZE; i++)
			{
				if (state == 0 && r_buffer[i] == 0xAA)
				{
					state++;
				}
				else if (state == 1 && r_buffer[i] == 0xBB)
				{
					state++;
				}
				else if (state == 2)
				{
					state++;
					switch (r_buffer[i]) {
						case 0x01:
							if (state == 3)
							{
								_datalen = r_buffer[i];
								state++;
							}
							else if (state >= 4 && state < 11)
							{
								buffer[state - 4] = r_buffer[i];
								state++;
							}
							if (state == 11)
							{
								for (int k = 0; k < 2; k++)
								{
									_ROL[k] = buffer[k];
									_PIT[k] = buffer[2 + k];
									_YAW[k] = buffer[4 + k];
								}
								_FUSION_STA = buffer[6];
								for (int j = 1; j >= 0; j--)
								{
									_exrol = _exrol * 10 + _ROL[j];
								}
								_trol = _exrol / 100;
								for (int m = 1; m >= 0; m--)
								{
									_expit = _expit * 10 + _PIT[m];
								}
								_tpit = _expit / 100;
								for (int n = 1; n >= 0; n--)
								{
									_exyaw = _exyaw * 10 + _YAW[n];
								}
								_tyaw = _exyaw / 100;
								ROS_INFO("%d, %.2f, %.2f, %.2f, %d", _FUSION_STA, _trol, _tpit, _tyaw, cnt);
								state = 0;
							}
							break;
						case 0x05:
							break;
						case 0x08:
							break;
						case 0x30:
							if (state == 3)
							{
								_datalen = r_buffer[i];
								state++;
							}
							else if (state >= 4 && state < 27)
							{
								buffer[state - 4] = r_buffer[i];
								state++;
							}
							if (state == 27)
							{
								_fixsta = buffer[0];
								for (int k = 0; k < 4; k++)
								{
									_lng[k] = buffer[2 + k];
									_lat[k] = buffer[6 + k];
									_alt[k] = buffer[10 + k];
								}
								for (int j = 3; j >= 0; j--)
								{
									_exlng = _exlng * 10 + _lng[j];
								}
								_tlng = _exlng / 10000000;
								for (int m = 3; m >= 0; m--)
								{
									_exlat = _exlat * 10 + _lat[m];
								}
								_tlat = _exlat / 10000000;
								for (int n = 3; n >= 0; n--)
								{
									_exalt = _exalt * 10 + _alt[n];
								}
								_talt = _exalt / 10000000;
								ROS_INFO("%d, %.7f, %.7f, %.7f, %d", _fixsta, _tlng, _tlat, _talt, cnt);
								state = 0;
							}
							break;
						case 0x33:
							break;
						default:
							ROS_ERROR("none of data.");
							break;
					}
					return 0;
				}
				
				else state = 0;
			}
		}
		msg.fixsta = _FUSION_STA;
		msg.lng = _trol;
		msg.lat = _tpit;
		msg.alt = _tyaw;

		imu_pub.publish(msg);

		r.sleep();
		cnt++;
	}
	ser.close();
	return 0;
}