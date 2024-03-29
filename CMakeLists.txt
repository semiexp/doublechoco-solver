cmake_minimum_required(VERSION 3.1)
project(doublechoco-solver CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(source ${PROJECT_SOURCE_DIR}/glucose/core/Solver.cc ${PROJECT_SOURCE_DIR}/glucose/utils/Options.cc ${PROJECT_SOURCE_DIR}/src/doublechoco/BoardManager.cc ${PROJECT_SOURCE_DIR}/src/doublechoco/Problem.cc ${PROJECT_SOURCE_DIR}/src/doublechoco/Propagator.cc ${PROJECT_SOURCE_DIR}/src/doublechoco/Solver.cc ${PROJECT_SOURCE_DIR}/src/doublechoco/Balancer.cc ${PROJECT_SOURCE_DIR}/src/doublechoco/Shape.cc ${PROJECT_SOURCE_DIR}/src/Group.cc)
set(evolmino_source ${PROJECT_SOURCE_DIR}/glucose/core/Solver.cc ${PROJECT_SOURCE_DIR}/glucose/utils/Options.cc ${PROJECT_SOURCE_DIR}/src/evolmino/BoardManager.cc ${PROJECT_SOURCE_DIR}/src/evolmino/Problem.cc ${PROJECT_SOURCE_DIR}/src/evolmino/Propagator.cc ${PROJECT_SOURCE_DIR}/src/evolmino/Solver.cc ${PROJECT_SOURCE_DIR}/src/Group.cc)

if (USE_EMSCRIPTEN)
    set(CMAKE_CXX_COMPILER em++)
    set(CMAKE_CXX_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s MODULARIZE=1 -s SINGLE_FILE=1 -s ENVIRONMENT=web,worker -s FILESYSTEM=0 --memory-init-file 0")
    add_executable(doublechoco-solver ${source} ${evolmino_source} ${PROJECT_SOURCE_DIR}/src/EmMain.cc)
    set_target_properties(doublechoco-solver PROPERTIES COMPILE_OPTIONS --bind)
    set_target_properties(doublechoco-solver PROPERTIES LINK_FLAGS --bind)
    set_target_properties(doublechoco-solver PROPERTIES OUTPUT_NAME "doublechoco_solver.js")
else()
    add_executable(doublechoco-solver ${source} ${PROJECT_SOURCE_DIR}/src/Main.cc)
    add_executable(evolmino-solver ${evolmino_source} ${PROJECT_SOURCE_DIR}/src/EvolminoMain.cc)
    target_include_directories(evolmino-solver PUBLIC ${PROJECT_SOURCE_DIR}/glucose ${PROJECT_SOURCE_DIR}/src)
endif()

target_include_directories(doublechoco-solver PUBLIC ${PROJECT_SOURCE_DIR}/glucose ${PROJECT_SOURCE_DIR}/src)
