#
# DANTE :: System Automatycznych Testów Jednostkowych oraz Akceptacyjnych
# Tomasz Jaworski, 2018-2020
#
# Plik wygenerowany automatycznie
# Znacznik czasowy: 2023-12-19 15:22:22.233625
#

cmake_minimum_required(VERSION 3.17)
project(malloc5 C)

# Przyjmij standard C11
set(CMAKE_C_STANDARD 11)

# Ustaw opcje kompilatora (z raportu Dante)
add_compile_options(
        "-Wextra"
        "-fmax-errors=5"
        "-Wno-parentheses"
        "-std=c11"
        "-D_GNU_SOURCE"
        "-pedantic"
        "-DINSIDE_DANTE"
        "-fdiagnostics-color"
        "-Wall"
        "-Werror=vla"
        "-D_ANSI_OUTPUT"
        "-Wno-error=unused-parameter"
        "-Wno-error=implicit-fallthrough"
        "-Werror"
        "-xc"
        "-ggdb3"
        "-Wno-error=parentheses"

)

# Ustaw opcje konsolidatora (z raportu Dante)
add_link_options(
)

# Utwórz target 'project1' i dodaj do niego pliki źródlowe:
add_executable(malloc5
        "heap.c"
        "main.c"
        "unit_helper_v2.c"
        "unit_test_v2.c"
        "rdebug.c"
        "memmanager.c"
        )

# Dołącz niezbędne biblioteki
target_link_libraries(malloc5
        )
