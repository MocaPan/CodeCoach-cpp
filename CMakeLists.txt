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
# Ruta exacta de tu instalación de OpenSSL (en Windows)
set(OPENSSL_ROOT_DIR "C:/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "C:/OpenSSL-Win64/include")
set(OPENSSL_CRYPTO_LIBRARY "C:/OpenSSL-Win64/lib/VC/x64/MD/libcrypto_static.lib")
set(OPENSSL_SSL_LIBRARY "C:/OpenSSL-Win64/lib/VC/x64/MD/libssl_static.lib")

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

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

target_include_directories(
        motor_evaluacion
        PUBLIC
        ${HEADER_LIB_DIR}
)

# --- Target 2: Analizador de Soluciones ---
# Este servicio se conecta a la API del LLM para dar feedback.
add_executable(
        analizador_soluciones
        "analizador_soluciones/analizador.cpp"
)

target_include_directories(
        analizador_soluciones
        PUBLIC
        ${HEADER_LIB_DIR}
)

target_link_libraries(
        analizador_soluciones
        PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
)

# --- Target 3: Gestor de Problemas ---
# Este servicio gestiona la base de datos de problemas.
add_executable(
        gestor_problemas
        "gestor_problemas/gestor.cpp"
)

target_include_directories(
        gestor_problemas
        PUBLIC
        ${HEADER_LIB_DIR}
        "${CMAKE_SOURCE_DIR}/gestor_problemas"
)

target_link_libraries(
        gestor_problemas
        PRIVATE
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
        OpenSSL::SSL
        OpenSSL::Crypto
)

# --- Target 4: Cargador de Problemas (Utilidad) ---
add_executable(
        load_sample_problems
        "utils/load_sample_problems.cpp"
)

target_include_directories(
        load_sample_problems
        PUBLIC
        ${HEADER_LIB_DIR}
)

target_link_libraries(
        load_sample_problems
        PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
)

# --- Target 5: Frontend Server ---
add_executable(
        frontend_server
        "frontend_server.cpp"
)

target_include_directories(
        frontend_server
        PUBLIC
        ${HEADER_LIB_DIR}
)

target_link_libraries(
        frontend_server
        PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
)


