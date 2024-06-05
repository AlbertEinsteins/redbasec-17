add_subdirectory(pf)
add_subdirectory(common)


add_library(redbase STATIC ${ALL_OBJECT_FILES})

set(REDBASE_LIBS
        redbase_pf)


find_package(Threads REQUIRED)


set(REDBASE_THIRDPARTY_LIBS
        fmt)

target_link_libraries(
        redbase
        ${REDBASE_LIBS}
        ${REDBASE_THIRDPARTY_LIBS}
)

target_include_directories(
        redbase PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)