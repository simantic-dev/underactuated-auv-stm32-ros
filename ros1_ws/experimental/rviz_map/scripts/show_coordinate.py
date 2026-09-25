#!/usr/bin/env python3
import rospy
from geometry_msgs.msg import PointStamped
from geographic_msgs.msg import GeoPoint
from geodesy import utm

class CoordinateConverter:
    def __init__(self):
        rospy.init_node('utm_to_latlon_converter')
        self.sub = rospy.Subscriber('/clicked_point', PointStamped, self.point_callback)
        self.pub = rospy.Publisher('/geopoint', GeoPoint, queue_size=1)
        self.utm_zone = 51  # 青岛市UTM带号为51N
        self.utm_band = 'N'  # 北半球

    def point_callback(self, msg):
        try:
            # 提取UTM坐标（假设地图为UTM Zone 51N）
            x = msg.point.x
            y = msg.point.y
            
            # 使用Geodesy库转换UTM到经纬度
            utm_point = utm.UTMPoint(x, y, self.utm_zone, self.utm_band)
            latlon = utm_point.toLatLon()
            
            # 发布经纬度到/geopoint话题
            geopoint = GeoPoint()
            geopoint.latitude = latlon.latitude
            geopoint.longitude = latlon.longitude
            self.pub.publish(geopoint)
            
            # 终端打印结果
            rospy.loginfo(f"经纬度: Lat={latlon.latitude:.6f}, Lon={latlon.longitude:.6f}")
        except Exception as e:
            rospy.logerr(f"转换失败: {str(e)}")

if __name__ == '__main__':
    converter = CoordinateConverter()
    rospy.spin()