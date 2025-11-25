/**
 * database.h
 * Funciones para interactuar con MongoDB
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include "models.h"

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

/**
 * Clase para manejar la conexión y operaciones con MongoDB
 */
class DatabaseManager {
private:
    mongocxx::client client;
    mongocxx::database db;
    mongocxx::collection problems_collection;

public:
    /**
     * Constructor: inicializa la conexión a MongoDB
     * @param connection_string URI de conexión (ej: "mongodb://localhost:27017")
     * @param db_name Nombre de la base de datos
     */
    DatabaseManager(const std::string& connection_string, const std::string& db_name) {
        client = mongocxx::client{ mongocxx::uri{connection_string} };
        db = client[db_name];
        problems_collection = db["problems"];
    }

    /**
     * Inserta un nuevo problema en la base de datos
     * @param problem El problema a insertar
     * @return true si se insertó correctamente, false si hubo error
     */
    bool insert_problem(const Problem& problem) {
        try {
            // Construir documento BSON
            auto builder = document{};
            builder << "id" << problem.id
                << "title" << problem.title
                << "category" << problem.category
                << "difficulty" << problem.difficulty
                << "description" << problem.description;

            // Agregar ejemplos
            auto examples_array = builder << "examples" << bsoncxx::builder::stream::open_array;
            for (const auto& ex : problem.examples) {
                examples_array << bsoncxx::builder::stream::open_document
                    << "input" << ex.input
                    << "output" << ex.output
                    << "explanation" << ex.explanation
                    << bsoncxx::builder::stream::close_document;
            }
            examples_array << bsoncxx::builder::stream::close_array;

            // Agregar casos de prueba
            auto test_cases_array = builder << "test_cases" << bsoncxx::builder::stream::open_array;
            for (const auto& tc : problem.test_cases) {
                test_cases_array << bsoncxx::builder::stream::open_document
                    << "input" << tc.input
                    << "expected_output" << tc.expected_output
                    << "is_hidden" << tc.is_hidden
                    << bsoncxx::builder::stream::close_document;
            }
            test_cases_array << bsoncxx::builder::stream::close_array;

            // Agregar restricciones
            auto constraints_array = builder << "constraints" << bsoncxx::builder::stream::open_array;
            for (const auto& constraint : problem.constraints) {
                constraints_array << constraint;
            }
            constraints_array << bsoncxx::builder::stream::close_array;

            builder << "template_code" << problem.template_code
                << "created_at" << problem.created_at
                << "updated_at" << problem.updated_at
                << finalize;

            problems_collection.insert_one(builder.view());
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error insertando problema: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * Obtiene todos los problemas
     * @return JSON string con todos los problemas
     */
    std::string get_all_problems() {
        try {
            auto cursor = problems_collection.find({});
            std::string result = "[";
            bool first = true;

            for (auto&& doc : cursor) {
                if (!first) result += ",";
                result += bsoncxx::to_json(doc);
                first = false;
            }
            result += "]";
            return result;
        }
        catch (const std::exception& e) {
            std::cerr << "Error obteniendo problemas: " << e.what() << std::endl;
            return "[]";
        }
    }

    /**
     * Obtiene problemas por categoría
     * @param category La categoría a filtrar
     * @return JSON string con los problemas filtrados
     */
    std::string get_problems_by_category(const std::string& category) {
        try {
            auto filter = document{} << "category" << category << finalize;
            auto cursor = problems_collection.find(filter.view());

            std::string result = "[";
            bool first = true;

            for (auto&& doc : cursor) {
                if (!first) result += ",";
                result += bsoncxx::to_json(doc);
                first = false;
            }
            result += "]";
            return result;
        }
        catch (const std::exception& e) {
            std::cerr << "Error filtrando por categoría: " << e.what() << std::endl;
            return "[]";
        }
    }

    /**
     * Obtiene un problema específico por ID
     * @param id El ID del problema
     * @return JSON string con el problema o "{}" si no existe
     */
    std::string get_problem_by_id(const std::string& id) {
        try {
            auto filter = document{} << "id" << id << finalize;
            auto maybe_result = problems_collection.find_one(filter.view());

            if (maybe_result) {
                return bsoncxx::to_json(*maybe_result);
            }
            return "{}";
        }
        catch (const std::exception& e) {
            std::cerr << "Error obteniendo problema por ID: " << e.what() << std::endl;
            return "{}";
        }
    }

    /**
     * Verifica si la conexión a la base de datos está activa
     * @return true si la conexión es exitosa
     */
    bool test_connection() {
        try {
            // Intenta ejecutar un comando simple
            auto result = db.run_command(document{} << "ping" << 1 << finalize);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error de conexión: " << e.what() << std::endl;
            return false;
        }
    }
};

#endif // DATABASE_H