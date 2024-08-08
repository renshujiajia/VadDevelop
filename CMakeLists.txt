cmake_minimum_required(VERSION 3.5)

project(test_project)


set(SRC_LISTS
    src/main.cpp
)

# include_directories(${CMAKE_SOURCE_DIR}/3rdParty/webrtc-vad/github_webrtc_vad/include/webrtc)
include_directories(${CMAKE_SOURCE_DIR}/3rdParty/webrtc-vad/github_webrtc_vad/include)



add_executable(${PROJECT_NAME} ${SRC_LISTS})
target_link_libraries(${PROJECT_NAME} asound)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)