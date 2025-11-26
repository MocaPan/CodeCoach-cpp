# 📚 DOCUMENTACIÓN DE LA API - GESTOR DE PROBLEMAS

## 🌐 Información General

- **Base URL:** `http://localhost:8080`
- **Formato:** JSON
- **CORS:** Habilitado para todos los orígenes

---

## 📍 Endpoints

### 1. **POST /problems**
Agregar un nuevo problema a la base de datos.

**Request:**
```http
POST http://localhost:8080/problems
Content-Type: application/json

{
  "id": "prob_001",
  "title": "Two Sum",
  "category": "arrays",
  "difficulty": "easy",
  "description": "Dado un array de enteros nums y un entero target...",
  "examples": [
    {
      "input": "4\n2 7 11 15\n9",
      "output": "0 1",
      "explanation": "Porque nums[0] + nums[1] == 9"
    }
  ],
  "test_cases": [
    {
      "input": "4\n2 7 11 15\n9",
      "expected_output": "0 1",
      "is_hidden": false
    }
  ],
  "constraints": [
    "2 <= nums.length <= 10^4",
    "-10^9 <= nums[i] <= 10^9"
  ],
  "template_code": "#include <iostream>\nusing namespace std;\n\nint main() {\n    return 0;\n}",
  "created_at": "2025-11-02",
  "updated_at": "2025-11-02"
}
```

**Response (201 Created):**
```json
{
  "message": "Problema creado exitosamente",
  "id": "prob_001"
}
```

**Response (400 Bad Request):**
```json
{
  "error": "Faltan campos obligatorios: id, title, category"
}
```

---

### 2. **GET /problems**
Obtener todos los problemas.

**Request:**
```http
GET http://localhost:8080/problems
```

**Response (200 OK):**
```json
[
  {
    "_id": {"$oid": "..."},
    "id": "prob_001",
    "title": "Two Sum",
    "category": "arrays",
    "difficulty": "easy",
    "description": "...",
    "examples": [...],
    "test_cases": [...],
    "constraints": [...],
    "template_code": "...",
    "created_at": "2025-11-02",
    "updated_at": "2025-11-02"
  },
  {
    "_id": {"$oid": "..."},
    "id": "prob_002",
    "title": "Reverse String",
    ...
  }
]
```

---

### 3. **GET /problems/category/:category**
Obtener problemas filtrados por categoría.

**Categorías disponibles:**
- `arrays`
- `strings`
- `stacks`
- `dynamic-programming`
- `trees`
- `graphs`

**Request:**
```http
GET http://localhost:8080/problems/category/arrays
```

**Response (200 OK):**
```json
[
  {
    "_id": {"$oid": "..."},
    "id": "prob_001",
    "title": "Two Sum",
    "category": "arrays",
    ...
  },
  {
    "_id": {"$oid": "..."},
    "id": "prob_005",
    "title": "Maximum Subarray",
    "category": "arrays",
    ...
  }
]
```

---

### 4. **GET /problems/:id**
Obtener un problema específico por su ID.

**Request:**
```http
GET http://localhost:8080/problems/prob_001
```

**Response (200 OK):**
```json
{
  "_id": {"$oid": "..."},
  "id": "prob_001",
  "title": "Two Sum",
  "category": "arrays",
  "difficulty": "easy",
  "description": "...",
  "examples": [...],
  "test_cases": [...],
  "constraints": [...],
  "template_code": "...",
  "created_at": "2025-11-02",
  "updated_at": "2025-11-02"
}
```

**Response (404 Not Found):**
```json
{
  "error": "Problema no encontrado"
}
```

---

### 5. **GET /health**
Verificar el estado del servidor y la conexión a la base de datos.

**Request:**
```http
GET http://localhost:8080/health
```

**Response (200 OK):**
```json
{
  "status": "OK",
  "database": "connected"
}
```

**Response (503 Service Unavailable):**
```json
{
  "status": "ERROR",
  "database": "disconnected"
}
```

---

## 🧪 PRUEBAS CON POSTMAN

### Colección de Postman

Importa esta colección en Postman para probar todos los endpoints:

```json
{
  "info": {
    "name": "CodeCoach - Gestor de Problemas",
    "schema": "https://schema.getpostman.com/json/collection/v2.1.0/collection.json"
  },
  "item": [
    {
      "name": "Health Check",
      "request": {
        "method": "GET",
        "header": [],
        "url": {
          "raw": "http://localhost:8080/health",
          "protocol": "http",
          "host": ["localhost"],
          "port": "8080",
          "path": ["health"]
        }
      }
    },
    {
      "name": "Crear Problema",
      "request": {
        "method": "POST",
        "header": [
          {
            "key": "Content-Type",
            "value": "application/json"
          }
        ],
        "body": {
          "mode": "raw",
          "raw": "{\n  \"id\": \"prob_001\",\n  \"title\": \"Two Sum\",\n  \"category\": \"arrays\",\n  \"difficulty\": \"easy\",\n  \"description\": \"Dado un array de enteros nums y un entero target, devuelve los índices de los dos números que suman target.\",\n  \"examples\": [\n    {\n      \"input\": \"4\\n2 7 11 15\\n9\",\n      \"output\": \"0 1\",\n      \"explanation\": \"Porque nums[0] + nums[1] == 9\"\n    }\n  ],\n  \"test_cases\": [\n    {\n      \"input\": \"4\\n2 7 11 15\\n9\",\n      \"expected_output\": \"0 1\",\n      \"is_hidden\": false\n    }\n  ],\n  \"constraints\": [\n    \"2 <= nums.length <= 10^4\"\n  ],\n  \"template_code\": \"#include <iostream>\\nusing namespace std;\\n\\nint main() {\\n    return 0;\\n}\",\n  \"created_at\": \"2025-11-02\",\n  \"updated_at\": \"2025-11-02\"\n}"
        },
        "url": {
          "raw": "http://localhost:8080/problems",
          "protocol": "http",
          "host": ["localhost"],
          "port": "8080",
          "path": ["problems"]
        }
      }
    },
    {
      "name": "Listar Todos los Problemas",
      "request": {
        "method": "GET",
        "header": [],
        "url": {
          "raw": "http://localhost:8080/problems",
          "protocol": "http",
          "host": ["localhost"],
          "port": "8080",
          "path": ["problems"]
        }
      }
    },
    {
      "name": "Filtrar por Categoría",
      "request": {
        "method": "GET",
        "header": [],
        "url": {
          "raw": "http://localhost:8080/problems/category/arrays",
          "protocol": "http",
          "host": ["localhost"],
          "port": "8080",
          "path": ["problems", "category", "arrays"]
        }
      }
    },
    {
      "name": "Obtener Problema Específico",
      "request": {
        "method": "GET",
        "header": [],
        "url": {
          "raw": "http://localhost:8080/problems/prob_001",
          "protocol": "http",
          "host": ["localhost"],
          "port": "8080",
          "path": ["problems", "prob_001"]
        }
      }
    }
  ]
}
```

---

## 🔧 INSTALACIÓN Y CONFIGURACIÓN

### Requisitos Previos

1. **MongoDB:** Debe estar instalado y corriendo en `localhost:27017`
2. **MongoDB C++ Driver:** Instalado en el sistema
3. **OpenSSL:** Para soporte HTTPS
4. **CMake:** Versión 3.15 o superior
5. **Compilador C++20:** GCC, Clang, o MSVC

### Pasos de Instalación

```bash
# 1. Clonar el repositorio
git clone <tu-repositorio>
cd CodeCoachBackend

# 2. Crear directorio de build
mkdir build
cd build

# 3. Configurar con CMake
cmake ..

# 4. Compilar
cmake --build .

# 5. Ejecutar el servidor
./gestor_problemas
```

### Verificar MongoDB

```bash
# Iniciar MongoDB (si no está corriendo)
mongod --dbpath /path/to/data

# Verificar conexión
mongo
> show dbs
```

---

## 📦 CARGAR DATOS DE EJEMPLO

Usa el archivo `sample_problems.json` para cargar problemas de prueba:

```bash
# Usando Python
python3 << EOF
import requests
import json

with open('sample_problems.json', 'r') as f:
    problems = json.load(f)

for problem in problems:
    response = requests.post('http://localhost:8080/problems', json=problem)
    print(f"Problema {problem['id']}: {response.status_code}")
EOF
```

O con **cURL**:

```bash
# Cargar un problema individual
curl -X POST http://localhost:8080/problems \
  -H "Content-Type: application/json" \
  -d @sample_problems.json
```

---

## 🐛 TROUBLESHOOTING

### Error: "No se pudo conectar a MongoDB"
```bash
# Verificar que MongoDB esté corriendo
sudo systemctl status mongodb
# o
ps aux | grep mongod
```

### Error de compilación: "mongocxx/client.hpp: No such file"
```bash
# Instalar MongoDB C++ Driver
# Ubuntu/Debian:
sudo apt-get install libmongocxx-dev libbsoncxx-dev

# macOS:
brew install mongo-cxx-driver
```

### Puerto 8080 en uso
```bash
# Cambiar el puerto en gestor.cpp línea final:
svr.listen("0.0.0.0", 8081);  // Usar otro puerto
```

---

## ✅ CHECKLIST DE VALIDACIÓN

- [ ] MongoDB está instalado y corriendo
- [ ] El servidor compila sin errores
- [ ] El servidor inicia en el puerto 8080
- [ ] GET /health retorna status "OK"
- [ ] Puedes crear un problema con POST /problems
- [ ] Puedes listar problemas con GET /problems
- [ ] Puedes filtrar por categoría
- [ ] Puedes obtener un problema específico por ID

---

## 📝 NOTAS ADICIONALES

### Estructura de la Base de Datos

```
codecoach_db
  └── problems (colección)
      ├── _id (ObjectId)
      ├── id (string)
      ├── title (string)
      ├── category (string)
      ├── difficulty (string)
      ├── description (string)
      ├── examples (array)
      ├── test_cases (array)
      ├── constraints (array)
      ├── template_code (string)
      ├── created_at (string)
      └── updated_at (string)
```

### Categorías Sugeridas

- `arrays` - Problemas de arreglos
- `strings` - Manipulación de cadenas
- `stacks` - Pilas y colas
- `trees` - Árboles binarios
- `graphs` - Grafos
- `dynamic-programming` - Programación dinámica
- `sorting` - Ordenamiento
- `searching` - Búsqueda
- `recursion` - Recursión

---

**Última actualización:** 2025-11-02