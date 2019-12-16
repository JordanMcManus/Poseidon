#!/bin/bash

#update the apps
echo --------------------
echo Updating Deb.
echo --------------------
sudo apt update

#upgrade the apps
echo --------------------
echo Updating Apps.
echo --------------------
sudo apt upgrade -y

# Install utilities with the YES answer to apt prompt
echo --------------------
echo Installing Utilities.
echo --------------------
sudo apt install gcc python3-dev python3-pip python3-setuptools git curl -y

# Install RPi.GPIO
pip3 install RPi.GPIO
# Now ROS Installation
echo --------------------
echo Installing ROS.
echo --------------------
# Setup sources.list
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
# Setup keys
sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
curl -sSL 'http://keyserver.ubuntu.com/pks/lookup?op=get&search=0xC1CF6E31E6BADE8868B172B4F42ED6FBAB17C654' | sudo apt-key add -

# Actual Installation
sudo apt-get update

#sudo apt install ros-kinetic-ros-base g++ -y
sudo apt install ros-melodic-ros-base g++ -y

# Initialize rosdep
echo --------------------
echo Initialize ROSdep
echo --------------------
sudo rosdep init
echo --------------------
echo Updating ROSdep
echo --------------------
rosdep update


#instaling system installation script here
echo --------------------
echo End of script
echo --------------------
