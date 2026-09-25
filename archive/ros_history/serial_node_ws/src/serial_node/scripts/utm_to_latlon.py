#!/usr/bin/env python3
import rospy
from geometry_msgs.msg import PointStamped
from geographic_msgs.msg import GeoPoint
from pyproj import Proj, Transformer

class CoordinateConverter:
    def __init__(self):
        rospy.init_node('utm_to_latlon_converter')
        self.sub = rospy.Subscriber('/clicked_point', PointStamped, self.point_callback)
        self.pub = rospy.Publisher('/geopoint', GeoPoint, queue_size=1)
        
        # 定义UTM投影参数（青岛市UTM Zone 51N）
        self.utm_zone = 51
        self.utm_proj = Proj(proj="utm", zone=self.utm_zone, ellps="WGS84", south=False)
        self.transformer = Transformer.from_proj(self.utm_proj, Proj("EPSG:4326"))  # WGS84经纬度

    def point_callback(self, msg):
        try:
            # 提取UTM坐标
            x = msg.point.x + 211912.1325
            y = msg.point.y + 3968128.5482
            
            # 转换UTM到经纬度
            lon, lat = self.transformer.transform(x, y)
            
            # 发布经纬度到/geopoint话题
            geopoint = GeoPoint()
            geopoint.latitude = lat
            geopoint.longitude = lon
            self.pub.publish(geopoint)
            
            # 终端打印结果
            rospy.loginfo(f"经纬度: Lat={lat:.6f}, Lon={lon:.6f}, x={x:.6f}, y={y:.6f}")
        except Exception as e:
            rospy.logerr(f"转换失败: {str(e)}")

if __name__ == '__main__':
    converter = CoordinateConverter()
    rospy.spin()