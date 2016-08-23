cmake_minimum_required(VERSION 3.3)
project(SEAL_ORAM)

include_directories(include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

set(GENERAL_SOURCE_FILES
        include/ServerConnector.h
        include/MongoConnector.h
        include/Config.h
        include/Util.h
        include/ORAM.h
        src/MongoConnector.cpp
        src/Util.cpp)

set(IBSOS_SOURCE_FILES
        include/IBSOS.h
        src/IBSOS.cpp)

set(PATHORAM_SOURCE_FILES
        include/PathORAM.h
        include/RecursivePathORAM.h
        src/PathORAM.cpp
        src/RecursivePathORAM.cpp)

set(BINARYORAM_SOURCE_FILES
        include/BinaryORAM.h
        include/RecursiveBinaryORAM.h
        src/BinaryORAM.cpp
        src/RecursiveBinaryORAM.cpp)

set(SRORAM_SOURCE_FILES
        include/SRORAM.h
        src/SRORAM.cpp)

set(TPORAM_SOURCE_FILES
        include/PartitionORAM.h
        include/PartitionORAMInstance.h
        include/TPPartitionORAMInstance.h
        include/RecursivePartitionORAM.h
        include/RecursiveTPPartitionORAMInstance.h
        src/TPPartitionORAMInstance.cpp
        src/RecursiveTPPartitionORAMInstance.cpp)

add_executable(IBSOS_main IBSOS_main.cpp ${GENERAL_SOURCE_FILES} ${IBSOS_SOURCE_FILES})
target_link_libraries(IBSOS_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(PathORAM_main PathORAM_main.cpp ${GENERAL_SOURCE_FILES} ${PATHORAM_SOURCE_FILES})
target_link_libraries(PathORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(BinaryORAM_main BinaryORAM_main.cpp ${GENERAL_SOURCE_FILES} ${BINARYORAM_SOURCE_FILES})
target_link_libraries(BinaryORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(SRORAM_main SRORAM_main.cpp ${GENERAL_SOURCE_FILES} ${SRORAM_SOURCE_FILES})
target_link_libraries(SRORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(TPORAM_main TPORAM_main.cpp ${GENERAL_SOURCE_FILES} ${TPORAM_SOURCE_FILES})
target_link_libraries(TPORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(RecursivePathORAM_main RecursivePathORAM_main.cpp ${GENERAL_SOURCE_FILES} ${PATHORAM_SOURCE_FILES})
target_link_libraries(RecursivePathORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(RecursiveBinaryORAM_main RecursiveBinaryORAM_main.cpp ${GENERAL_SOURCE_FILES} ${BINARYORAM_SOURCE_FILES})
target_link_libraries(RecursiveBinaryORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)

add_executable(RecursiveTPORAM_main RecursiveTPORAM_main.cpp ${GENERAL_SOURCE_FILES} ${TPORAM_SOURCE_FILES})
target_link_libraries(RecursiveTPORAM_main cryptopp pthread mongoclient boost_thread boost_system boost_regex)
