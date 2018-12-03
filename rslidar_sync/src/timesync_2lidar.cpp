#include <ros/ros.h>
#include <std_msgs/Int32.h>
#include <std_msgs/String.h>
#include <sensor_msgs/TimeReference.h>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

ros::Publisher g_skippackets_num_pub1;
ros::Publisher g_skippackets_num_pub2;
ros::Publisher g_maxnum_diff_packetnum_pub;
bool comparePair(const std::pair<unsigned, double> a, const std::pair<unsigned, double> b)
{
  return a.second < b.second;
}

void scanCallback(const sensor_msgs::TimeReference::ConstPtr& scan_msg1,
                  const sensor_msgs::TimeReference::ConstPtr& scan_msg2)
{
  // calculate the first packet timestamp difference (us)
  double time1 = scan_msg1->header.stamp.sec * 1e6 + scan_msg1->header.stamp.nsec * 0.001;
  double time2 = scan_msg2->header.stamp.sec * 1e6 + scan_msg2->header.stamp.nsec * 0.001;

  std::vector<std::pair<unsigned, double> > lidar_vector;
  lidar_vector.push_back(std::make_pair(1, time1));
  lidar_vector.push_back(std::make_pair(2, time2));
  std::sort(lidar_vector.begin(), lidar_vector.end(), comparePair);
  unsigned delta_skip = (lidar_vector[1].second - lidar_vector[0].second) / 1200;

  std_msgs::String msg;
  std::stringstream ss;
  ss << "sync diff packets: " << delta_skip;
  ;
  msg.data = ss.str();
  g_maxnum_diff_packetnum_pub.publish(msg);

  std_msgs::Int32 skip_num;
  skip_num.data = 1;
  if (delta_skip > 0)
  {
    if (lidar_vector[0].first == 1)
    {
      g_skippackets_num_pub1.publish(skip_num);
    }
    else if (lidar_vector[0].first == 2)
    {
      g_skippackets_num_pub2.publish(skip_num);
    }
  }
  else
  {
    // std::cout << "Synchronizer!" << std::endl;
  }
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "rs_sync");
  ros::NodeHandle nh;
  ros::NodeHandle nh_private("~");

  // get the topic parameters
  std::string scan1_topic;
  std::string scan2_topic;
  nh_private.getParam(std::string("scan1_topic"), scan1_topic);
  nh_private.getParam(std::string("scan2_topic"), scan2_topic);

  std::string skippackets1_topic;
  std::string skippackets2_topic;
  nh_private.getParam(std::string("skippackets1_topic"), skippackets1_topic);
  nh_private.getParam(std::string("skippackets2_topic"), skippackets2_topic);

  // sync the rslidarscan
  message_filters::Subscriber<sensor_msgs::TimeReference> scan_sub1(nh, scan1_topic, 1);
  message_filters::Subscriber<sensor_msgs::TimeReference> scan_sub2(nh, scan2_topic, 1);
  typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::TimeReference, sensor_msgs::TimeReference>
      ScanSyncPolicy;
  message_filters::Synchronizer<ScanSyncPolicy> scan_sync(ScanSyncPolicy(10), scan_sub1, scan_sub2);
  scan_sync.registerCallback(boost::bind(&scanCallback, _1, _2));

  g_skippackets_num_pub1 = nh.advertise<std_msgs::Int32>(skippackets1_topic, 1, true);
  g_skippackets_num_pub2 = nh.advertise<std_msgs::Int32>(skippackets2_topic, true);

  std::string sync_packet_diff_topic("sync_packet_diff");
  nh_private.getParam(std::string("sync_packet_diff_topic"), sync_packet_diff_topic);
  g_maxnum_diff_packetnum_pub = nh.advertise<std_msgs::String>(sync_packet_diff_topic, 1, true);

  ros::spin();
  return 0;
}
