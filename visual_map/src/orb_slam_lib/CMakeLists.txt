cmake_minimum_required(VERSION 2.8.3)
project(orb_slam_lib)

find_package(catkin_simple REQUIRED)
catkin_simple(ALL_DEPS_REQUIRED)

cs_add_library(${PROJECT_NAME}
  src/two_frame_pose.cc
  src/ORBmatcher.cc
  src/ORBextractor.cc
)

############
## EXPORT ##
############
cs_install()
cs_export()
