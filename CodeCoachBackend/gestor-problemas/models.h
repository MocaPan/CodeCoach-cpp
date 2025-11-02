/**
 * models.h
 * Estructuras de datos para el Gestor de Problemas
 */

#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

 /**
  * Representa un ejemplo de uso del problema
  */
struct Example {
    std::string input;
    std::string output;
    std::string explanation;
};

/**
 * Representa un caso de prueba
 */
struct TestCase {
    std::string input;
    std::string expected_output;
    bool is_hidden;  // Si es visible para el usuario o solo para evaluación
};

/**
 * Representa un problema completo
 */
struct Problem {
    std::string id;                          // ID personalizado (ej: "prob_001")
    std::string title;                       // Título del problema
    std::string category;                    // Categoría (arrays, strings, etc.)
    std::string difficulty;                  // Dificultad (easy, medium, hard)
    std::string description;                 // Descripción completa
    std::vector<Example> examples;           // Ejemplos de uso
    std::vector<TestCase> test_cases;        // Casos de prueba
    std::vector<std::string> constraints;    // Restricciones
    std::string template_code;               // Código plantilla en C++
    std::string created_at;                  // Fecha de creación
    std::string updated_at;                  // Fecha de actualización
};

#endif // MODELS_H