/**
 * gestor.cpp
 * REST API para el Gestor de Problemas
 *
 * Endpoints:
 * - POST   /problems          → Agregar un problema
 * - GET    /problems          → Listar todos los problemas
 * - GET    /problems/category/:category → Filtrar por categoría
 * - GET    /problems/:id      → Obtener un problema específico
 */

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "database.h"
#include "models.h"
#include "json.hpp"  // ✅ CORRECCIÓN: Ruta arreglada
#include <iostream>
#include <mongocxx/instance.hpp>

using json = nlohmann::json;

/**
 * Convierte un JSON a una estructura Problem
 */
Problem json_to_problem(const json& j) {
    Problem p;
    p.id = j.value("id", "");
    p.title = j.value("title", "");
    p.category = j.value("category", "");
    p.difficulty = j.value("difficulty", "medium");
    p.description = j.value("description", "");
    p.template_code = j.value("template_code", "");
    p.created_at = j.value("created_at", "");
    p.updated_at = j.value("updated_at", "");

    // Convertir ejemplos
    if (j.contains("examples")) {
        for (const auto& ex : j["examples"]) {
            Example example;
            example.input = ex.value("input", "");
            example.output = ex.value("output", "");
            example.explanation = ex.value("explanation", "");
            p.examples.push_back(example);
        }
    }

    // Convertir casos de prueba
    if (j.contains("test_cases")) {
        for (const auto& tc : j["test_cases"]) {
            TestCase test_case;
            test_case.input = tc.value("input", "");
            test_case.expected_output = tc.value("expected_output", "");
            test_case.is_hidden = tc.value("is_hidden", false);
            p.test_cases.push_back(test_case);
        }
    }

    // Convertir restricciones
    if (j.contains("constraints")) {
        for (const auto& constraint : j["constraints"]) {
            p.constraints.push_back(constraint.get<std::string>());
        }
    }

    return p;
}

int main() {
    // Inicializar instancia de mongocxx (debe hacerse una sola vez)
    mongocxx::instance instance{};

    // Crear gestor de base de datos
    DatabaseManager db("mongodb://localhost:27017", "codecoach_db");

    // Probar conexión
    if (!db.test_connection()) {
        std::cerr << "ERROR: No se pudo conectar a MongoDB. Asegúrate de que esté corriendo." << std::endl;
        return 1;
    }
    std::cout << "✓ Conexión a MongoDB exitosa" << std::endl;

    // Crear servidor HTTP
    httplib::Server svr;

    // Configurar CORS
    svr.set_pre_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        return httplib::Server::HandlerResponse::Unhandled;
        });

    // Manejar OPTIONS (para CORS)
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
        });

    /**
     * POST /problems
     * Agregar un nuevo problema
     *
     * Body JSON esperado:
     * {
     *   "id": "prob_001",
     *   "title": "Two Sum",
     *   "category": "arrays",
     *   "difficulty": "easy",
     *   "description": "...",
     *   "examples": [...],
     *   "test_cases": [...],
     *   "constraints": [...],
     *   "template_code": "...",
     *   "created_at": "2025-11-02",
     *   "updated_at": "2025-11-02"
     * }
     */
    svr.Post("/problems", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            // Parsear JSON del body
            auto j = json::parse(req.body);

            // Validar campos obligatorios
            if (!j.contains("id") || !j.contains("title") || !j.contains("category")) {
                res.status = 400;
                res.set_content(
                    "{\"error\": \"Faltan campos obligatorios: id, title, category\"}",
                    "application/json"
                );
                return;
            }

            // Convertir JSON a Problem
            Problem problem = json_to_problem(j);

            // Insertar en la base de datos
            if (db.insert_problem(problem)) {
                res.status = 201;
                res.set_content(
                    "{\"message\": \"Problema creado exitosamente\", \"id\": \"" + problem.id + "\"}",
                    "application/json"
                );
                std::cout << "✓ Problema creado: " << problem.id << std::endl;
            }
            else {
                res.status = 500;
                res.set_content(
                    "{\"error\": \"Error al insertar el problema en la base de datos\"}",
                    "application/json"
                );
            }
        }
        catch (const json::parse_error& e) {
            res.status = 400;
            res.set_content(
                "{\"error\": \"JSON inválido: " + std::string(e.what()) + "\"}",
                "application/json"
            );
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                "{\"error\": \"Error interno: " + std::string(e.what()) + "\"}",
                "application/json"
            );
        }
        });

    /**
     * GET /problems
     * Obtener todos los problemas
     */
    svr.Get("/problems", [&db](const httplib::Request&, httplib::Response& res) {
        try {
            std::string problems_json = db.get_all_problems();
            res.set_content(problems_json, "application/json");
            std::cout << "✓ Listado de problemas enviado" << std::endl;
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                "{\"error\": \"Error obteniendo problemas: " + std::string(e.what()) + "\"}",
                "application/json"
            );
        }
        });

    /**
     * GET /problems/category/:category
     * Obtener problemas filtrados por categoría
     */
    svr.Get("/problems/category/:category", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string category = req.path_params.at("category");
            std::string problems_json = db.get_problems_by_category(category);
            res.set_content(problems_json, "application/json");
            std::cout << "✓ Problemas de categoría '" << category << "' enviados" << std::endl;
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                "{\"error\": \"Error filtrando problemas: " + std::string(e.what()) + "\"}",
                "application/json"
            );
        }
        });

    /**
     * GET /problems/:id
     * Obtener un problema específico por ID
     */
    svr.Get("/problems/:id", [&db](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string id = req.path_params.at("id");
            std::string problem_json = db.get_problem_by_id(id);

            if (problem_json == "{}") {
                res.status = 404;
                res.set_content(
                    "{\"error\": \"Problema no encontrado\"}",
                    "application/json"
                );
            }
            else {
                res.set_content(problem_json, "application/json");
                std::cout << "✓ Problema '" << id << "' enviado" << std::endl;
            }
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                "{\"error\": \"Error obteniendo problema: " + std::string(e.what()) + "\"}",
                "application/json"
            );
        }
        });

    /**
     * GET /health
     * Endpoint para verificar que el servidor está funcionando
     */
    svr.Get("/health", [&db](const httplib::Request&, httplib::Response& res) {
        if (db.test_connection()) {
            res.set_content(
                "{\"status\": \"OK\", \"database\": \"connected\"}",
                "application/json"
            );
        }
        else {
            res.status = 503;
            res.set_content(
                "{\"status\": \"ERROR\", \"database\": \"disconnected\"}",
                "application/json"
            );
        }
        });

    // Iniciar servidor
    std::cout << "\n========================================" << std::endl;
    std::cout << "Gestor de Problemas - CodeCoach" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Servidor iniciado en http://localhost:8080" << std::endl;
    std::cout << "\nEndpoints disponibles:" << std::endl;
    std::cout << "  POST   /problems                → Agregar problema" << std::endl;
    std::cout << "  GET    /problems                → Listar todos" << std::endl;
    std::cout << "  GET    /problems/category/:cat  → Filtrar por categoría" << std::endl;
    std::cout << "  GET    /problems/:id            → Obtener problema específico" << std::endl;
    std::cout << "  GET    /health                  → Estado del servidor" << std::endl;
    std::cout << "========================================\n" << std::endl;

    svr.listen("0.0.0.0", 8080);

    return 0;
}