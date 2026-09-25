#!/usr/bin/env python
import rospy
from serial_node.msg import gps, gps_path
from geographiclib.geodesic import Geodesic
from geometry_msgs.msg import Point

class RelativePositionCalculator:
    def __init__(self):
        rospy.init_node('relative_position_calculator')
        
        # 订阅 /topic 话题
        self.sub = rospy.Subscriber('/gps/fix', gps, self.callback)
        
        # 发布路径信息
        self.path_pub = rospy.Publisher('/path_info', gps_path, queue_size=10)
        
        self.previous_point = None  # 存储前一个经纬度点
        self.path = gps_path()          # 存储路径点列表

    def callback(self, msg):
        current_lat = msg.latitude
        current_lon = msg.longitude
        
        if self.previous_point is not None:
            # 计算相对位置（距离和方位角）
            geod = Geodesic.WGS84
            result = geod.Inverse(
                self.previous_point[0], self.previous_point[1],
                current_lat, current_lon
            )
            distance = result['s12']  # 距离（米）
            azimuth = result['azi1']  # 方位角（度）
            rospy.loginfo(f"Distance: {distance:.2f} m, Azimuth: {azimuth:.2f}°")
            
            # 将当前点添加到路径
            point = Point()
            point.x = current_lat
            point.y = current_lon
            point.z = 0.0
            self.path.points.append(point)
            
            # 发布路径信息
            self.path_pub.publish(self.path)
        
        self.previous_point = (current_lat, current_lon)

if __name__ == '__main__':
    try:
        calculator = RelativePositionCalculator()
        rospy.spin()
    except rospy.ROSInterruptException:
        pass