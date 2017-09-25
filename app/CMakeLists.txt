cmake_minimum_required(VERSION 3.4.1)

#寻找SDK中自带的库
#路径:\SDK\ndk-bundle\platforms\android-23\arch-arm\usr\lib
find_library( log-lib
              log )
find_library( android-lib
              android )
#设置变量
set(DIR ../../../../libs)

add_library(avcodec-56
            SHARED
            IMPORTED)
set_target_properties( avcodec-56
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libavcodec-56.so)

add_library(avdevice-56
            SHARED
            IMPORTED)
set_target_properties( avdevice-56
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libavdevice-56.so)

add_library(avfilter-5
            SHARED
            IMPORTED)
set_target_properties( avfilter-5
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libavfilter-5.so)

add_library(avformat-56
            SHARED
            IMPORTED)
set_target_properties( avformat-56
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libavformat-56.so)

add_library(avutil-54
            SHARED
            IMPORTED)
set_target_properties( avutil-54
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libavutil-54.so)

add_library(postproc-53
            SHARED
            IMPORTED)
set_target_properties( postproc-53
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libpostproc-53.so)

add_library(swresample-1
            SHARED
            IMPORTED)
set_target_properties( swresample-1
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libswresample-1.so)

add_library(swscale-3
            SHARED
            IMPORTED)
set_target_properties( swscale-3
                       PROPERTIES IMPORTED_LOCATION
                       ${DIR}/armeabi/libswscale-3.so)

#引入include下的头文件
include_directories(libs/include)
#include_directories(src/main/cpp/include)

#引入自己的库
add_library( native-lib
             SHARED
             src/main/cpp/native-lib.cpp )

#链接的目标库
target_link_libraries( native-lib
                       avcodec-56
                       avdevice-56
                       avfilter-5
                       avformat-56
                       avutil-54
                       postproc-53
                       swresample-1
                       swscale-3
                       ${log-lib}
                       ${android-lib})