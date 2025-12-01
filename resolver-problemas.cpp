/**
 * load_sample_problems.cpp
 *
 * Programa utilitario para cargar problemas de ejemplo en el Gestor de Problemas.
 * Lee el archivo sample_problems.json y envía cada problema al API REST.
 *
 * Uso:
 *   ./load_sample_problems
 *
 * Requisitos:
 *   - El servidor gestor_problemas debe estar corriendo en localhost:8080
 *   - El archivo sample_problems.json debe estar en el mismo directorio
 */

#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

// Configuración
const std::string API_URL = "localhost";
const int API_PORT = 8080;
const std::string SAMPLE_FILE = "sample_problems.json";

/**
 * Lee el contenido completo de un archivo y lo retorna como string
 */
std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Verifica que el servidor esté corriendo y la base de datos conectada
 */
bool check_server_health(httplib::Client& client) {
    std::cout << "1. Verificando servidor..." << std::endl;

    auto res = client.Get("/health");

    if (!res) {
        std::cerr << "   ✗ No se pudo conectar al servidor" << std::endl;
        std::cerr << "   Error: " << httplib::to_string(res.error()) << std::endl;
        return false;
    }

    if (res->status != 200) {
        std::cerr << "   ✗ El servidor respondió con código " << res->status << std::endl;
        return false;
    }

    try {
        json health_data = json::parse(res->body);
        std::string status = health_data.value("status", "");
        std::string database = health_data.value("database", "");

        if (status == "OK" && database == "connected") {
            std::cout << "   ✓ Servidor OK - Base de datos conectada" << std::endl;
            return true;
        }
        else {
            std::cerr << "   ✗ Estado del servidor: " << status << std::endl;
            std::cerr << "   ✗ Estado de la BD: " << database << std::endl;
            return false;
        }
    }
    catch (const json::exception& e) {
        std::cerr << "   ✗ Error al parsear respuesta de salud: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Carga los problemas desde el archivo JSON
 */
json load_problems_from_file(const std::string& filename) {
    std::cout << "\n2. Cargando problemas del archivo..." << std::endl;

    try {
        std::string file_content = read_file(filename);
        json problems = json::parse(file_content);

        if (!problems.is_array()) {
            throw std::runtime_error("El archivo JSON debe contener un array de problemas");
        }

        std::cout << "   ✓ Archivo '" << filename << "' cargado correctamente ("
            << problems.size() << " problemas)" << std::endl;

        return problems;
    }
    catch (const std::exception& e) {
        std::cerr << "   ✗ Error al cargar archivo: " << e.what() << std::endl;
        throw;
    }
}

/**
 * Sube un problema individual al servidor
 */
bool upload_problem(httplib::Client& client, const json& problem) {
    try {
        std::string problem_id = problem.value("id", "unknown");
        std::string problem_title = problem.value("title", "Sin título");

        // Convertir el problema a string JSON
        std::string problem_json = problem.dump();

        // Enviar POST request
        auto res = client.Post("/problems", problem_json, "application/json");

        if (!res) {
            std::cerr << "  ✗ " << problem_id << ": Error de conexión - "
                << httplib::to_string(res.error()) << std::endl;
            return false;
        }

        if (res->status == 201) {
            std::cout << "  ✓ " << problem_id << ": " << problem_title << std::endl;
            return true;
        }
        else if (res->status == 400) {
            json error_response = json::parse(res->body);
            std::string error_msg = error_response.value("error", "Error desconocido");
            std::cerr << "  ✗ " << problem_id << ": Error 400 - " << error_msg << std::endl;
            return false;
        }
        else {
            std::cerr << "  ✗ " << problem_id << ": Error " << res->status << std::endl;
            std::cerr << "    Respuesta: " << res->body << std::endl;
            return false;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "  ✗ Error al subir problema: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Imprime un separador visual
 */
void print_separator(char c = '=', int length = 60) {
    std::cout << std::string(length, c) << std::endl;
}

int main() {
    print_separator();
    std::cout << "CARGADOR DE PROBLEMAS DE EJEMPLO - CODECOACH" << std::endl;
    print_separator();
    std::cout << std::endl;

    try {
        // Crear cliente HTTP
        httplib::Client client(API_URL, API_PORT);
        client.set_connection_timeout(5);  // 5 segundos de timeout

        // 1. Verificar que el servidor esté corriendo
        if (!check_server_health(client)) {
            std::cout << "\n  El servidor no está disponible. Asegúrate de que:" << std::endl;
            std::cout << "   - El gestor_problemas esté corriendo" << std::endl;
            std::cout << "   - MongoDB esté activo" << std::endl;
            std::cout << "   - El servidor esté en http://" << API_URL << ":" << API_PORT << std::endl;
            return 1;
        }

        // 2. Cargar problemas del archivo
        json problems = load_problems_from_file(SAMPLE_FILE);

        // 3. Subir problemas al servidor
        std::cout << "\n3. Subiendo problemas al servidor..." << std::endl;

        int successful = 0;
        int failed = 0;

        for (const auto& problem : problems) {
            if (upload_problem(client, problem)) {
                successful++;
            }
            else {
                failed++;
            }
        }

        // 4. Mostrar resumen
        std::cout << std::endl;
        print_separator();
        std::cout << "RESUMEN:" << std::endl;
        std::cout << "  ✓ Exitosos: " << successful << "/" << problems.size() << std::endl;
        std::cout << "  ✗ Fallidos:  " << failed << "/" << problems.size() << std::endl;
        print_separator();

        if (failed == 0) {
            std::cout << "\n ¡Todos los problemas se cargaron correctamente!" << std::endl;
            return 0;
        }
        else {
            std::cout << "\n  " << failed << " problema(s) no se pudieron cargar" << std::endl;
            return 1;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "\n Error fatal: " << e.what() << std::endl;
        return 1;
    }
}