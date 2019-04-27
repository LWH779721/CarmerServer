cmake_minimum_required(VERSION 2.8) 

PROJECT(CarmerServerTest)

INCLUDE_DIRECTORIES(../include) 
INCLUDE_DIRECTORIES(../depend/jrtplib-3.11.1/include/)
INCLUDE_DIRECTORIES(../depend/faac-1.29.9.2/include) 

link_directories(${PROJECT_SOURCE_DIR}/../depend/jrtplib-3.11.1/libs/)
link_directories(${PROJECT_SOURCE_DIR}/../depend/faac-1.29.9.2/libs/)

ADD_EXECUTABLE(H264Nalu_test H264Nalu_test.c)  
TARGET_LINK_LIBRARIES(H264Nalu_test CarmerServer)

ADD_EXECUTABLE(jrtpSend_h264_test jrtpSend_h264_test.cpp)  
TARGET_LINK_LIBRARIES(jrtpSend_h264_test CarmerServer jrtp)

ADD_EXECUTABLE(jrtpSend_aac_test jrtpSend_aac_test.cpp)  
TARGET_LINK_LIBRARIES(jrtpSend_aac_test CarmerServer jrtp faac)

ADD_EXECUTABLE(jrtpSend_pcm_test jrtpSend_pcm_test.cpp)  
TARGET_LINK_LIBRARIES(jrtpSend_pcm_test CarmerServer jrtp asound faac)
