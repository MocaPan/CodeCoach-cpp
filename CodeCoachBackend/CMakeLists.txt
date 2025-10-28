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