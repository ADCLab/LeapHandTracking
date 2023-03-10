#!/bin/bash
CUR_DIR=$(pwd)
SRC_DIR=${CUR_DIR}/ultraleap-hand-tracking-service/samples/   
BUILD_TYPE='Release'    
REPOS_BUILD_ROOT=${CUR_DIR}/build     
REPOS_INSTALL_ROOT=${CUR_DIR}/exe    
    
cmake -S ${SRC_DIR} -B ${REPOS_BUILD_ROOT}/${BUILD_TYPE}/LeapSDK/leapc_example -DCMAKE_INSTALL_PREFIX="${REPOS_INSTALL_ROOT}/leapc_example" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"    
    
cmake --build ${REPOS_BUILD_ROOT}/${BUILD_TYPE}/LeapSDK/leapc_example -j --config ${BUILD_TYPE}    