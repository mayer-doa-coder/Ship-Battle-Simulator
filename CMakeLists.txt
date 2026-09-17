cmake_minimum_required(VERSION 3.10)

project(ShipBattleSimulator VERSION 0.1.0 LANGUAGES C CXX)

# C++17 is the single language level for every phase of the project.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ship_battle_simulator
    src/main.cpp
    src/glad.c
)

target_include_directories(ship_battle_simulator PRIVATE
    include
    external/glm
)

find_package(OpenGL REQUIRED)

if(WIN32)
    # GLFW is vendored so the Windows project builds without a package manager.
    set(GLFW_LIBRARY "${CMAKE_SOURCE_DIR}/external/glfw/lib-vc2022/glfw3.lib")
    if(NOT EXISTS "${GLFW_LIBRARY}")
        message(FATAL_ERROR "Missing ${GLFW_LIBRARY}. Restore the vendored GLFW files.")
    endif()

    target_include_directories(ship_battle_simulator PRIVATE external/glfw/include)
    target_link_libraries(ship_battle_simulator PRIVATE
        "${GLFW_LIBRARY}"
        OpenGL::GL
    )
else()
    # On Linux, GLFW is supplied by the system package manager.
    find_package(glfw3 REQUIRED)
    target_link_libraries(ship_battle_simulator PRIVATE
        glfw
        OpenGL::GL
        ${CMAKE_DL_LIBS}
    )
endif()
