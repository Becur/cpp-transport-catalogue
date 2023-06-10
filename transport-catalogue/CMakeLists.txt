cmake_minimum_required(VERSION 3.10)

project(transport_catalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto svg.proto data_catalogue.proto graph.proto transport_router.proto)

set(TRANSPORT_CATALOGUE_FILES transport_router.h transport_router.cpp main.cpp transport_catalogue.h transport_catalogue.cpp transport_catalogue.proto svg.h svg.cpp serialization.h serialization.cpp request_handler.h request_handler.cpp map_renderer.h map_renderer.cpp json.h json.cpp json_builder.h json_builder.cpp geo.h geo.cpp domain.h domain.cpp deserialization.h deserialization.cpp router.h ranges.h graph.h svg.proto data_catalogue.proto graph.proto transport_router.proto)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORT_CATALOGUE_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)