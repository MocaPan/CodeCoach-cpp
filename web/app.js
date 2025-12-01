// app.js — frontend principal

let problemas = [];
let problemaSeleccionado = null;

function mostrarSalida(msg) {
    document.getElementById("salida").textContent = msg;
}

// Cargar lista de problemas
document.getElementById("btnCargar").addEventListener("click", async () => {
    mostrarSalida("Cargando problemas...");
    try {
        const res = await fetch("/api/problemas");
        if (!res.ok) throw new Error("No se pudo obtener problemas");
        const data = await res.json();
        problemas = data;

        const cont = document.getElementById("problemas");
        cont.innerHTML = "";

        data.forEach(p => {
            const card = document.createElement("div");
            card.className = "problema";
            card.innerHTML = `
                <h3>${p.title}</h3>
                <p><b>Dificultad:</b> ${p.difficulty}</p>
                <p>${p.description}</p>
                <button data-id="${p.id}">Seleccionar</button>
            `;
            card.querySelector("button")
                .addEventListener("click", () => seleccionarProblema(p.id));
            cont.appendChild(card);
        });

        mostrarSalida("Problemas cargados correctamente.");
    } catch (err) {
        console.error(err);
        mostrarSalida("Error cargando problemas: " + err.message);
    }
});

function seleccionarProblema(id) {
    const p = problemas.find(x => x.id === id);
    if (!p) {
        alert("Problema no encontrado");
        return;
    }

    problemaSeleccionado = p.id;
    document.getElementById("tituloProblema").textContent = p.title;
    document.getElementById("metaProblema").textContent =
        `Categoría: ${p.category || "N/A"} | Dificultad: ${p.difficulty}`;

    let texto = p.description + "\n\nEjemplos:\n";
    if (Array.isArray(p.examples)) {
        p.examples.forEach((ex, i) => {
            texto += `\nEjemplo ${i + 1}\nInput:\n${ex.input}\nOutput:\n${ex.output}\n`;
        });
    }
    document.getElementById("descripcion").textContent = texto;

    document.getElementById("codigo").value =
        p.template_code || "// Escribe tu solución aquí";

    mostrarSalida("Problema seleccionado: " + p.title);
}

// Ejecutar código real + análisis IA
document.getElementById("btnEjecutar").addEventListener("click", async () => {
    if (!problemaSeleccionado) {
        alert("Selecciona un problema primero.");
        return;
    }

    const codigo = document.getElementById("codigo").value.trim();
    if (!codigo) {
        alert("Escribe tu código antes de ejecutar.");
        return;
    }

    mostrarSalida("Ejecutando código...");
    try {
        const res = await fetch("/api/ejecutar", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                problemaId: problemaSeleccionado,
                codigo
            })
        });

        const data = await res.json();

        // Error de compilación
        if (data.compilacion === "error") {
            mostrarSalida("Error de compilación:\n\n" + data.mensaje);
            return;
        }

        //Preparar salida del usuario
        const salidaUsuario =
            (data.salida_usuario && data.salida_usuario.trim().length > 0)
                ? data.salida_usuario.trim()
                : "(sin salida)";

        //ANALIZAR SOLUCIÓN CON TU IA
        let analisis;
        try {
            const respAI = await fetch("http://localhost:8081/analyze", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    code: codigo,
                    results:
                        "Input:\n" + data.input +
                        "\nSalida usuario:\n" + salidaUsuario +
                        "\nSalida esperada:\n" + data.salida_esperada
                })
            });

            analisis = await respAI.json();
        } catch (error) {
            analisis = {
                feedback: "No se pudo obtener análisis del coach.",
                analysis_algorithm: "N/A",
                analysis_complexity_time: "N/A",
                analysis_complexity_space: "N/A"
            };
        }

        //MOSTRAR TODO EN LA INTERFAZ
        mostrarSalida(
            "Ejecución exitosa\n\n" +
            "Input:\n" + data.input + "\n\n" +
            "Salida del usuario:\n" + salidaUsuario + "\n\n" +
            "Salida esperada:\n" + data.salida_esperada + "\n\n" +
            "Feedback del Coach:\n" + analisis.feedback + "\n\n" +
            "Algoritmo detectado: " + analisis.analysis_algorithm + "\n" +
            "Complejidad temporal: " + analisis.analysis_complexity_time + "\n" +
            "Complejidad espacial: " + analisis.analysis_complexity_space
        );

    } catch (err) {
        mostrarSalida("Error ejecutando: " + err.message);
    }
});

// Insertar código de prueba
document.getElementById("presetCodigo").addEventListener("change", (e) => {
    const val = e.target.value;
    if (val) {
        document.getElementById("codigo").value = val;
    }
});

