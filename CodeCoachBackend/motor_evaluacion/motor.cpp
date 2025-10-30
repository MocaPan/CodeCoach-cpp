/**
 * motor_evaluacion/motor.cpp
 *
 * Servicio REST API en C++ que recibe código C++ y casos de prueba,
 * compila ese código, lo ejecuta contra cada caso y devuelve los resultados.
 *
 * Los comentarios están escritos para que una persona sin experiencia
 * en programación entienda qué hace cada bloque y las líneas críticas.
 */

#include "httplib.h"    // Librería mínima para crear servidor HTTP y manejar peticiones
#include "json.hpp"     // Librería nlohmann::json para parsear y construir JSON
#include <iostream>     // Para imprimir mensajes en la consola (errores / logs)
#include <fstream>      // Para leer y escribir archivos
#include <string>       // Para usar std::string
#include <memory>       // Para std::unique_ptr (gestión automática de memoria)
#include <cstdio>       // Para _popen, _pclose y remove (ejecutar comandos y borrar archivos)
#include <array>        // Para std::array (buffer de lectura)
#include <chrono>       // Para medir tiempo (duración de ejecución)
#include <sstream>      // Para construir string desde un stream

// Alias corto para usar la librería JSON de forma cómoda
using json = nlohmann::json;

/**
 * Ejecuta un comando del sistema y captura su salida (texto que el comando escribe).
 *
 * Explicación simple:
 * - Imagina que ejecutas "g++ archivo.cpp -o programa" en una terminal.
 * - Esta función ejecuta ese comando y recoge todo el texto que produce
 *   (tanto la salida normal como los mensajes de error).
 * - Devuelve ese texto como un std::string.
 *
 * Parámetros:
 * - cmd: texto con el comando a ejecutar (ej. "g++ temp_solution.cpp -o temp_solution.exe")
 *
 * Retorno:
 * - Un string con todo lo que el comando imprimió.
 */
std::string exec(const char* cmd) {
    std::array<char, 128> buffer; // Espacio temporal para leer bytes devolvidos por el comando
    std::string result;           // Aquí acumulamos la salida completa

    // Construimos un proceso para ejecutar el comando y leer su salida.
    // "2>&1" significa: enviar también los errores al mismo flujo de texto
    // para que podamos leerlos junto con la salida normal.
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(
        _popen((std::string(cmd) + " 2>&1").c_str(), "r"),
        _pclose
    );

    // Si no se pudo crear el proceso, lanzamos un error.
    if (!pipe) {
        throw std::runtime_error("_popen() failed!");
    }

    // Leemos en bloques el texto que genera el comando hasta que termine.
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data(); // Concatenamos cada bloque al resultado final
    }

    // Devolvemos todo el texto recogido
    return result;
}

/**
 * Lee el contenido completo de un archivo y devuelve ese contenido como string.
 *
 * Explicación simple:
 * - Abre el archivo indicado y copia todo su contenido en una cadena.
 * - Elimina saltos de línea al final para que la comparación con la salida esperada
 *   sea más directa.
 *
 * Parámetros:
 * - filename: nombre del archivo que queremos leer (ej. "temp_output.txt")
 *
 * Retorno:
 * - El texto del archivo. Si no se pudo abrir, devuelve un mensaje de error.
 */
std::string read_file_content(const std::string& filename) {
    std::ifstream file(filename); // Intentamos abrir el archivo para lectura

    // Si no se pudo abrir (p. ej. porque el programa crasheó y no creó el archivo),
    // devolvemos un mensaje que indica ese problema.
    if (!file.is_open()) {
        return "[Error: No se pudo abrir el archivo de salida. ¿El programa crasheó?]";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();           // Leemos todo el contenido del archivo al buffer
    std::string content = buffer.str(); // Convertimos el buffer a string

    // Eliminamos saltos de línea al final para normalizar la salida
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
        content.pop_back();
    }

    return content; // Devolvemos el texto limpio
}

/**
 * Estructura que guarda todos los resultados de compilar y ejecutar las pruebas.
 *
 * Campos:
 * - compiled: indica si la compilación fue exitosa
 * - compile_error: texto con los errores de compilación (si los hay)
 * - test_results: array JSON con los resultados de cada caso de prueba
 * - total_execution_time_ms: tiempo total en milisegundos que tomó ejecutar todas las pruebas
 */
struct EvaluationResult {
    bool compiled = false;
    std::string compile_error;
    json test_results = json::array();
    long long total_execution_time_ms = 0;
};

/**
 * Función principal que compila y ejecuta el código del usuario contra casos de prueba.
 *
 * Flujo general:
 * 1) Guarda el código recibido en un archivo temporal.
 * 2) Compila ese archivo con g++.
 * 3) Si la compilación falla, devuelve el error.
 * 4) Si compila, ejecuta el programa para cada caso de prueba:
 *    - Escribe el input en un archivo temporal.
 *    - Ejecuta el programa redirigiendo ese archivo al stdin y guardando stdout en otro archivo.
 *    - Lee la salida generada y la compara con la salida esperada.
 * 5) Calcula el tiempo total y limpia archivos temporales.
 *
 * Parámetros:
 * - user_code: el código fuente en C++ enviado por el usuario (como string)
 * - test_cases: objeto JSON que contiene un array de casos { "input": "...", "expected": "..." }
 *
 * Retorno:
 * - Un EvaluationResult con todos los datos de la ejecución.
 */
EvaluationResult evaluate_code(const std::string& user_code, const json& test_cases) {
    EvaluationResult eval; // Donde almacenaremos resultados finales

    // Nombres de archivos temporales que usamos en todo el proceso
    const std::string code_file = "temp_solution.cpp";   // archivo que contendrá el código
    const std::string exe_file = "temp_solution.exe";    // ejecutable resultante
    const std::string input_file = "temp_input.txt";     // archivo usado como stdin
    const std::string output_file = "temp_output.txt";   // archivo donde redirigimos stdout

    // 1. Guardar el código en un archivo temporal
    std::ofstream temp_file(code_file); // Abrimos (o creamos) el archivo para escribir
    temp_file << user_code;             // Escribimos el código dentro del archivo
    temp_file.close();                  // Cerramos el archivo para asegurar que se haya guardado

    // 2. Compilar el código con g++
    std::string compile_output = exec(("g++ " + code_file + " -o " + exe_file).c_str());

    // Si hubo cualquier texto en la salida de compilación, lo tratamos como error.
    // (g++ escribe errores en la salida)
    if (!compile_output.empty()) {
        eval.compiled = false;         // Indicamos que no compiló correctamente
        eval.compile_error = compile_output; // Guardamos el mensaje de error
        remove(code_file.c_str());     // Limpiamos el archivo de código temporal
        return eval;                   // Devolvemos inmediatamente el resultado con error
    }

    eval.compiled = true; // Si llegamos aquí, la compilación fue exitosa

    // 3. Ejecutar las pruebas y medir el tiempo total
    auto start_time = std::chrono::high_resolution_clock::now(); // Marca de tiempo inicial
    int test_num = 1; // Contador legible para numerar los tests en la salida

    // Recorremos cada caso de prueba proporcionado en el JSON
    for (const auto& test_case : test_cases) {
        // Extraemos el input y la salida esperada del objeto JSON del caso
        std::string input = test_case.at("input");
        std::string expected_output = test_case.at("expected");

        // a) Escribir el input en el archivo temporal que usaremos como stdin
        std::ofstream temp_input(input_file);
        temp_input << input;
        temp_input.close();

        // b) Ejecutar el programa redirigiendo stdin desde input_file y stdout a output_file
        //    Equivalente en la terminal: temp_solution.exe < temp_input.txt > temp_output.txt
        std::string command = exe_file + " < " + input_file + " > " + output_file;
        system(command.c_str()); // system ejecuta el comando en el sistema operativo

        // c) Leer el contenido que el programa escribió en output_file
        std::string actual_output = read_file_content(output_file);

        // Comparación directa: aceptamos solo si el texto coincide exactamente
        bool passed = (actual_output == expected_output);

        // Guardamos en el array de resultados los detalles de este caso
        eval.test_results.push_back({
            {"test_case", test_num},
            {"input", input},
            {"expected", expected_output},
            {"actual", actual_output},
            {"passed", passed}
        });

        test_num++; // Incrementamos el número del test
    }

    // Medimos el tiempo final y calculamos la diferencia con la marca inicial
    auto end_time = std::chrono::high_resolution_clock::now();
    eval.total_execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 4. Limpiar todos los archivos temporales que creamos
    remove(code_file.c_str());
    remove(exe_file.c_str());
    remove(input_file.c_str());
    remove(output_file.c_str());

    // Devolvemos todos los resultados recogidos
    return eval;
}

// --- Servidor Principal: define endpoints y arranca el servicio ---

int main() {
    httplib::Server svr; // Creamos un servidor HTTP que escuchará peticiones

    /**
     * Endpoint: POST /evaluate
     *
     * Explicación simple:
     * - Un cliente envía una petición POST a /evaluate con un JSON que contiene:
     *   { "code": "<codigo C++>", "test_cases": [ { "input": "...", "expected": "..." }, ... ] }
     * - El servidor compila y ejecuta el código contra cada caso y devuelve un JSON con resultados.
     */
    svr.Post("/evaluate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            // Intentamos parsear el cuerpo de la petición como JSON
            // Si el cliente envía algo que no es JSON válido, esto lanzará una excepción.
            json body = json::parse(req.body);

            // Extraemos el código del usuario y la lista de casos de prueba del JSON
            std::string user_code = body.at("code");
            json test_cases = body.at("test_cases");

            // Llamamos a la función que compila y ejecuta los tests
            EvaluationResult result = evaluate_code(user_code, test_cases);

            // Construimos la respuesta JSON que enviaremos al cliente con todos los detalles
            json response_json = {
                {"compiled", result.compiled},
                {"compile_error", result.compile_error},
                {"test_results", result.test_results},
                {"total_execution_time_ms", result.total_execution_time_ms}
            };

            // Enviamos la respuesta al cliente como JSON formateado (indentado para legibilidad)
            res.set_content(response_json.dump(4), "application/json");
        }
        catch (const json::parse_error& e) {
            // Si el JSON de entrada es inválido respondemos con 400 Bad Request
            res.status = 400; // Código HTTP que indica que la petición es incorrecta
            std::string err_msg = "Error: JSON mal formado. " + std::string(e.what());
            res.set_content(err_msg, "text/plain"); // Devolvemos el mensaje como texto simple
            std::cerr << err_msg << std::endl;      // Además registramos el error en consola
        }
        catch (const std::exception& e) {
            // Capturamos cualquier otro error inesperado y devolvemos 500 Internal Server Error
            res.status = 500;
            std::string err_msg = "Error interno del servidor: " + std::string(e.what());
            res.set_content(err_msg, "text/plain");
            std::cerr << err_msg << std::endl; // Registramos el error en consola para depuración
        }
    });

    // Mensaje informativo en la consola y arrancamos la escucha en todas las interfaces en el puerto 8080
    std::cout << "Servidor Motor de Evaluacion iniciado en http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0; // Fin del programa
}