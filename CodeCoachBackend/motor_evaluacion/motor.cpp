/**
 * motor_evaluacion/motor.cpp
 *
 * Servicio REST API en C++ para el "Motor de Evaluación".
 *
 * Expone el endpoint /evaluate que recibe código C++ y una lista de
 * casos de prueba. Compila el código en un "sandbox" simple, lo ejecuta
 * contra cada prueba, mide el tiempo y devuelve los resultados.
 */

#include "httplib.h"
#include "json.hpp" // Este servicio SÍ usa la librería JSON
#include <iostream>
#include <fstream>
#include <string>
#include <memory>   // Para std::unique_ptr
#include <cstdio>   // Para _popen, _pclose, remove
#include <array>    // Para std::array
#include <chrono>   // Para medir el tiempo
#include <sstream>  // Para std::stringstream

 // Se usa el alias 'json' para la librería nlohmann::json
using json = nlohmann::json;

/**
 * Ejecuta un comando del sistema y captura su salida (stdout + stderr).
 * @param cmd El comando a ejecutar (ej. "g++ ...").
 * @return El string de la salida del comando.
 */
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    // Se usa 2>&1 para redirigir stderr a stdout y poder capturar errores de compilación
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen((std::string(cmd) + " 2>&1").c_str(), "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("_popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

/**
 * Lee el contenido completo de un archivo y lo limpia.
 * @param filename El nombre del archivo a leer (ej. "temp_output.txt").
 * @return El contenido del archivo como un string, sin saltos de línea al final.
 */
std::string read_file_content(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Devuelve un error específico si el archivo de salida no se creó
        return "[Error: No se pudo abrir el archivo de salida. ¿El programa crasheó?]";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Limpieza: Elimina saltos de línea (CR/LF) al final
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
        content.pop_back();
    }
    return content;
}

/**
 * Define la estructura de los resultados de la evaluación.
 */
struct EvaluationResult {
    bool compiled = false;
    std::string compile_error;
    json test_results = json::array();
    long long total_execution_time_ms = 0;
};

/**
 * Lógica principal: compila y ejecuta el código del usuario contra los casos de prueba.
 * @param user_code El string del código C++ enviado por el usuario.
 * @param test_cases El objeto JSON que contiene el array de casos de prueba.
 * @return Una struct EvaluationResult con todos los resultados.
 */
EvaluationResult evaluate_code(const std::string& user_code, const json& test_cases) {
    EvaluationResult eval;
    const std::string code_file = "temp_solution.cpp";
    const std::string exe_file = "temp_solution.exe";
    const std::string input_file = "temp_input.txt";
    const std::string output_file = "temp_output.txt";

    // 1. Guardar el código en un archivo temporal
    std::ofstream temp_file(code_file);
    temp_file << user_code;
    temp_file.close();

    // 2. Compilar el código
    std::string compile_output = exec(("g++ " + code_file + " -o " + exe_file).c_str());

    if (!compile_output.empty()) {
        eval.compiled = false;
        eval.compile_error = compile_output;
        remove(code_file.c_str()); // Limpiar
        return eval;
    }

    eval.compiled = true;

    // 3. Ejecutar pruebas predefinidas
    auto start_time = std::chrono::high_resolution_clock::now();
    int test_num = 1;

    for (const auto& test_case : test_cases) {
        std::string input = test_case.at("input");
        std::string expected_output = test_case.at("expected");

        // a. Escribir el input en un archivo temporal
        std::ofstream temp_input(input_file);
        temp_input << input;
        temp_input.close();

        // b. Ejecutar el programa redirigiendo stdin y stdout
        std::string command = exe_file + " < " + input_file + " > " + output_file;
        system(command.c_str());

        // c. Leer el resultado del archivo de salida
        std::string actual_output = read_file_content(output_file);

        bool passed = (actual_output == expected_output);

        eval.test_results.push_back({
            {"test_case", test_num},
            {"input", input},
            {"expected", expected_output},
            {"actual", actual_output},
            {"passed", passed}
            });
        test_num++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    eval.total_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 4. Limpiar todos los archivos temporales
    remove(code_file.c_str());
    remove(exe_file.c_str());
    remove(input_file.c_str());
    remove(output_file.c_str());

    return eval;
}

// --- Servidor Principal ---

int main() {
    httplib::Server svr;

    /**
     * Endpoint: POST /evaluate
     * Recibe un JSON con "code" (string) y "test_cases" (array).
     * Devuelve un JSON con los resultados de la compilación y las pruebas.
     */
    svr.Post("/evaluate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            // Parsear el JSON de la petición
            json body = json::parse(req.body);
            std::string user_code = body.at("code");
            json test_cases = body.at("test_cases");

            // Ejecutar la lógica de evaluación
            EvaluationResult result = evaluate_code(user_code, test_cases);

            // Construir la respuesta JSON
            json response_json = {
                {"compiled", result.compiled},
                {"compile_error", result.compile_error},
                {"test_results", result.test_results},
                {"total_execution_time_ms", result.total_execution_time_ms}
            };

            // Enviar la respuesta formateada (con 4 espacios de indentación)
            res.set_content(response_json.dump(4), "application/json");

        }
        catch (const json::parse_error& e) {
            // Error si el JSON de entrada está mal formado
            res.status = 400; // Bad Request
            std::string err_msg = "Error: JSON mal formado. " + std::string(e.what());
            res.set_content(err_msg, "text/plain");
            std::cerr << err_msg << std::endl;

        }
        catch (const std::exception& e) {
            // Captura de seguridad para cualquier otro error (ej. _popen falló)
            res.status = 500; // Internal Server Error
            std::string err_msg = "Error interno del servidor: " + std::string(e.what());
            res.set_content(err_msg, "text/plain");
            std::cerr << err_msg << std::endl;
        }
        });

    // Iniciar el servidor en el puerto 8080
    std::cout << "Servidor Motor de Evaluacion iniciado en http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}