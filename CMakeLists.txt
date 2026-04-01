# Configure with: fxsdk build-cg

cmake_minimum_required(VERSION 3.15)
project(apmicro_cg50 C)

include(GenerateG3A)
find_package(Gint 2.1 REQUIRED)

set(SOURCES
    src/main.c
    src/ui.c
    src/graphs.c
    src/graph_model.c
    src/generated/apmicro_content.c
)

add_executable(apmicro ${SOURCES})
target_include_directories(apmicro PRIVATE src src/generated)
target_compile_options(apmicro PRIVATE -Wall -Wextra -Os -std=c11)
target_link_libraries(apmicro Gint::Gint m)

generate_g3a(
    TARGET apmicro
    OUTPUT "cgmicro.g3a"
    NAME "CGMICRO"
    ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png
)
