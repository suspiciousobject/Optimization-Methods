cmake_minimum_required(VERSION 3.10)
project(BirdsOptimization)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги оптимизации
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Установка типа сборки по умолчанию
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Директории проекта
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

# Проверка наличия необходимых директорий
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
file(MAKE_DIRECTORY "${DATA_DIR}/checker_result")
file(MAKE_DIRECTORY "${DATA_DIR}/solver_result")

# ============================================================================
# SOLVER - переструктурированный с модульной архитектурой
# ============================================================================

# Исполняемый файл solver (только .cpp файл, .hpp уже в include директории)
add_executable(solver ${SRC_DIR}/solver.cpp)

# Добавляем директорию include в путь поиска заголовков
target_include_directories(solver PRIVATE ${INCLUDE_DIR})

# Устанавливаем директорию вывода для исполняемого файла
set_target_properties(solver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin"
)

# ============================================================================
# CHECKER - проверяет корректность результатов
# ============================================================================

add_executable(checker ${SRC_DIR}/checker.cpp)
set_target_properties(checker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin"
)

# ============================================================================
# CUSTOM TARGETS для удобства
# ============================================================================

# Цель для сборки всего проекта с максимальной оптимизацией
add_custom_target(build_all
    COMMAND ${CMAKE_COMMAND} --build . --config Release
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
