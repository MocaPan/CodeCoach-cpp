/**
 * analizador_soluciones/analizador.cpp
 *
 * Servicio REST API en C++ para el "Analizador de Soluciones".
 *
 * Este servicio expone un endpoint (/analyze) que recibe el código de un usuario
 * y los resultados de una evaluación. Se conecta a un LLM (Google Gemini) para
 * obtener feedback, análisis de complejidad y tipo de algoritmo, y lo devuelve al cliente.
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
 * Función de parsing manual MEJORADA para extraer el JSON anidado de la respuesta de Gemini.
 * Busca {"text": "..."} y extrae el contenido de "text", manejando comillas escapadas.
 * @param gemini_body La respuesta JSON completa de la API de Gemini.
 * @return El string JSON interno (que contiene nuestro feedback), o un string vacío si falla.
 */
std::string extract_json_text_from_gemini_response(const std::string& gemini_body) {
    const std::string key_to_find = "\"text\": \"";
    size_t start = gemini_body.find(key_to_find);
    if (start == std::string::npos) {
        return ""; // "text": " no encontrado
    }
    start += key_to_find.length();

    size_t end = start;
    int backslashes = 0;
    while (end < gemini_body.length()) {
        if (gemini_body[end] == '\\') {
            backslashes++;
        }
        else if (gemini_body[end] == '"') {
            if (backslashes % 2 == 0) {
                // Esta comilla NO está escapada (precedida por 0, 2, 4... backslashes)
                // Es el final de nuestro string.
                break;
            }
            // La comilla SÍ está escapada (precedida por 1, 3, 5... backslashes)
            backslashes = 0; // Reiniciar contador
        }
        else {
            // Cualquier otro caracter
            backslashes = 0; // Reiniciar contador
        }
        end++;
    }

    if (end == gemini_body.length()) {
        return ""; // Se llegó al final sin encontrar la comilla de cierre
    }

    return gemini_body.substr(start, end - start);
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

// Construye la petición al LLM (Gemini), la envía y devuelve el JSON de feedback que genera el LLM.
// - user_code: código fuente enviado por el estudiante
// - eval_results: texto con los resultados de las pruebas (fallos, aciertos, etc.)
// Retorna un string que contiene un objeto JSON con campos: feedback, analysis_algorithm, analysis_complexity_time, analysis_complexity_space.
std::string get_llm_feedback(const std::string& user_code, const std::string& eval_results) {

    // Obtiene la clave de la variable de entorno
    std::string api_key = get_google_api_key();
    if (api_key.empty()) {
        // Si no hay clave, devolvemos un JSON indicando el error para el cliente.
        return "{\"feedback\": \"Error del servidor: La clave API de Google no está configurada.\", \"analysis_algorithm\": \"Error\", \"analysis_complexity_time\": \"Error\", \"analysis_complexity_space\": \"Error\"}";
    }

    // Crea un cliente HTTP apuntando al host de la API de Google.
    // httplib::Client se encarga de HTTPS cuando se define CPPHTTPLIB_OPENSSL_SUPPORT.
    httplib::Client cli("https://generativelanguage.googleapis.com");

    // PROMPTS:
    // system_prompt: indica al LLM el "rol" y el formato estricto de salida (solo JSON).
    std::string system_prompt =
        "Eres un 'Code Coach' experto en C++. Tu trabajo es analizar el código de un estudiante y los resultados de sus pruebas. "
        "NUNCA debes dar la solución completa; solo pistas, explicaciones de errores y sugerencias de mejora. Mantén el reto. "
        "Responde ÚNICAMENTE con un objeto JSON (sin ```json ni ningún otro texto introductorio). "
        "El JSON debe tener la siguiente estructura exacta: "
        "{"
        "  \"feedback\": \"Tu pista o explicación breve y amigable aquí.\","
        "  \"analysis_algorithm\": \"El nombre del algoritmo o patrón que identificaste (ej. 'Búsqueda Lineal', 'Programación Dinámica', 'N/A' si no es claro).\", "
        "  \"analysis_complexity_time\": \"Tu estimación de la complejidad de tiempo (ej. 'O(n)', 'O(n^2)').\", "
        "  \"analysis_complexity_space\": \"Tu estimación de la complejidad de espacio (ej. 'O(1)', 'O(n)').\""
        "}";

    // user_prompt: contiene el código del alumno y los resultados de las pruebas.
    std::string user_prompt =
        "Mi código:\n```cpp\n" + user_code + "\n```\n\n"
        "Resultados de las pruebas:\n" + eval_results + "\n\n"
        "Por favor, dame tu análisis en el formato JSON solicitado.";

    // full_prompt junta el rol y el contenido que el LLM debe analizar.
    std::string full_prompt = system_prompt + "\n\n" + user_prompt;

    // Construimos manualmente el payload JSON que requiere la API de Gemini.
    // Aquí ponemos el prompt dentro de "contents" -> "parts" -> "text".
    std::string payload =
        "{\"contents\":[{\"parts\":[{\"text\": \"" + escape_json_string(full_prompt) + "\"}]}]}";

    // Endpoint del modelo (incluye la clave de API en la URL de consulta)
    std::string url = "/v1beta/models/gemini-2.5-pro:generateContent?key=" + api_key;

    // Enviamos POST con el payload; la librería devuelve un objeto opcional con la respuesta.
    if (auto res = cli.Post(url, payload, "application/json")) {
        if (res->status == 200) {
            // Respuesta exitosa: el cuerpo contiene JSON con la salida del modelo.
            // Gemini coloca el texto generado en una propiedad "text" que, a su vez,
            // es una cadena que contiene nuestro JSON de feedback. Extraemos esa cadena.
            std::string feedback_json_string = extract_json_text_from_gemini_response(res->body);

            if (feedback_json_string.empty()) {
                // Si no encontramos la propiedad "text" o estaba mal formada, log y error.
                std::cerr << "Error parseando respuesta de Gemini (no se encontró 'text' o estaba mal formado):\n" << res->body << std::endl;
                return "{\"feedback\": \"Error: Respuesta inválida del Coach AI (JSON anidado no encontrado).\", \"analysis_algorithm\": \"Error\", \"analysis_complexity_time\": \"Error\", \"analysis_complexity_space\": \"Error\"}";
            }

            // --- INICIO DE LA LÓGICA DE LIMPIEZA ---

            // Gemini devuelve la cadena con escapes (\n, \", \\). Aquí los convertimos a sus caracteres reales.
            size_t pos = 0;
            while ((pos = feedback_json_string.find("\\n", pos)) != std::string::npos) {
                feedback_json_string.replace(pos, 2, "\n"); // Reemplaza \n por salto de línea real
                pos += 1;
            }
            pos = 0;
            while ((pos = feedback_json_string.find("\\\"", pos)) != std::string::npos) {
                feedback_json_string.replace(pos, 2, "\""); // Reemplaza \" por comilla real
                pos += 1;
            }
            pos = 0;
            while ((pos = feedback_json_string.find("\\\\", pos)) != std::string::npos) {
                feedback_json_string.replace(pos, 2, "\\"); // Reemplaza \\ por barra invertida real
                pos += 1;
            }

            // Elimina el wrapper ```json del LLM (si existe)
            std::string markdown_wrapper = "```json";
            pos = feedback_json_string.find(markdown_wrapper);
            if (pos != std::string::npos) {
                feedback_json_string.erase(pos, markdown_wrapper.length());
            }

            // Elimina el ``` final (si existe)
            pos = feedback_json_string.rfind("```");
            if (pos != std::string::npos) {
                feedback_json_string.erase(pos, 3);
            }

            // Limpia espacios en blanco o saltos de línea al inicio y al final
            pos = feedback_json_string.find_first_not_of(" \n\r\t");
            if (pos != std::string::npos && pos > 0) {
                feedback_json_string.erase(0, pos);
            }
            pos = feedback_json_string.find_last_not_of(" \n\r\t");
            if (pos != std::string::npos) {
                feedback_json_string.erase(pos + 1);
            }

            // --- FIN DE LA LÓGICA DE LIMPIEZA ---

            // Ya tenemos el JSON "limpio" que produjo el LLM; lo devolvemos tal cual.
            return feedback_json_string;
        }
        else {
            // La API devolvió un código de error (p. ej. 4xx o 5xx).
            std::cerr << "Error de Google API: " << res->status << "\n" << res->body << std::endl;
            return "{\"feedback\": \"Error: No se pudo contactar al Coach AI. Estado: " + std::to_string(res->status) + "\", \"analysis_algorithm\": \"Error\", \"analysis_complexity_time\": \"Error\", \"analysis_complexity_space\": \"Error\"}";
        }
    }
    else {
        // No se recibió respuesta: error de red. 'res' no existirá, pero la librería permite
        // consultar el error a través de res.error().
        auto err = res.error();
        std::cerr << "Error de red: " << httplib::to_string(err) << std::endl;
        return "{\"feedback\": \"Error: Falla de conexión con el Coach AI.\", \"analysis_algorithm\": \"Error\", \"analysis_complexity_time\": \"Error\", \"analysis_complexity_space\": \"Error\"}";
    }
}

// --- Servidor Principal ---

int main() {
    httplib::Server svr;

    /**
     * Endpoint: POST /analyze
     * Recibe un JSON con "code" y "results".
     * Devuelve un JSON con el "feedback", "analysis_algorithm",
     * "analysis_complexity_time" y "analysis_complexity_space".
     */
    svr.Post("/analyze", [](const httplib::Request& req, httplib::Response& res) {
        try {
            // Parseo manual para evitar conflictos de librería
            std::string user_code = manual_json_parse(req.body, "code");
            std::string results = manual_json_parse(req.body, "results");

            if (user_code.empty() || results.empty()) {
                res.status = 400; // Bad Request
                res.set_content("{\"feedback\": \"Error: JSON mal formado. Se esperan 'code' y 'results'.\", \"analysis_algorithm\": \"Error\", \"analysis_complexity_time\": \"Error\", \"analysis_complexity_space\": \"Error\"}", "application/json");
                return;
            }

            // Llama a la lógica principal del LLM
            // 'feedback_json' AHORA CONTIENE EL JSON COMPLETO (como string)
            std::string feedback_json = get_llm_feedback(user_code, results);

            // Como la función de LLM ya devuelve un JSON (como string),
            // simplemente lo reenviamos.
            res.set_content(feedback_json, "application/json");

        }
        catch (const std::exception& e) {
            // Captura de seguridad para cualquier error inesperado
            res.status = 500; // Internal Server Error
            std::string err_msg = "Error interno del servidor: " + std::string(e.what());

            std::string error_json = "{\"feedback\": \"" + escape_json_string(err_msg) + "\", \"analysis_algorithm\": \"Error\", \"analysis_complexity_time\": \"Error\", \"analysis_complexity_space\": \"Error\"}";
            res.set_content(error_json, "application/json");

            std::cerr << err_msg << std::endl;
        }
        });

    std::cout << "Servidor Analizador de Soluciones iniciado en http://localhost:8081" << std::endl;
    svr.listen("0.0.0.0", 8081); // Escucha en el puerto 8081

    return 0;
}