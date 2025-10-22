cmake_minimum_required(VERSION 3.15)
project(CodeCoachBackend LANGUAGES CXX)


# --- Configuración del Proyecto ---
# Necesitamos C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Encontrar Librerías ---

# 1. Encontrar OpenSSL (¡Necesario para httplib modo CLIENTE para llamar a OpenAI!)
#    httplib necesita esto para hacer peticiones HTTPS.
set(OPENSSL_ROOT_DIR "Y:/OpenSSL/OpenSSL-Win64") # ¡La ruta de tu imagen!
find_package(OpenSSL REQUIRED)

# 2. Definir nuestras librerías "header-only" (httplib y json.hpp)
#    Le decimos a CMake dónde están los encabezados.
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")

# --- Definir los Ejecutables (Tus 2 APIs) ---

# 1. MOTOR DE EVALUACIÓN
add_executable(
    motor_evaluacion
    "motor_evaluacion/motor.cpp"
)
# Le decimos al motor dónde encontrar httplib.h y json.hpp
target_include_directories(
    motor_evaluacion 
    PUBLIC ${LIB_DIR}
)

# 2. ANALIZADOR DE SOLUCIONES
add_executable(
    analizador_soluciones
    "analizador_soluciones/analizador.cpp"
)
# Le decimos al analizador dónde encontrar httplib.h y (si lo tuviera) json.hpp
target_include_directories(
    analizador_soluciones 
    PUBLIC ${LIB_DIR}              # <-- ¡¡ESTA ES LA VERSIÓN CORRECTA!!
)
# El analizador necesita vincular OpenSSL para llamar a la API de OpenAI (HTTPS)
target_link_libraries(
    analizador_soluciones 
    PRIVATE OpenSSL::SSL OpenSSL::Crypto
)