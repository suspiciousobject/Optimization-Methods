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

# Директория с исходниками
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

# Проверка наличия сборочной директории
set(BUILD_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
file(MAKE_DIRECTORY "${BUILD_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${DATA_DIR}/checker_result")
file(MAKE_DIRECTORY "${DATA_DIR}/solver_result")

# ============================================================================
# SOLVER - исправленный и оптимизированный
# ============================================================================
add_executable(solver ${SRC_DIR}/solver.cpp)
set_target_properties(solver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BUILD_OUTPUT_DIR}"
)

# ============================================================================
# CHECKER - проверяет корректность результатов
# ============================================================================
add_executable(checker ${SRC_DIR}/checker.cpp)
set_target_properties(checker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BUILD_OUTPUT_DIR}"
)

# ============================================================================
# CUSTOM TARGETS для удобства
# ============================================================================

# Цель для сборки всего проекта с максимальной оптимизацией
add_custom_target(build_all
    COMMAND ${CMAKE_COMMAND} --build . --config Release
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Цель для запуска solver на всех тестах
add_custom_target(run_tests
    COMMAND echo "Запуск тестов..."
    COMMAND "${BUILD_OUTPUT_DIR}/solver" < "${DATA_DIR}/BIRDS_4.txt" > "${DATA_DIR}/BIRDS_4.txt.solution"
    COMMAND "${BUILD_OUTPUT_DIR}/checker" < "${DATA_DIR}/BIRDS_4.txt.solution" > "${DATA_DIR}/checker_result/BIRDS_4.log" 2>&1
    COMMAND echo "BIRDS_4 OK"
    
    COMMAND "${BUILD_OUTPUT_DIR}/solver" < "${DATA_DIR}/BIRDS_6.txt" > "${DATA_DIR}/BIRDS_6.txt.solution"
    COMMAND "${BUILD_OUTPUT_DIR}/checker" < "${DATA_DIR}/BIRDS_6.txt.solution" > "${DATA_DIR}/checker_result/BIRDS_6.log" 2>&1
    COMMAND echo "BIRDS_6 OK"
    
    COMMAND "${BUILD_OUTPUT_DIR}/solver" < "${DATA_DIR}/BIRDS_11.txt" > "${DATA_DIR}/BIRDS_11.txt.solution"
    COMMAND "${BUILD_OUTPUT_DIR}/checker" < "${DATA_DIR}/BIRDS_11.txt.solution" > "${DATA_DIR}/checker_result/BIRDS_11.log" 2>&1
    COMMAND echo "BIRDS_11 OK"
    
    COMMAND echo ""
    COMMAND echo "✅ Все тесты завершены!"
    COMMAND echo "Результаты в data/checker_result/"
    
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
