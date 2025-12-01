#include "lib/httplib.h"
#include "lib/json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <algorithm>
#include <filesystem>

using namespace httplib;
using json = nlohmann::json;

const std::string BASE = "C:\\CodeCoachRuntime\\";

// Leer archivo
std::string read_file(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void set_utf8(Response &res, const std::string &data, const std::string &type) {
    res.set_content(data, (type + "; charset=utf-8").c_str());
}

// Ejecutar comando y capturar salida
std::string exec_cmd(const std::string &cmd) {
    std::array<char, 256> buffer;
    std::string result;

    FILE *pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "Error ejecutando comando.";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    _pclose(pipe);
    return result;
}

int main() {
    std::filesystem::create_directories(BASE);

    Server svr;

    svr.Get("/", [&](const Request &, Response &res) {
        set_utf8(res, read_file("web/index.html"), "text/html");
    });

    svr.Get("/app.js", [&](const Request &, Response &res) {
        set_utf8(res, read_file("web/app.js"), "application/javascript");
    });

    svr.Get("/style.css", [&](const Request &, Response &res) {
        set_utf8(res, read_file("web/style.css"), "text/css");
    });

    svr.Get("/api/problemas", [&](const Request &, Response &res) {
        set_utf8(res, read_file("problemas.json"), "application/json");
    });

    // ===================== EJECUTAR CÓDIGO =======================
    svr.Post("/api/ejecutar", [&](const Request &req, Response &res) {
        try {
            auto body = json::parse(req.body);
            std::string codigo = body.value("codigo", "");
            std::string problemaId = body.value("problemaId", "");

            json problemas = json::parse(read_file("problemas.json"));
            json problema;
            bool found = false;

            for (auto &p : problemas) {
                if (p["id"] == problemaId) {
                    problema = p;
                    found = true;
                }
            }

            if (!found) {
                set_utf8(res, R"({"error":"Problema no encontrado"})", "application/json");
                return;
            }

            std::string input = problema["examples"][0]["input"];
            std::string esperado = problema["examples"][0]["output"];

            std::string f_src = BASE + "temp.cpp";
            std::string f_exe = BASE + "temp_exec.exe";
            std::string f_in  = BASE + "input.txt";

            { std::ofstream out(f_src); out << codigo; }
            { std::ofstream out(f_in);  out << input; }

            // COMPILAR
            std::string cmd_compile =
                "C:\\mingw64\\bin\\g++.exe \"" + f_src + "\" -o \"" + f_exe + "\" 2>&1";

            std::string err = exec_cmd(cmd_compile);

            if (!err.empty()) {
                json resp = {
                    {"compilacion", "error"},
                    {"mensaje", err}
                };
                set_utf8(res, resp.dump(2), "application/json");
                return;
            }

            // EJECUTAR PROGRAMA
            std::string cmd_run =
                "cmd.exe /C \"" + f_exe + " < " + f_in + "\"";

            std::string salida = exec_cmd(cmd_run);

            // RESPUESTA BASE
            json resp = {
                {"compilacion", "ok"},
                {"input", input},
                {"salida_usuario", salida},
                {"salida_esperada", esperado}
            };

            //  LLAMAR ANALIZADOR IA
            try {
                httplib::Client cli_ai("http://localhost:8081");

                auto escape = [](const std::string &s) {
                    std::string o = s;
                    size_t pos = 0;
                    while ((pos = o.find("\"", pos)) != std::string::npos) {
                        o.replace(pos, 1, "\\\"");
                        pos += 2;
                    }
                    return o;
                };

                std::string payload_ai =
                    "{ \"code\": \"" + escape(codigo) + "\", "
                    "\"results\": \"Input: " + escape(input) +
                    "\\nOutput Usuario: " + escape(salida) +
                    "\\nOutput Esperado: " + escape(esperado) + "\" }";

                if (auto res_ai = cli_ai.Post("/analyze", payload_ai, "application/json")) {
                    resp["coach_ai"] = res_ai->body;
                } else {
                    resp["coach_ai"] = "No se pudo contactar al analizador.";
                }
            }
            catch (...) {
                resp["coach_ai"] = "Error llamando al analizador.";
            }

            // RESPUESTA FINAL
            set_utf8(res, resp.dump(2), "application/json");

        } catch (std::exception &e) {
            json err = {{"error", e.what()}};
            set_utf8(res, err.dump(2), "application/json");
        }
    });

    std::cout << "Servidor iniciado en http://localhost:8083\n";
    svr.listen("0.0.0.0", 8083);
}


