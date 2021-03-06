cmake_minimum_required(VERSION 2.8.3)
project(gazebo_ros_replay_plugins)

# Check c++11 / c++0x
# Added for NRP
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    message(STATUS "Compiler supports cxx11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    message(STATUS "Compiler supports c++0x")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()
# End of NRP check

find_package(catkin REQUIRED COMPONENTS
  cmake_modules
  roscpp
  rosbag
  gazebo_msgs
  std_srvs
  cle_ros_msgs
)

# Depend on system install of Gazebo
find_package(gazebo REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS}")

find_package(Boost REQUIRED COMPONENTS filesystem)

catkin_package(
  LIBRARIES
  gazebo_ros_recording_plugin
  gazebo_ros_playback_plugin

  CATKIN_DEPENDS
  roscpp
  rosbag
  gazebo_msgs
  std_srvs
  cle_ros_msgs
)

include_directories(include
  ${Boost_INCLUDE_DIRS}
  ${catkin_INCLUDE_DIRS}
  ${GAZEBO_INCLUDE_DIRS}
  ${TBB_INCLUDE_DIR}
)

link_directories(
  ${catkin_LIBRARY_DIRS}
)

set(cxx_flags)
foreach (item ${GAZEBO_CFLAGS})
  set(cxx_flags "${cxx_flags} ${item}")
endforeach ()

set(ld_flags)
foreach (item ${GAZEBO_LDFLAGS})
  set(ld_flags "${ld_flags} ${item}")
endforeach ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GAZEBO_CXX_FLAGS}")

## Plugins
add_library(gazebo_ros_recording_plugin src/gazebo_ros_recording_plugin.cpp src/rosbag/recorder.cpp)
add_dependencies(gazebo_ros_recording_plugin ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
set_target_properties(gazebo_ros_recording_plugin PROPERTIES LINK_FLAGS "${ld_flags}")
set_target_properties(gazebo_ros_recording_plugin PROPERTIES COMPILE_FLAGS "${cxx_flags}")
target_link_libraries(gazebo_ros_recording_plugin ${GAZEBO_LIBRARIES} ${catkin_LIBRARIES} ${Boost_LIBRARIES})

add_library(gazebo_ros_playback_plugin src/gazebo_ros_playback_plugin.cpp src/rosbag/player.cpp)
add_dependencies(gazebo_ros_playback_plugin ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
set_target_properties(gazebo_ros_playback_plugin PROPERTIES LINK_FLAGS "${ld_flags}")
set_target_properties(gazebo_ros_playback_plugin PROPERTIES COMPILE_FLAGS "${cxx_flags}")
target_link_libraries(gazebo_ros_playback_plugin ${GAZEBO_LIBRARIES} ${catkin_LIBRARIES} ${Boost_LIBRARIES})

# Install Gazebo System Plugins
install(TARGETS gazebo_ros_recording_plugin gazebo_ros_playback_plugin
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)
