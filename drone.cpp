#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/Waypoint.h>
#include <mavros_msgs/WaypointPush.h>
#include <mavros_msgs/CommandCode.h>

mavros_msgs::State current_state;

void state_cb(const mavros_msgs::State::ConstPtr& msg)
{
    current_state = *msg;
    bool connected = current_state.connected;
	bool armed = current_state.armed;
	ROS_INFO("%s", armed ? "Armed" : "DisArmed");
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "drone");
    ros::NodeHandle nh;
    
    mavros_msgs::SetMode set_mode;
    set_mode.request.custom_mode = "AUTO.MISSION";

    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>
            ("mavros/state", 10, state_cb);
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>
            ("mavros/set_mode");
    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>
            ("mavros/cmd/arming");
    ros::ServiceClient wp_client = nh.serviceClient<mavros_msgs::WaypointPush>
            ("mavros/mission/push");

    ros::Rate rate(20.0);

    mavros_msgs::WaypointPush wp_push;
    mavros_msgs::Waypoint wp;

    wp.frame          = mavros_msgs::Waypoint::FRAME_GLOBAL_REL_ALT;
    wp.command        = mavros_msgs::CommandCode::NAV_TAKEOFF;
    wp.is_current     = true;
    wp.autocontinue   = true;
    wp.x_lat          = 41.104132;
    wp.y_long         = 29.024368;
    wp.z_alt          = 10;
    wp_push.request.waypoints.push_back(wp);

    wp.frame          = mavros_msgs::Waypoint::FRAME_GLOBAL_REL_ALT;
    wp.command        = mavros_msgs::CommandCode::NAV_LOITER_TIME;
    wp.is_current     = false;
    wp.autocontinue   = true;
    wp.x_lat          = 41.103493;
    wp.y_long         = 29.024617;
    wp.z_alt          = 20;
	wp.param1		    = 10;
	wp.param3			= 2;
	wp.param4			= 1;
    wp_push.request.waypoints.push_back(wp);

    wp.frame          = mavros_msgs::Waypoint::FRAME_MISSION;
    wp.command        = mavros_msgs::CommandCode::NAV_RETURN_TO_LAUNCH;
    wp.is_current     = false;
    wp.autocontinue   = true;
    wp.x_lat          = 0;
    wp.y_long         = 0;
    wp.z_alt          = 0;
    wp_push.request.waypoints.push_back(wp);

    while(ros::ok() && current_state.connected)
    {
        ros::spinOnce();
        rate.sleep();
    }

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;
    if (arming_client.call(arm_cmd) &&
        arm_cmd.response.success){
        ROS_INFO("Vehicle armed");
    }

    // Send WPs to Vehicle
    if (wp_client.call(wp_push)) {
        ROS_INFO("Send waypoints ok: %d", wp_push.response.success);
        if (current_state.mode != "AUTO.MISSION") {
            if( set_mode_client.call(set_mode) &&
                set_mode.response.mode_sent){
                ROS_INFO("AUTO.MISSION enabled");
            }
        }
    }
    else
        ROS_ERROR("Send waypoints FAILED.");

    while(ros::ok()){
        ros::spinOnce();
        rate.sleep();
    }

    return 0;
}