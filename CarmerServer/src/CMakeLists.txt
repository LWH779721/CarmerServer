cmake_minimum_required(VERSION 2.8) 

PROJECT(CarmerServer)
 
INCLUDE_DIRECTORIES(../include) 

AUX_SOURCE_DIRECTORY(. CarmerServer)

ADD_LIBRARY (CarmerServer SHARED ${CarmerServer})
