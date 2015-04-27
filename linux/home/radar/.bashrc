# .bashrc
#Not the full bashrc file for the fhrlinux, but the ROS important parts copied here.

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific aliases and functions
PATH=$PATH:$HOME/bin

export PATH

# FHR ROS operational : 20150427
export ROSHOST="209.114.1.2"
export RSTPATH=$HOME/ros.3.6/
source $RSTPATH/.profile.bash
#Old style RST stuff here
#export RSTPATH=$HOME/rst
#source $RSTPATH/profile.superdarn-rst.linux.bash
export EDITOR=nano

