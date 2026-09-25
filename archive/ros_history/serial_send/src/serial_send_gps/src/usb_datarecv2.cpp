#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "serial/serial.h"
#include "serial_send_gps/gps.h"
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

int8_t _ROL[2] = { 0 };
int8_t _PIT[2] = { 0 };
int8_t _YAW[2] = { 0 };
uint8_t _FUSION_STA = 0;

int8_t _ALT_FU[4] = { 0 };
int8_t _ALT_ADD[4] = { 0 };
uint8_t _ALT_STA = 0;

int8_t _POS_X[4] = { 0 };
int8_t _POS_Y[4] = { 0 };

uint8_t _fixsta = 0;
int8_t _lng[4] = { 0 };
int8_t _lat[4] = { 0 };
int8_t _alt[4] = { 0 };

int8_t _SPED_X[2] = { 0 };
int8_t _SPED_Y[2] = { 0 };
int8_t _SPED_Z[2] = { 0 };

float _trol, _tpit, _tyaw, _taltfu, _taltadd, _tspedx, _tspedy, _tspedz, _tlng, _tlat, _talt = 0;
int32_t _exrol, _expit, _exyaw, _exlng, _exlat, _exalt, _tposx, _tposy = 0;

int main(int argc, char** argv)
{
	ros::init(argc, argv, "serial_node");
	ros::NodeHandle nh;
	try
	{
		ser.setPort("/dev/ttyACM0");
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
	catch (const serial::SerialException& e) 
	{  
		ROS_ERROR_STREAM("Communication error: " << e.what());  	// 新增：读写过程中断
		ser.close();
		return -1;
	}
	ROS_INFO_STREAM("Succeed to open port");

	int cnt = 0;
	ros::Rate r(1);
	ros::Publisher imu_pub = nh.advertise<serial_send_gps::gps>("current_gps", 1000);
	while (ros::ok())
	{
		serial_send_gps::gps msg;
		try {
			if (!ser.isOpen()) 
			{
				ser.open();
				ROS_INFO("Reconnected to port");
			}
			if (ser.available())
			{
				// 修改为（限制单次读取长度）：
				size_t bytes_available = ser.available();
				size_t bytes_to_read = std::min(bytes_available, (size_t)rBUFFER_SIZE);			// 安全读取
				size_t bytes_read = ser.read(r_buffer, bytes_to_read);  
				
				int state = 0;
				int euagl_sta = 0;
				int alt_sta = 0;
				int poserr_sta = 0;
				int gps_sta = 0;
				int dvl_sta = 0;
				uint8_t data_len = 0;
				unsigned char buffer[23] = { 0 };
				for (int i = 0; i < bytes_read && i < rBUFFER_SIZE; i++)
				{
					if (state == 0 && r_buffer[i] == 0xAA)
					{
						state++;
					}
					else if (state == 1 && r_buffer[i] == 0xFF)
					{
						state++;
					}

					else if (state == 2 && r_buffer[i] == 0x03)
					{
						euagl_sta++;
						state++;
					}

					else if (state == 2 && r_buffer[i] == 0x05)
					{
						alt_sta++;
						state++;
					}

					else if (state == 2 && r_buffer[i] == 0x08)
					{
						poserr_sta++;
						state++;
					}

					else if (state == 2 && r_buffer[i] == 0x30)
					{
						gps_sta++;
						state++;
					}

					else if (state == 2 && r_buffer[i] == 0x33)
					{
						dvl_sta++;
						state++;
					}

					else if (state == 3)
					{
						data_len = r_buffer[i];
						state++;
					}

					else if (state == 4) 
					{
						if (euagl_sta >= 1 && euagl_sta < 8)
						{
							buffer[euagl_sta - 1] = r_buffer[i];
							euagl_sta++;
							if (euagl_sta == 8)
							{
								for (int k = 0; k < 2; k++)
								{
									_ROL[k] = buffer[k];
									_PIT[k] = buffer[2 + k];
									_YAW[k] = buffer[4 + k];
								}
								_FUSION_STA = buffer[6];
								for (int j = 0; j <= 1; j++)
								{
									_exrol = _exrol * 10 + _ROL[j];
								}
								_trol = _exrol / 100;
								for (int m = 0; m <= 1; m++)
								{
									_expit = _expit * 10 + _PIT[m];
								}
								_tpit = _expit / 100;
								for (int n = 0; n <= 1; n++)
								{
									_exyaw = _exyaw * 10 + _YAW[n];
								}
								_tyaw = _exyaw / 100;
								ROS_INFO("%d, %.2f, %.2f, %.2f, %d", _FUSION_STA, _trol, _tpit, _tyaw, data_len);
								_exrol = 0;
								_expit = 0;
								_exyaw = 0;
								_trol = 0;
								_tpit = 0;
								_tyaw = 0;
								euagl_sta = 0;
								state = 0;
							}
						}

						else if (alt_sta >= 1 && alt_sta < 10)
						{
							buffer[alt_sta - 1] = r_buffer[i];
							alt_sta++;
							if (alt_sta == 10)
							{
								for (int k = 0; k < 4; k++)
								{
									_ALT_FU[k] = buffer[k];
									_ALT_ADD[k] = buffer[4 + k];
								}
								_ALT_STA = buffer[8];
								for (int j = 0; j <= 3; j++)
								{
									_taltfu = _taltfu * 10 + _ALT_FU[j];
								}
								for (int m = 0; m <= 3; m++)
								{
									_taltadd = _taltadd * 10 + _ALT_ADD[m];
								}
								ROS_INFO("%d, %.2f, %.2f, %d", _ALT_STA, _taltfu, _taltadd, data_len);
								_taltfu = 0;
								_taltadd = 0;
								alt_sta = 0;
								state = 0;
							}
						}

						else if (poserr_sta >= 1 && poserr_sta < 9)
						{
							buffer[poserr_sta - 1] = r_buffer[i];
							poserr_sta++;
							if (poserr_sta == 9)
							{
								for (int k = 0; k < 4; k++)
								{
									_POS_X[k] = buffer[k];
									_POS_Y[k] = buffer[4 + k];
								}
								for (int j = 0; j <= 3; j++)
								{
									_tposx = _tposx * 10 + _POS_X[j];
								}
								for (int m = 0; m <= 3; m++)
								{
									_tposy = _tposy * 10 + _POS_Y[m];
								}
								ROS_INFO("%d, %d, %d", _tposx, _tposy, data_len);
								_tposx = 0;
								_tposy = 0;
								poserr_sta = 0;
								state = 0;
							}
						}

						else if (gps_sta >= 1 && gps_sta < 24)
						{
							buffer[gps_sta - 1] = r_buffer[i];
							gps_sta++;
							if (gps_sta == 24)
							{
								_fixsta = buffer[0];
								for (int k = 0; k < 4; k++)
								{
									_lng[k] = buffer[2 + k];
									_lat[k] = buffer[6 + k];
									_alt[k] = buffer[10 + k];
								}
								for (int j = 0; j <= 3; j++)
								{
									_exlng = _exlng * 10 + _lng[j];
								}
								_tlng = _exlng / 10000000;
								for (int m = 0; m <= 3; m++)
								{
									_exlat = _exlat * 10 + _lat[m];
								}
								_tlat = _exlat / 10000000;
								for (int n = 0; n <= 3; n++)
								{
									_exalt = _exalt * 10 + _alt[n];
								}
								_talt = _exalt / 10000000;
								ROS_INFO("%d, %.7f, %.7f, %.7f, %d", _fixsta, _tlng, _tlat, _talt, data_len);
								_exlng = 0;
								_exlat = 0;
								_exalt = 0;
								_tlng = 0;
								_tlat = 0;
								_talt = 0;
								gps_sta = 0;
								state = 0;
							}
						}

						else if (dvl_sta >= 1 && dvl_sta < 7)
						{
							buffer[dvl_sta - 1] = r_buffer[i];
							dvl_sta++;
							if (dvl_sta == 7)
							{
								for (int k = 0; k < 2; k++)
								{
									_SPED_X[k] = buffer[k];
									_SPED_Y[k] = buffer[2 + k];
									_SPED_Z[k] = buffer[4 + k];
								}
								for (int j = 0; j <= 1; j++)
								{
									_tspedx = _tspedx * 10 + _SPED_X[j];
								}
								for (int m = 0; m <= 1; m++)
								{
									_tspedy = _tspedy * 10 + _SPED_Y[m];
								}
								for (int n = 0; n <= 1; n++)
								{
									_tspedz = _tspedz * 10 + _SPED_Z[n];
								}
								ROS_INFO("%.2f, %.2f, %.2f, %d", _tspedx, _tspedy, _tspedz, data_len);
								_tspedx = 0;
								_tspedy = 0;
								_tspedz = 0;
								dvl_sta = 0;
								state = 0;
							}
						}
					}
					else {
						state = 0;
					}
				}
			}
			}
		catch (const serial::SerialException& e) 
		{
			ROS_ERROR_STREAM("Read error: " << e.what());
			ser.close();
			ros::Duration(1.0).sleep();
		}
		msg.fixsta = _fixsta;
		msg.longitude = _tlng;
		msg.latitude = _tlat;
		msg.alt = _talt;

		imu_pub.publish(msg);
		ser.close();
		r.sleep();
		cnt++;
	}
		return 0;
}