/*
 * Desc: Gazebo plugin providing generic controllers for robot joints
 * This plugin provides ROS topics to control single joints of the robot. The controlled joints can be specified in the SDF as plugin tags
 * Author: Lars Pfotzer
 */ 

#include "generic_controller_plugin.h"

#include <boost/bind.hpp>

#include <sensor_msgs/JointState.h>
#include <ros/time.h>

namespace gazebo
{

GenericControlPlugin::GenericControlPlugin()
{
  m_nh = ros::NodeHandle();
}

void GenericControlPlugin::Load(physics::ModelPtr parent, sdf::ElementPtr sdf) 
{
  // Store the pointer to the model
  m_model = parent;
  m_joint_controller = m_model->GetJointController();
  m_joints = m_joint_controller->GetJoints();

  //sdf::ElementPtr sdf->GetElement("");
  ROS_INFO("sdf name %s, sdf description %s", sdf->GetName().c_str(), sdf->GetDescription().c_str());
  
  for (JointMap::iterator joint_iter = m_joints.begin(); joint_iter != m_joints.end(); ++joint_iter)
  {
    physics::JointPtr joint = joint_iter->second;
    sdf::ElementPtr sdf_ctrl_def;
    
    // check, if controller for current joint was specified in the SDF and return sdf element pointer to controller 
    if (existsControllerSDF(sdf_ctrl_def, sdf, joint))
    {
      physics::JointPtr joint = joint_iter->second;

      // get controller parameter from sdf file
      common::PID ctrl_pid = getControllerPID(sdf_ctrl_def);
      std::string ctrl_type = getControllerType(sdf_ctrl_def);
      
      // create controller regarding the specified controller type
      if (ctrl_type == "position")
      {
        createPositionController(joint, ctrl_pid);
      }
      else if (ctrl_type == "velocity")
      {
        createVelocityController(joint, ctrl_pid);
      }
    }
  }
  
  // Listen to the update event. This event is broadcast every simulation iteration.
  m_updateConnection = event::Events::ConnectWorldUpdateBegin(boost::bind(&GenericControlPlugin::OnUpdate, this, _1));
}

// Called by the world update start event
void GenericControlPlugin::OnUpdate(const common::UpdateInfo & /*_info*/)
{
}

///////////////////////////////////////// SDF parser functions ////////////////////////////////////////////

bool GenericControlPlugin::existsControllerSDF(sdf::ElementPtr &sdf_ctrl_def, const sdf::ElementPtr &sdf, 
                                               const physics::JointPtr &joint)
{
  sdf::ElementPtr sdf_ctrl = sdf->GetElement("controller");
  while (sdf_ctrl != NULL)
  {
    sdf::ParamPtr joint_name_attr = sdf_ctrl->GetAttribute("joint_name");
    if (joint_name_attr != NULL)
    {
      std::string joint_name = joint_name_attr->GetAsString();
      if (joint_name == joint->GetName())
      {
        ROS_INFO("Found controller for joint %s", joint_name.c_str());
        sdf_ctrl_def = sdf_ctrl;
        return true;
      }
      else
      {
        // find next controller
        sdf_ctrl = sdf_ctrl->GetNextElement("controller");
      }
    }
    else
    {
      ROS_WARN("Attribute 'joint_name' is not available for current controller.");
      return false;
    }
  }
  
  ROS_WARN("No controller for joint %s found", joint->GetName().c_str());
  return false;
}

common::PID GenericControlPlugin::getControllerPID(const sdf::ElementPtr &sdf_ctrl_def)
{
  common::PID pid_param;
  
  if (sdf_ctrl_def != NULL)
  {
    sdf::ElementPtr elem_pid = sdf_ctrl_def->GetElement("pid");
    if (elem_pid != NULL)
    {
      sdf::Vector3 pid_values = elem_pid->GetValueVector3();
      ROS_INFO("Controller PID values p=%f, i=%f, d=%f", pid_values.x, pid_values.y, pid_values.z);
      pid_param = common::PID(pid_values.x, pid_values.y, pid_values.z);
      return pid_param;
    }
  }
  
  ROS_WARN("Could not find controller PID parameter in SDF file: Using default values.");
  pid_param = common::PID(1.0, 0.1, 0.01);
  return pid_param;
}

std::string GenericControlPlugin::getControllerType(const sdf::ElementPtr &sdf_ctrl_def)
{
  std::string ctrl_type = "";
  
  if (sdf_ctrl_def != NULL)
  {
    sdf::ElementPtr elem_type = sdf_ctrl_def->GetElement("type");
    if (elem_type != NULL)
    {
      ctrl_type = elem_type->GetValueString();
      ROS_INFO("Controller has type %s", ctrl_type.c_str());
      return ctrl_type;
    }
  }
  
  ROS_WARN("Could not find controller type in SDF file.");
  return ctrl_type;
}

//////////////////////////////////////// Controller construction //////////////////////////////////////////

void GenericControlPlugin::createPositionController(const physics::JointPtr &joint, const common::PID &pid_param)
{
  // generate joint topic name using the model name as prefix
  std::string topic_name = m_model->GetName() + "/" + joint->GetName() + "/cmd_pos";
  
  // Add ROS topic for position control
  m_pos_sub_vec.push_back(m_nh.subscribe<std_msgs::Float64>(topic_name, 1, 
                                                            boost::bind(&GenericControlPlugin::positionCB, this, _1, joint)));

  // Create PID parameter for position controller
  m_joint_controller->SetPositionPID(joint->GetScopedName(), pid_param);
  
  // Initialize controller with zero position
  m_joint_controller->SetPositionTarget(joint->GetScopedName(), 0.0);
  
  ROS_INFO("Added new position controller for joint %s", joint->GetName().c_str());
}

void GenericControlPlugin::createVelocityController(const physics::JointPtr &joint, const common::PID &pid_param)
{
  // generate joint topic name using the model name as prefix
  std::string topic_name = m_model->GetName() + "/" + joint->GetName() + "/cmd_vel";
  
  // Add ROS topic for velocity control
  m_vel_sub_vec.push_back(m_nh.subscribe<std_msgs::Float64>(topic_name, 1, 
                                                            boost::bind(&GenericControlPlugin::velocityCB, this, _1, joint)));

  // Create PID parameter for velocity controller
  m_joint_controller->SetVelocityPID(joint->GetScopedName(), pid_param);
  
  // Initialize controller with zero velocity
  m_joint_controller->SetVelocityTarget(joint->GetScopedName(), 0.0);
  
  ROS_INFO("Added new velocity controller for joint %s", joint->GetName().c_str());
}

//////////////////////////////////////// ROS topic callback functions //////////////////////////////////////////

void GenericControlPlugin::positionCB(const std_msgs::Float64::ConstPtr &msg, const physics::JointPtr &joint)
{
  ROS_INFO("positionCB called! Joint name = %s, joint pos = %f", joint->GetName().c_str(), msg->data);

  double angle_rad(msg->data);
  m_joint_controller->SetPositionTarget(joint->GetScopedName(), angle_rad);
  //pid.SetCmd(angle_rad);
}

void GenericControlPlugin::velocityCB(const std_msgs::Float64::ConstPtr &msg, const physics::JointPtr &joint)
{
  ROS_INFO("velocityCB called! Joint name = %s, joint vel = %f", joint->GetName().c_str(), msg->data);

  double velocity_m_per_sec(msg->data);
  m_joint_controller->SetVelocityTarget(joint->GetScopedName(), velocity_m_per_sec);
  //pid.SetCmd(velocity_m_per_sec);
}

// Register this plugin with the simulator
GZ_REGISTER_MODEL_PLUGIN(GenericControlPlugin)

} // namespace gazebo

