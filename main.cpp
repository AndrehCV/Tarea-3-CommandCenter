#include <iostream>
#include <string>
#include <list>
#include <map>
#include <functional>
#include <stdexcept>
#include <utility>


class Entity {
private:
    std::string name;
    float health;
    int x, y;

public:
    Entity(std::string n) : name(n), health(100.0f), x(0), y(0) {
    }

    // Métodos públicos para modificar el estado
    void move(int dx, int dy) {
        x += dx;
        y += dy;
    }

    void heal(float amount) {
        health += amount;
        if (health > 100.0f) health = 100.0f;
    }

    void damage(float amount) {
        health -= amount;
        if (health < 0.0f) health = 0.0f;
    }

    void reset() {
        health = 100.0f;
        x = 0;
        y = 0;
    }

    // Método para obtener el estado actual como string
    std::string getStatus() const {
        return "[" + name + "] HP: " + std::to_string(health) + " | Pos: (" + std::to_string(x) + ", " +
               std::to_string(y) + ")";
    }
};


// Contenedor genérico para llamadas
using Command = std::function<void(const std::list<std::string> &)>;


// (CommandCenter)

class CommandCenter {
private:
    Entity &entity; // Instancia compartida
    std::map<std::string, Command> commands;
    std::list<std::string> history;
    std::map<std::string, std::list<std::pair<std::string, std::list<std::string> > > > macros;

public:
    CommandCenter(Entity &e) : entity(e) {
    }

    // Registra un comando simple
    void registerCommand(const std::string &name, Command cmd) {
        commands[name] = cmd;
    }

    // Elimina un comando dinámicamente
    void removeCommand(const std::string &name) {
        std::map<std::string, Command>::iterator it = commands.find(name); // Uso explícito de iterador
        if (it != commands.end()) {
            commands.erase(it);
            std::cout << "-> Comando '" << name << "' eliminado exitosamente.\n";
        } else {
            std::cout << "-> Error: El comando '" << name << "' no existe para ser eliminado.\n";
        }
    }

    // Ejecuta un comando simple
    void execute(const std::string &name, const std::list<std::string> &args) {
        std::map<std::string, Command>::iterator it = commands.find(name);

        if (it != commands.end()) {
            std::string stateBefore = entity.getStatus();
            try {
                // Ejecución del comando
                it->second(args);
                std::string stateAfter = entity.getStatus();

                // Guardado en el historial
                std::string logEntry = "CMD: " + name + " | Antes: " + stateBefore + " | Despues: " + stateAfter;
                history.push_back(logEntry);
            } catch (const std::exception &e) {
                std::cout << "-> Error en la ejecucion del comando '" << name << "': " << e.what() << "\n";
            }
        } else {
            std::cout << "-> Error: Comando '" << name << "' no encontrado.\n";
        }
    }

    // Registra un comando compuesto
    void registerMacro(const std::string &name,
                       const std::list<std::pair<std::string, std::list<std::string> > > &steps) {
        macros[name] = steps;
    }

    // Ejecuta un comando compuesto
    void executeMacro(const std::string &name) {
        std::map<std::string, std::list<std::pair<std::string, std::list<std::string> > > >::iterator macroIt = macros.
                find(name);

        if (macroIt != macros.end()) {
            std::cout << "\n=== Ejecutando Macro: " << name << " ===\n";

            // Recorrido de pasos
            std::list<std::pair<std::string, std::list<std::string> > >::iterator stepIt = macroIt->second.begin();
            for (; stepIt != macroIt->second.end(); ++stepIt) {
                // Verificación de existencia del comando interno
                if (commands.find(stepIt->first) == commands.end()) {
                    std::cout << "-> Ejecucion de macro detenida. Comando interno no existe: " << stepIt->first << "\n";
                    return;
                }

                // Toda ejecución debe realizarse a través de execute
                execute(stepIt->first, stepIt->second);
            }
        } else {
            std::cout << "-> Error: Macro '" << name << "' no encontrado.\n";
        }
    }

    //  historial completo de ejecuciones
    void showHistory() {
        std::cout << "\n--- Historial de Ejecucion ---\n";
        std::list<std::string>::iterator it = history.begin(); // Uso explícito de iterador
        for (; it != history.end(); ++it) {
            std::cout << *it << "\n";
        }
        std::cout << "------------------------------\n";
    }
};


// A) Comando como Función Libre
void moveFunction(Entity &e, const std::list<std::string> &args) {
    if (args.size() != 2) throw std::invalid_argument("El comando 'move' requiere exactamente 2 argumentos (x, y).");

    auto it = args.begin();
    int x = std::stoi(*it);
    int y = std::stoi(*(++it)); // Validación y conversión
    e.move(x, y);
}

void statusFunction(Entity &e, const std::list<std::string> &args) {
    if (!args.empty()) throw std::invalid_argument("El comando 'status' no requiere argumentos.");
    std::cout << e.getStatus() << "\n";
}

// B) Comando como Functor
class DamageCommand {
private:
    Entity &entity;
    int executionCount;
public:
    DamageCommand(Entity &e) : entity(e), executionCount(0) {
    }

    void operator()(const std::list<std::string> &args) {
        if (args.size() != 1) throw std::invalid_argument(
            "El comando 'damage' requiere exactamente 1 argumento (cantidad).");

        float dmg = std::stof(args.front()); // Conversión a decimal
        entity.damage(dmg);
        executionCount++;
        std::cout << "  (Nota interna: Comando damage ejecutado " << executionCount << " veces en total)\n";
    }
};


int main() {
    // 1. Creación de la entidad y el centro de comandos
    Entity player("Heroe");
    CommandCenter center(player);

    // 2. Registro de comandos

    // Registro usando std::bind para enlazar la función libre con la entidad
    center.registerCommand("move", std::bind(moveFunction, std::ref(player), std::placeholders::_1));
    center.registerCommand("status", std::bind(statusFunction, std::ref(player), std::placeholders::_1));

    // Registro como Lambda con captura por referencia
    center.registerCommand("heal", [&player](const std::list<std::string> &args) {
        if (args.size() != 1) throw std::invalid_argument("El comando 'heal' requiere 1 argumento (cantidad).");
        float hp = std::stof(args.front());
        player.heal(hp);
    });

    center.registerCommand("reset", [&player](const std::list<std::string> &args) {
        if (!args.empty()) throw std::invalid_argument("El comando 'reset' no requiere argumentos.");
        player.reset();
    });

    // Registro usando Functor instanciado
    DamageCommand damageFunctor(player);
    center.registerCommand("damage", damageFunctor);


    // 3. Creación y Registro de Macros
    center.registerMacro("defensive_retreat", {
                             {"move", {"-10", "-5"}},
                             {"heal", {"30.5"}}
                         });

    center.registerMacro("reckless_attack", {
                             {"move", {"20", "0"}},
                             {"damage", {"15.0"}},
                             {"status", {}}
                         });

    center.registerMacro("panic_reset", {
                             {"damage", {"999"}},
                             {"reset", {}},
                             {"status", {}}
                         });


    // 4. Ejecución y validaciones
    std::cout << "=== PRUEBAS DE COMANDOS AISLADOS ===\n";

    // Ejemplos válidos
    center.execute("status", {});
    center.execute("move", {"10", "20"});
    center.execute("damage", {"40.5"});
    center.execute("heal", {"15"});

    // Ejemplos inválidos
    std::cout << "\n=== PRUEBAS DE MANEJO DE ERRORES ===\n";
    center.execute("move", {"10"}); // Faltan argumentos
    center.execute("heal", {"texto_invalido"}); // Error de conversión
    center.execute("fly", {}); // Comando inexistente

    std::cout << "\n=== PRUEBAS DE MACROS ===\n";
    center.executeMacro("defensive_retreat");
    center.executeMacro("reckless_attack");
    center.executeMacro("panic_reset");

    std::cout << "\n=== ELIMINACION DINAMICA Y VERIFICACION ===\n";
    // Eliminar comando y probar macro fallido
    center.removeCommand("reset");
    center.executeMacro("panic_reset"); // Debe detenerse a mitad de la ejecución

    center.showHistory();

    return 0;
}
