#!/bin/bash
##
# @file This file is part of EDGE.
#
# @author Alexander Breuer (anbreuer AT ucsd.edu)
#
# @section LICENSE
# Copyright (c) 2018, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# @section DESCRIPTION
# Installation of HPC-related tools and HPC-specific parameters.
##
EDGE_CURRENT_DIR=$(pwd)
EDGE_TMP_DIR=$(mktemp -d)
EDGE_N_BUILD_PROC=$(cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}')
EDGE_N_HYPER_THREADS=$(cat /proc/cpuinfo | grep processor | wc -l)

EDGE_DIST=$(cat /etc/*-release | grep PRETTY_NAME)

# detect compilers
[[ $(type -P mpiicc)  ]] && export CC=mpiicc   || export CC=mpicc
[[ $(type -P mpiicpc) ]] && export CXX=mpiicpc || export CXX=mpiCC
[[ $(type -P mpiifort) ]] && export F90=mpiifort || export CXX=mpifort

cd ${EDGE_TMP_DIR}

##########
# STREAM #
##########
wget http://www.cs.virginia.edu/stream/FTP/Code/stream.c
if [[ $(type -P icc) ]]
then
  icc -Ofast -xhost -qopenmp \
      -DNTIMES=1000 -DSTREAM_ARRAY_SIZE=$(cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}')000000 \
      -qopt-streaming-cache-evict=0 -qopt-streaming-stores always -qopt-prefetch-distance=64,8 stream.c -o stream-bench
else
  gcc -Ofast -march=native -fopenmp \
      -DNTIMES=1000 -DSTREAM_ARRAY_SIZE=$(cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}')000000 \
      stream.c -o stream-bench
fi
sudo mv ./stream-bench /usr/local/bin

###########
# OSU MPI #
###########
wget http://mvapich.cse.ohio-state.edu/download/mvapich/osu-micro-benchmarks-5.4.4.tar.gz -O osu.tar.gz
mkdir osu; tar -xzf osu.tar.gz -C osu --strip-components=1
cd osu
./configure
make -j ${EDGE_N_BUILD_PROC}
sudo make install
for osu_type in collective one-sided pt2pt startup
do
  echo "export PATH=/usr/local/libexec/osu-micro-benchmarks/mpi/${osu_type}:\${PATH}" | sudo tee --append /etc/bashrc
done
cd ..

###########
# SCORE-P #
###########
wget https://www.vi-hps.org/upload/packages/scorep/scorep-4.1.tar.gz -O scorep.tar.gz
mkdir scorep; tar -xzf scorep.tar.gz -C scorep --strip-components=1
cd scorep
if [[ $(type -P mpiicc) ]] && [[ $(type -P mpiicpc) ]] && [[ $(type -P mpiifort) ]]
then
  ./configure --disable-static --enable-shared --with-mpi=intel --with-nocross-compiler-suite=intel
else
  ./configure --disable-static --enable-shared
fi
make -j ${EDGE_N_BUILD_PROC}
sudo make install
echo "export PATH=/opt/scorep/bin:\${PATH}" | sudo tee --append /etc/bashrc
cd ..

############
# Scalasca #
############
wget http://apps.fz-juelich.de/scalasca/releases/scalasca/2.4/dist/scalasca-2.4.tar.gz -O scalasca.tar.gz
mkdir scalasca; tar -xzf scalasca.tar.gz -C scalasca --strip-components=1
cd scalasca
if [[ $(type -P mpiicc) ]] && [[ $(type -P mpiicpc) ]] && [[ $(type -P mpiifort) ]]
then
  ./configure --with-libz=no --disable-static --enable-shared --with-mpi=intel --with-nocross-compiler-suite=intel
else
  ./configure --with-libz=no --disable-static --enable-shared
fi
make -j ${EDGE_N_BUILD_PROC}
sudo make install
echo "export PATH=/opt/scalasca/bin:\${PATH}" | sudo tee --append /etc/bashrc
cd ..

##########################
# Disable NUMA balancing #
##########################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" numa_balancing=disable\""/ /etc/default/grub
  PATH=/usr/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

#####################
# Limit max C-state #
#####################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" intel_idle.max_cstate=1\""/ /etc/default/grub
  PATH=/usr/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

####################
# TSC clock source #
####################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  sudo bash -c 'echo tsc > /sys/devices/system/clocksource/clocksource0/current_clocksource'
  sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" clocksource=tsc tsc=reliable\""/ /etc/default/grub
  PATH=/usr/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

######################################################################
# Reserved kernel memory (ENA for high rate of packet buffer allocs) #
######################################################################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  sudo bash -c "echo vm.min_free_kbytes = 1048576 >> /etc/sysctl.conf"
  PATH=/sbin:$PATH sudo sysctl -p
fi

#####################################################################################################################################
# TCP Tuning for 10G and 40G following Open MPI recommentations: https://www.open-mpi.org/faq/?category=tcp#tcp-linux-kernel-params #
#####################################################################################################################################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  sudo bash -c "echo net.core.rmem_max = 16777216 >> /etc/sysctl.conf"
  sudo bash -c "echo net.core.wmem_max = 16777216 >> /etc/sysctl.conf"

  sudo bash -c "echo net.ipv4.tcp_rmem = 4096 87380 16777216 >> /etc/sysctl.conf"
  sudo bash -c "echo net.ipv4.tcp_wmem = 4096 65536 16777216 >> /etc/sysctl.conf"

  sudo bash -c "echo net.core.netdev_max_backlog = 30000 >> /etc/sysctl.conf"

  sudo bash -c "echo net.core.rmem_default = 16777216 >> /etc/sysctl.conf"
  sudo bash -c "echo net.core.wmem_default = 16777216 >> /etc/sysctl.conf"

  sudo bash -c "echo net.ipv4.tcp_mem = 16777216 16777216 16777216 >> /etc/sysctl.conf"

  sudo bash -c "echo net.ipv4.route.flush = 1 >> /etc/sysctl.conf"

  PATH=/sbin:$PATH sudo sysctl -p
fi

###############################################
# Disable irqbalance (distributed interrupts) #
###############################################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  PATH=/sbin:$PATH sudo service irqbalance stop
  PATH=/sbin:$PATH sudo chkconfig irqbalance off
fi

###############
# Set ulimits #
###############
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
# Set ulimits
cat << EOF | sudo tee /etc/security/limits.conf
# -c maximum size of core files
*           hard    core           0
*           soft    core           0

# -d maximum size of a process's data segment
*           hard    data           unlimited
*           soft    data           unlimited

# -e maximum scheduling priority (nice)
*           hard    priority       0
*           soft    priority       0

# -f maximum size of files written by the shell and childs
*           hard    fsize          unlimited
*           soft    fsize          unlimited

# -i maximum number of pending signals
*           hard    sigpending     1015390
*           soft    sigpending     1015390

# -l maximum size that my be locked into memory
*           hard    memlock        unlimited
*           soft    memlock        unlimited

# -n maximum number of open file descriptors
*           hard    nofile         65536
*           soft    nofile         65536

# -q maximum number of bytes in POSIX message queues
*           hard    msgqueue       819200
*           soft    msgqueue       819200

# -r maximum real-time scheduling priority
*           hard    rtprio         0
*           soft    rtprio         0

# -s maximum stack size
*           hard    stack          unlimited
*           soft    stack          unlimited

# -t maximum amount of CPU time in seconds
*           hard    cpu            unlimited
*           soft    cpu            unlimited

# -u maximum number of processes available to a user
*           soft    nproc          16384
*           hard    nproc          16384

# -x maximum number of file locks
*           hard    locks          unlimited
*           soft    locks          unlimited
EOF
fi

################################
# Disable hardlockup detection #
################################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" nmi_watchdog=0\""/ /etc/default/grub
  PATH=/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

###########################################################
# Boot tickless, except for the first core of each socket #
###########################################################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  if [[ ${EDGE_N_HYPER_THREADS} == 72 ]]
  then
    sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" nohz_full=1-17,19-35,37-53,55-71\""/ /etc/default/grub
  fi

  if [[ ${EDGE_N_HYPER_THREADS} == 96 ]]
  then
    sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" nohz_full=1-23,25-47,49-71,73-95\""/ /etc/default/grub
  fi

  PATH=/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

##################################################################
# Isolate all but the first core per socket from OS disturbances #
##################################################################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  if [[ ${EDGE_N_HYPER_THREADS} == 72 ]]
  then
    sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" isolcpus=1-17,19-35,37-53,55-71\""/ /etc/default/grub
  fi

  if [[ ${EDGE_N_HYPER_THREADS} == 96 ]]
  then
    sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" isolcpus=1-23,25-47,49-71,73-95\""/ /etc/default/grub
  fi

  PATH=/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

#########################
# Offload RCU callbacks #
#########################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  if [[ ${EDGE_N_HYPER_THREADS} == 72 ]]
  then
    sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" rcu_nocbs=1-17,19-35,37-53,55-71\""/ /etc/default/grub
  fi

  if [[ ${EDGE_N_HYPER_THREADS} == 96 ]]
  then
    sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" rcu_nocbs=1-23,25-47,49-71,73-95\""/ /etc/default/grub
  fi

  PATH=/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

########################
# Configure huge pages #
########################
if [[ ${EDGE_DIST} == *"CentOS"* ]]
then
  # enable transparent huge pages, set default size
  sudo sed -i /GRUB_CMDLINE_LINUX/s/\"$/" transparent_hugepage=always default_hugepagesz=2M hugepagesz=2M hugepages=0\""/ /etc/default/grub
  PATH=/sbin:$PATH sudo grub2-mkconfig -o /boot/grub2/grub.cfg
fi

############
# Clean up #
############
sudo rm -rf ${EDGE_TMP_DIR}

cd ${EDGE_CURRENT_DIR}