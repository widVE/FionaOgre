cmake_minimum_required(VERSION 2.6.2)
project(FionaOgre)

SET(USERSPECIFICPATH /mnt/dscvr/apps)
SET(DEPENDS_PATH /mnt/dscvr/apps/FionaOgre/SDK/linux/install)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${USERSPECIFICPATH}/FionaLib)
INCLUDE_DIRECTORIES(${USERSPECIFICPATH}/FionaLib/include)
INCLUDE_DIRECTORIES(${DEPENDS_PATH}/include)
INCLUDE_DIRECTORIES(${DEPENDS_PATH}/include/OGRE)
INCLUDE_DIRECTORIES(${USERSPECIFICPATH}/FionaOgre/SDK/linux/include) #for SOIL

LINK_DIRECTORIES(${USERSPECIFICPATH}/FionaOgre)
LINK_DIRECTORIES(${DEPENDS_PATH}/lib)

if(WIN32)
else()
	add_definitions(-DLINUX_BUILD)
#	add_definitions(-DROSS_TEST)
endif()

ADD_EXECUTABLE(FionaOgre main.cpp MovableText.cpp OgreDotScene.cpp VROgreAction.cpp WordCake.cpp FionaOgre.cpp )
TARGET_LINK_LIBRARIES(FionaOgre ${USERSPECIFICPATH}/FionaLib/libFionaLib.a libGLEW.so libGL.so libGLU.so ${DEPENDS_PATH}/lib/libOgreMain.so.1.9.0 ${DEPENDS_PATH}/lib/libOgrePaging.so.1.9.0 ${DEPENDS_PATH}/lib/libOgreTerrain.so.1.9.0 ${DEPENDS_PATH}/lib/libOgreOverlay.so.1.9.0 ${USERSPECIFICPATH}/FionaLib/linux/lib/libvrpn.a pthread Xext Xxf86vm)
