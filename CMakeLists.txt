cmake_minimum_required(VERSION 3.10)
project(DiscreteSystems LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directorios de salida dentro de build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# ---- Librería estática de tus .cpp ----
file(GLOB LIB_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_library(DiscreteSystems STATIC ${LIB_SOURCES})
target_include_directories(DiscreteSystems PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ---- Librerías estáticas externas (.a) ----
file(GLOB ALL_STATIC_LIBS "${CMAKE_SOURCE_DIR}/lib/*.a")

foreach(LIBFILE ${ALL_STATIC_LIBS})
    get_filename_component(LIBNAME ${LIBFILE} NAME_WE)

    add_library(${LIBNAME} STATIC IMPORTED)
    set_target_properties(${LIBNAME} PROPERTIES
        IMPORTED_LOCATION ${LIBFILE}
    )
endforeach()

# ---- Buscar todos los .cpp de test/ ----
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/*.cpp")

# ---- Crear y enlazar ejecutables automáticamente ----
foreach(TEST_SRC ${TEST_SOURCES})
    # Extraer el nombre del ejecutable sin extensión
    get_filename_component(EXE_NAME ${TEST_SRC} NAME_WE)

    # Crear el ejecutable
    add_executable(${EXE_NAME} ${TEST_SRC})

    # Enlazar tu librería DiscreteSystems
    target_link_libraries(${EXE_NAME} PRIVATE DiscreteSystems)

    # Enlazar todas las librerías .a externas
    foreach(LIBFILE ${ALL_STATIC_LIBS})
        get_filename_component(LIBNAME ${LIBFILE} NAME_WE)
        target_link_libraries(${EXE_NAME} PRIVATE ${LIBNAME})
    endforeach()
endforeach()


