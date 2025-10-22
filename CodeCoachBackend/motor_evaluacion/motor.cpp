// motor_evaluacion/motor.cpp
#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <cstdio>
#include <array>
#include <chrono>

using json = nlohmann::json;

// Función para ejecutar un comando del sistema y capturar su salida (stdout)
// Esta es la base del "sandbox" simple.
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    // Usamos _popen en Windows (popen en Linux/macOS)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("_popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Estructura para el resultado de la evaluación
struct EvaluationResult {
    bool compiled = false;
    json test_results = json::array(); // Array de resultados
    std::string compile_error;
    long long execution_time_ms = 0;
};

// Función principal del motor de evaluación [cite: 32, 33]
EvaluationResult evaluate_code(const std::string& user_code, const json& test_cases) {
    EvaluationResult eval;

    // 1. Guardar el código en un archivo temporal
    std::ofstream temp_file("temp_solution.cpp");
    temp_file << user_code;
    temp_file.close();

    // 2. Compilar el código (¡capturando stderr!)
    //    Usamos 2>&1 para redirigir stderr a stdout y poder capturarlo
    std::string compile_output = exec("g++ temp_solution.cpp -o temp_solution.exe 2>&1");

    if (!compile_output.empty()) {
        eval.compiled = false;
        eval.compile_error = compile_output;
        // Limpiar archivos
        remove("temp_solution.cpp");
        return eval;
    }
    eval.compiled = true;

    // 3. Ejecutar pruebas predefinidas [cite: 32]
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& test_case : test_cases) {
        std::string input = test_case.at("input");
        std::string expected_output = test_case.at("expected");

        // Crear el comando para ejecutar. Usamos 'echo' para pasar el input por stdin.
        // NOTA: Esto es muy simple, solo funciona para una línea de input.
        // Para inputs más complejos, se necesita una técnica más avanzada (ej. archivos)
        std::string command = "echo " + input + " | temp_solution.exe";

        std::string actual_output = exec(command.c_str());

        // Limpiar saltos de línea del output
        if (!actual_output.empty() && actual_output.back() == '\n') {
            actual_output.pop_back();
        }

        bool passed = (actual_output == expected_output);
        eval.test_results.push_back({
            {"input", input},
            {"expected", expected_output},
            {"actual", actual_output},
            {"passed", passed}
            });
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    eval.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 4. Limpiar archivos
    remove("temp_solution.cpp");
    remove("temp_solution.exe");

    return eval;
}


int main() {
    httplib::Server svr;

    // Definimos el endpoint /evaluate
    svr.Post("/evaluate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::string user_code = body.at("code");
            json test_cases = body.at("test_cases"); // Espera un array de {input, expected}

            EvaluationResult result = evaluate_code(user_code, test_cases);

            json response_json = {
                {"compiled", result.compiled},
                {"compile_error", result.compile_error},
                {"test_results", result.test_results},
                {"execution_time_ms", result.execution_time_ms}
            };

            res.set_content(response_json.dump(), "application/json");

        }
        catch (json::parse_error& e) {
            res.status = 400; // Bad Request
            res.set_content("Error: JSON mal formado.", "text/plain");
        }
        });

    std::cout << "Servidor Motor de Evaluacion iniciado en http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080); // Puerto 8080
    return 0;
}