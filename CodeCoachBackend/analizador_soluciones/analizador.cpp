/**
 * analizador_soluciones/analizador.cpp
 *
 * Servicio REST API en C++ para el "Analizador de Soluciones".
 *
 * Este servicio expone un endpoint (/analyze) que recibe el código de un usuario
 * y los resultados de una evaluación. Se conecta a un LLM (Google Gemini) para
 * obtener feedback y lo devuelve al cliente.
 *
 * Nota: Este archivo evita deliberadamente usar librerías de parsing JSON
 * (como nlohmann::json) y en su lugar utiliza parsing manual de strings.
 * Esto se hizo para resolver un conflicto de bajo nivel entre
 * OpenSSL y la librería JSON que causaba corrupción de memoria.
 */

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <iostream>
#include <string>
#include <stdlib.h> // Para std::getenv

 // --- Utilidades de String (Parsing/Building Manual) ---

 /**
  * Extrae un valor string de un objeto JSON simple.
  * Busca un patrón como: "key": "value"
  * @param json_str El string JSON completo.
  * @param key La clave a buscar.
  * @return El valor encontrado, o un string vacío si no se encuentra.
  */
std::string manual_json_parse(const std::string& json_str, const std::string& key) {
    std::string key_to_find = "\"" + key + "\": \"";
    size_t start = json_str.find(key_to_find);
    if (start == std::string::npos) {
        return ""; // No encontrado
    }
    start += key_to_find.length();
    size_t end = json_str.find("\"", start);
    if (end == std::string::npos) {
        return ""; // JSON mal formado
    }
    return json_str.substr(start, end - start);
}

/**
 * Escapa caracteres especiales en un string para que sea JSON-safe.
 * @param input El string de entrada.
 * @return Un nuevo string con los caracteres escapados.
 */
std::string escape_json_string(const std::string& input) {
    std::string output;
    output.reserve(input.length());
    for (char c : input) {
        switch (c) {
        case '"':  output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\n': output += "\\n";  break;
        case '\r': output += "\\r";  break;
        case '\t': output += "\\t";  break;
        default:   output += c;     break;
        }
    }
    return output;
}

// --- Lógica del Coach AI (Google Gemini) ---

/**
 * Obtiene la clave de API de Google desde una variable de entorno.
 * @return La clave de API, o un string vacío si no está configurada.
 */
std::string get_google_api_key() {
    const char* key = std::getenv("GOOGLE_API_KEY");
    if (key == nullptr) {
        std::cerr << "Error Crítico: Variable de entorno GOOGLE_API_KEY no configurada." << std::endl;
        return "";
    }
    return std::string(key);
}

/**
 * Se conecta a la API de Google Gemini para obtener feedback sobre el código.
 * @param user_code El código C++ del estudiante.
 * @param eval_results Los resultados de las pruebas (ej. "Prueba 3 fallida").
 * @return El feedback generado por el LLM.
 */
std::string get_llm_feedback(const std::string& user_code, const std::string& eval_results) {

    std::string api_key = get_google_api_key();
    if (api_key.empty()) {
        return "Error del servidor: La clave API de Google no está configurada.";
    }

    // Inicializa el cliente HTTPS para la API de Google
    httplib::Client cli("https://generativelanguage.googleapis.com");

    // --- Ingeniería de Prompts ---
    // Se define el "rol" del LLM y se le da el contexto.
    std::string system_prompt =
        "Eres un 'Code Coach' para un estudiante de programación. "
        "El estudiante te enviará su código y los resultados de las pruebas. "
        "Tu trabajo es dar una pista o explicar el error, pero NUNCA dar la solución completa. "
        "Mantén el reto. Sé breve y amigable.";

    std::string user_prompt =
        "Mi código:\n```cpp\n" + user_code + "\n```\n\n"
        "Resultados de las pruebas:\n" + eval_results + "\n\n"
        "Por favor, dame una pista.";

    std::string full_prompt = system_prompt + "\n\n" + user_prompt;

    // --- Construcción Manual del Payload JSON ---
    std::string payload =
        "{\"contents\":[{\"parts\":[{\"text\": \"" + escape_json_string(full_prompt) + "\"}]}]}";

    // Define el endpoint del modelo a usar (obtenido de ListModels)
    std::string url = "/v1beta/models/gemini-2.5-pro:generateContent?key=" + api_key;

    // Realiza la petición POST
    if (auto res = cli.Post(url, payload, "application/json")) {
        if (res->status == 200) {
            // Petición exitosa, parsea la respuesta manualmente
            std::string feedback = manual_json_parse(res->body, "text");
            if (feedback.empty()) {
                std::cerr << "Error parseando respuesta de Gemini:\n" << res->body << std::endl;
                return "Error: Respuesta inválida del Coach AI (no se encontró 'text').";
            }
            // Limpia los escapes \n que devuelve el JSON
            size_t pos = 0;
            while ((pos = feedback.find("\\n", pos)) != std::string::npos) {
                feedback.replace(pos, 2, "\n");
                pos += 1;
            }
            return feedback;
        }
        else {
            // La API de Google devolvió un error (4xx, 5xx)
            std::cerr << "Error de Google API: " << res->status << "\n" << res->body << std::endl;
            return "Error: No se pudo contactar al Coach AI. Estado: " + std::to_string(res->status);
        }
    }
    else {
        // Error de red (no se pudo conectar)
        auto err = res.error();
        std::cerr << "Error de red: " << httplib::to_string(err) << std::endl;
        return "Error: Falla de conexión con el Coach AI.";
    }
}

// --- Servidor Principal ---

int main() {
    httplib::Server svr;

    /**
     * Endpoint: POST /analyze
     * Recibe un JSON con "code" y "results".
     * Devuelve un JSON con el "feedback" del LLM.
     */
    svr.Post("/analyze", [](const httplib::Request& req, httplib::Response& res) {
        try {
            // Parseo manual para evitar conflictos de librería
            std::string user_code = manual_json_parse(req.body, "code");
            std::string results = manual_json_parse(req.body, "results");

            if (user_code.empty() || results.empty()) {
                res.status = 400; // Bad Request
                res.set_content("Error: JSON mal formado. Se esperan 'code' y 'results'.", "text/plain");
                return;
            }

            // Llama a la lógica principal del LLM
            std::string feedback = get_llm_feedback(user_code, results);

            // Construye la respuesta JSON manualmente
            std::string response_json =
                "{\"feedback\": \"" + escape_json_string(feedback) + "\"}";

            res.set_content(response_json, "application/json");

        }
        catch (const std::exception& e) {
            // Captura de seguridad para cualquier error inesperado
            res.status = 500; // Internal Server Error
            std::string err_msg = "Error interno del servidor: " + std::string(e.what());
            res.set_content(err_msg, "text/plain");
            std::cerr << err_msg << std::endl;
        }
        });

    std::cout << "Servidor Analizador de Soluciones iniciado en http://localhost:8081" << std::endl;
    svr.listen("0.0.0.0", 8081); // Escucha en el puerto 8081

    return 0;
}