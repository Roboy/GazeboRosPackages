#ifndef GENERIC_CONTROLLER_PLUGIN_H
#define GENERIC_CONTROLLER_PLUGIN_H

#include <vector>
#include <string>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/gazebo.hh>

#include <std_msgs/Float64.h>
#include <ros/ros.h>

namespace gazebo
{

typedef std::map<std::string, physics::JointPtr> JointMap;

class GenericControlPlugin : public ModelPlugin
{

public:

  GenericControlPlugin();
  ~GenericControlPlugin() {}

  // Load the plugin and initilize all controllers
  void Load(physics::ModelPtr parent, sdf::ElementPtr sdf);

  // Simulation update callback function
  void OnUpdate(const common::UpdateInfo & /*_info*/);

private:

  // check if controller for current joint is specified in SDF and return pointer to sdf element
  bool existsControllerSDF(sdf::ElementPtr &sdf_ctrl_def, const sdf::ElementPtr &sdf, 
                           const physics::JointPtr &joint);
  
  // get PID controller values from SDF
  common::PID getControllerPID(const sdf::ElementPtr &sdf_ctrl_def);
  
  // get controller type from SDF
  std::string getControllerType(const sdf::ElementPtr &sdf_ctrl_def);
  
  // Method for creating a position controller for a given joint
  void createPositionController(const physics::JointPtr &joint, const common::PID &pid_param);
  
  // Method for creating a velocity controller for a given joint
  void createVelocityController(const physics::JointPtr &joint, const common::PID &pid_param);
  
  // Generic position command callback function (ROS topic)
  void positionCB(const std_msgs::Float64::ConstPtr &msg, const physics::JointPtr &joint);

  // Generic velocity command callback function (ROS topic)
  void velocityCB(const std_msgs::Float64::ConstPtr &msg, const physics::JointPtr &joint);
  
  // ROS node handle
  ros::NodeHandle m_nh;

  // Pointer to the model
  physics::ModelPtr m_model;

  // Pointer to joint controllers
  physics::JointControllerPtr m_joint_controller;

  // Map of joint pointers
  JointMap m_joints;

  // Pointer to the update event connection
  event::ConnectionPtr m_updateConnection;

  // ROS subscriber for joint control values
  std::vector<ros::Subscriber> m_pos_sub_vec;
  std::vector<ros::Subscriber> m_vel_sub_vec;

};

} // namespace gazebo

#endif