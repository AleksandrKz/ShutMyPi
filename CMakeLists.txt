# CMakeList.txt: проект CMake для ShutMyPi; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
project ("ShutMyPi")

find_package(OpenSSL REQUIRED)

# Добавьте источник в исполняемый файл этого проекта.
add_executable (ShutMyPi "ShutMyPi.cpp" "ShutMyPi.h")

# TODO: Добавьте тесты и целевые объекты, если это необходимо.

target_link_libraries(ShutMyPi PRIVATE OpenSSL::SSL)
target_link_libraries(ShutMyPi PRIVATE OpenSSL::Crypto)