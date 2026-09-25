#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "serial/serial.h"
#include "serial_send_gps/usb_gps.h"
#include <fstream>  // 添加文件流头文件

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


uint8_t sc = 0;
uint8_t ac = 0;

typedef union
{
	int16_t data_euagl;
	int8_t data8_euagl[2];
}data_recv_euagl;

data_recv_euagl _ROL;
data_recv_euagl _PIT;
data_recv_euagl _YAW;
uint8_t _FUSION_STA = 0;

typedef union
{
	int32_t data_alt;
	int8_t data8_alt[4];
}data_recv_alt;

data_recv_alt _ALT_FU;
data_recv_alt _ALT_ADD;
uint8_t _ALT_STA = 0;

typedef union
{
	int32_t data_poserr;
	int8_t data8_poserr[4];
}data_recv_poserr;

data_recv_poserr _POS_X;
data_recv_poserr _POS_Y;

typedef union
{
	uint32_t data_gps1;
	uint8_t data8_gps1[4];
}data_recv_gps1;

typedef union
{
	int16_t data_gps2;
	int8_t data8_gps2[2];
}data_recv_gps2;

uint8_t _sta1, _sta2, _sta3, _sta4 = 0;
data_recv_gps1 _lng;
data_recv_gps1 _lat;
data_recv_gps2 _ang;

typedef union
{
	int16_t data_dvl;
	int8_t data8_dvl[2];
}data_recv_dvl;

data_recv_dvl _SPED_X;
data_recv_dvl _SPED_Y;
data_recv_dvl _SPED_Z;

float _trol, _tpit, _tyaw, _tlng, _tlat, _tang = 0;

// 在main函数中添加文件流对象
std::ofstream outFile;
std::string filename = "gps_data.txt";

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
	catch (const serial::SerialException& e) 
	{  
		ROS_ERROR_STREAM("Communication error: " << e.what());  	// 新增：读写过程中断
		ser.close();
		return -1;
	}
	ROS_INFO_STREAM("Succeed to open port");

	int cnt = 0;
	ros::Rate r(10);
	ros::Publisher imu_pub = nh.advertise<serial_send_gps::usb_gps>("current_gps", 1000);

	// 打开文件（追加模式）
    outFile.open(filename.c_str(), std::ios::app);
    if (!outFile.is_open()) {
        ROS_ERROR("Failed to open output file!");
        return -1;
    }
    
    // 添加CSV表头
    outFile << "timestamp,fixsta,latitude,longitude,altitude" << std::endl;

	while (ros::ok())
	{
		serial_send_gps::usb_gps msg;
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
				size_t bytes_to_read = std::min(bytes_available, (size_t)128);			// 安全读取
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

					else if (state == 2 && r_buffer[i] == 0xf1)
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
						/*for (int q = 0; q < data_len + 4; q++)
						{
							sc += r_buffer[q];
							ac += sc;
						}
						if (sc != r_buffer[data_len + 4] || ac != r_buffer[data_len + 5])
						{
							return 0;
						}
						*/

						if (euagl_sta >= 1 && euagl_sta < 8)
						{
							buffer[euagl_sta - 1] = r_buffer[i];
							euagl_sta++;
							if (euagl_sta == 8)
							{
								for (int k = 0; k < 2; k++)
								{
									_ROL.data8_euagl[k] = buffer[k];
									_PIT.data8_euagl[k] = buffer[2 + k];
									_YAW.data8_euagl[k] = buffer[4 + k];
								}
								_FUSION_STA = buffer[6];
								_trol = _ROL.data_euagl / 100.0;
								_tpit = _PIT.data_euagl / 100.0;
								_tyaw = _YAW.data_euagl / 100.0;
								ROS_INFO("%d, %.2f, %.2f, %.2f, %d", _FUSION_STA, _trol, _tpit, _tyaw, data_len);
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
									_ALT_FU.data8_alt[k] = buffer[k];
									_ALT_ADD.data8_alt[k] = buffer[4 + k];
								}
								_ALT_STA = buffer[8];
								ROS_INFO("%d, %d, %d, %d", _ALT_STA, _ALT_FU.data_alt, _ALT_ADD.data_alt, data_len);
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
									_POS_X.data8_poserr[k] = buffer[k];
									_POS_Y.data8_poserr[k] = buffer[4 + k];
								}
								ROS_INFO("%d, %d, %d", _POS_X.data_poserr, _POS_Y.data_poserr, data_len);
								poserr_sta = 0;
								state = 0;
							}
						}

						else if (gps_sta >= 1 && gps_sta < 15)
						{
							buffer[gps_sta - 1] = r_buffer[i];
							gps_sta++;
							if (gps_sta == 15)
							{
								for (int k = 0; k < 4; k++)
								{
									_lng.data8_gps1[k] = buffer[k];
									_lat.data8_gps1[k] = buffer[4 + k];
								}
								for (int l = 0; l < 2;l++)
								{
									_ang.data8_gps2[l] = buffer[8+l];
								}
								for(int p = 0;p < 1;p++)
								{
									_sta1 = buffer[10 + p];
									_sta2 = buffer[11 + p];
									_sta3 = buffer[12 + p];
									_sta4 = buffer[13 + p];
								}
								_tlng = _lng.data_gps1 / 100000000.0;
								_tlat = _lat.data_gps1 / 100000000.0;
								_tang= _ang.data_gps2 / 100.0;
								ROS_INFO("%d, %d, %d, %d, %.7f, %.7f, %.2f, %d", _sta1, _sta2, _sta3, _sta4, _tlng, _tlat, _tang, data_len);
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
									_SPED_X.data8_dvl[k] = buffer[k];
									_SPED_Y.data8_dvl[k] = buffer[2 + k];
									_SPED_Z.data8_dvl[k] = buffer[4 + k];
								}
								ROS_INFO("%d, %d, %d, %d", _SPED_X.data_dvl, _SPED_Y.data_dvl, _SPED_Z.data_dvl, data_len);
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
		msg.fixsta = _sta1;
		msg.lng = _tlng;
		msg.lat = _tlat;
		msg.alt = _tang;
		msg.alt = _trol;
		msg.alt = _tpit;
		msg.alt = _tyaw;
		
        // 写入文件
        outFile << ros::Time::now() << "," 
                << msg.fixsta << ","
                << msg.lat << "," 
                << msg.lng << ","
                << msg.alt << ","
				<< msg.alt << ","
				<< msg.alt << ","
				<< msg.alt <<std::endl;

		imu_pub.publish(msg);
		r.sleep();
		cnt++;
	}
    // 程序结束时关闭文件
    	outFile.close();
    	ser.close();  // 最终关闭串口
		return 0;
}