# 示例Python脚本（需适配实际消息类型）
import rospy
from serial_send_gps.msg import gps

def simulate_movement():
    pub = rospy.Publisher('/current_gps', gps, queue_size=10)
    rate = rospy.Rate(10)
    current_lat = 39.904200
    current_lon = 116.397500
    
    # 逐步向目标1移动
    while current_lon < 116.398676:
        current_lon += 0.00001  # 微小步长
        pub.publish(gps(lat=current_lat, lon=current_lon))
        rate.sleep()
    
    # 逐步向目标2移动...
    while current_lat < 39.905101:
        current_lat += 0.00001  # 微小步长
        pub.publish(gps(lat=current_lat, lon=current_lon))
        rate.sleep()   

    # # 逐步向目标3移动...
    # while current_lat > 39.905101:
    #     current_lat += 0.00001  # 微小步长
    #     current_lon += 0.00001  # 微小步长
    #     pub.publish(gps(lat=current_lat, lon=current_lon))
    #     rate.sleep()      