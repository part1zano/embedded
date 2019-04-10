cmake_minimum_required (VERSION 2.6)

project(sysctl_temp)

add_executable(temp cgi-sysctl.c)

install(TARGETS temp DESTINATION www)
