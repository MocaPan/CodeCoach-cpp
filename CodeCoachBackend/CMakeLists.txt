# Establece la versión mínima de CMake requerida.
cmake_minimum_required(VERSION 3.15)

# Define el nombre del proyecto y el lenguaje principal.
project(CodeCoachBackend LANGUAGES CXX)

# ===================================================================
# == CONFIGURACIÓN GLOBAL DEL PROYECTO
# ===================================================================

# Establece el estándar de C++ 20 como requerido para todo el proyecto.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===================================================================
# == DEFINICIÓN DE LIBRERÍAS
# ===================================================================

# --- Librerías "Header-Only" ---
# Define la ubicación de nuestras librerías de solo encabezado (httplib, json).
set(HEADER_LIB_DIR "${CMAKE_SOURCE_DIR}/lib")

# --- Librerías Externas (OpenSSL) ---
# httplib necesita OpenSSL para realizar llamadas HTTPS (al Analizador de Soluciones).
# Se especifica la ruta de instalación de OpenSSL en Windows.
set(OPENSSL_ROOT_DIR "Y:/OpenSSL/OpenSSL-Win64")
find_package(OpenSSL REQUIRED)

# --- MongoDB C++ Driver ---
# Buscar la instalación de mongocxx
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

# ===================================================================
# == DEFINICIÓN DE TARGETS (EJECUTABLES)
# ===================================================================

# --- Target 1: Motor de Evaluación ---
# Este servicio compila y ejecuta el código del usuario.
add_executable(
    motor_evaluacion
    "motor_evaluacion/motor.cpp"
)

# Enlaza las librerías necesarias para el motor.
target_include_directories(
    motor_evaluacion
    PUBLIC
        ${HEADER_LIB_DIR} # Para httplib.h y json.hpp
)

# --- Target 2: Analizador de Soluciones ---
# Este servicio se conecta a la API del LLM para dar feedback.
add_executable(
    analizador_soluciones
    "analizador_soluciones/analizador.cpp"
)

# Enlaza las librerías necesarias para el analizador.
target_include_directories(
    analizador_soluciones
    PUBLIC
        ${HEADER_LIB_DIR} # Para httplib.h
)

target_link_libraries(
    analizador_soluciones
    PRIVATE
        OpenSSL::SSL      # Requerido para llamadas HTTPS
        OpenSSL::Crypto   # Requerido para llamadas HTTPS
)

# --- Target 3: Gestor de Problemas ---
# Este servicio gestiona la base de datos de problemas.
add_executable(
    gestor_problemas
    "gestor_problemas/gestor.cpp"
)

# Enlaza las librerías necesarias para el gestor.
target_include_directories(
    gestor_problemas
    PUBLIC
        ${HEADER_LIB_DIR}                    # Para httplib.h y json.hpp
        "${CMAKE_SOURCE_DIR}/gestor_problemas" # Para database.h y models.h
)

target_link_libraries(
    gestor_problemas
    PRIVATE
        mongo::mongocxx_shared  # MongoDB C++ driver
        mongo::bsoncxx_shared   # BSON library
)

# --- Target 4: Cargador de Problemas (Utilidad) ---
# Programa utilitario para cargar problemas de ejemplo.
add_executable(
    load_sample_problems
    "utils/load_sample_problems.cpp"
)

# Enlaza las librerías necesarias para el cargador.
target_include_directories(
    load_sample_problems
    PUBLIC
        ${HEADER_LIB_DIR} # Para httplib.h y json.hpp
)

