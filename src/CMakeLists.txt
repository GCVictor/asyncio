set(SOURCES eventloop.cc task.cc)
add_library(asyncio ${SOURCES})
target_include_directories(asyncio PRIVATE ${ASYNCIO_SOURCE_INCLUDE_DIRS})
