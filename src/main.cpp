#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

// Function to generate a random action based on probability
bool random_action(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

// Função para gerar números aleatórios entre min e max
int random_int(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

// Função para criar e posicionar as plantas
void create_plants(uint32_t num_plants) {
    for (uint32_t i = 0; i < num_plants; ++i) {
        int row = random_int(0, NUM_ROWS - 1);
        int col = random_int(0, NUM_ROWS - 1);

        // Verifica se a célula está vazia antes de colocar uma planta
        if (entity_grid[row][col].type == empty) {
            entity_grid[row][col] = {plant, 0, 0};
        }
    }
}

// Função para criar e posicionar os herbívoros
void create_herbivores(uint32_t num_herbivores) {
    for (uint32_t i = 0; i < num_herbivores; ++i) {
        int row = random_int(0, NUM_ROWS - 1);
        int col = random_int(0, NUM_ROWS - 1);

        // Verifica se a célula está vazia antes de colocar um herbívoro
        if (entity_grid[row][col].type == empty) {
            entity_grid[row][col] = {herbivore, MAXIMUM_ENERGY, 0};
        }
    }
}

// Função para criar e posicionar os carnívoros
void create_carnivores(uint32_t num_carnivores) {
    for (uint32_t i = 0; i < num_carnivores; ++i) {
        int row = random_int(0, NUM_ROWS - 1);
        int col = random_int(0, NUM_ROWS - 1);

        // Verifica se a célula está vazia antes de colocar um carnívoro
        if (entity_grid[row][col].type == empty) {
            entity_grid[row][col] = {carnivore, MAXIMUM_ENERGY, 0};
        }
    }
}

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entities = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entities > NUM_ROWS * NUM_ROWS) {
            res.code = 400;
            res.body = "Too many entities";
            res.end();
            return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create and position the entities
        create_plants((uint32_t)request_body["plants"]);
        create_herbivores((uint32_t)request_body["herbivores"]);
        create_carnivores((uint32_t)request_body["carnivores"]);

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behavior of each entity
        for (uint32_t row = 0; row < NUM_ROWS; ++row) {
            for (uint32_t col = 0; col < NUM_ROWS; ++col) {
                entity_t &entity = entity_grid[row][col];

                if (entity.type == plant) {
                    // Implement plant behavior for the next iteration
                    // Plants don't move or eat, just age and check if they die
                    entity.age++;

                    if (entity.age >= PLANT_MAXIMUM_AGE) {
                        entity.type = empty; // Plant dies and becomes empty cell
                    }

                    // Probabilidade de Crescimento: 20% de chance por etapa de tempo.
                    // Direção de Crescimento: Célula vazia adjacente selecionada aleatoriamente.
                    if (random_action(PLANT_REPRODUCTION_PROBABILITY)) {
                        pos_t newPos;
                        do {
                            int direction = random_int(0, 3); // 0: up, 1: down, 2: left, 3: right
                            newPos = {row, col};

                            if (direction == 0 && row > 0) {
                                newPos.i = row - 1;
                            } else if (direction == 1 && row < NUM_ROWS - 1) {
                                newPos.i = row + 1;
                            } else if (direction == 2 && col > 0) {
                                newPos.j = col - 1;
                            } else if (direction == 3 && col < NUM_ROWS - 1) {
                                newPos.j = col + 1;
                            }
                        } while (entity_grid[newPos.i][newPos.j].type != empty);

                        // Create a new plant in the adjacent cell
                        entity_grid[newPos.i][newPos.j] = {plant, 0, 0};
                    }
                } else if (entity.type == herbivore) {
                    // Rest of the logic for herbivores
                    entity.age++;

                    // Herbivores have a chance to move
                    if (random_action(HERBIVORE_MOVE_PROBABILITY)) {
                        // Find an empty neighboring cell to move to
                        pos_t newPos;
                        do {
                            int direction = random_int(0, 3); // 0: up, 1: down, 2: left, 3: right
                            newPos = {row, col};

                            if (direction == 0 && row > 0) {
                                newPos.i = row - 1;
                            } else if (direction == 1 && row < NUM_ROWS - 1) {
                                newPos.i = row + 1;
                            } else if (direction == 2 && col > 0) {
                                newPos.j = col - 1;
                            } else if (direction == 3 && col < NUM_ROWS - 1) {
                                newPos.j = col + 1;
                            }
                        } while (entity_grid[newPos.i][newPos.j].type != empty);

                        // Move the herbivore to the new position
                        entity_grid[newPos.i][newPos.j] = entity;
                        entity_grid[row][col] = {empty, 0, 0};
                    }

                    // Herbivores have a chance to eat
                    if (random_action(HERBIVORE_EAT_PROBABILITY)) {
                        // Check neighboring cells for plants to eat
                        for (int i = -1; i <= 1; ++i) {
                            for (int j = -1; j <= 1; ++j) {
                                int newRow = row + i;
                                int newCol = col + j;

                                if (newRow >= 0 && newRow < NUM_ROWS && newCol >= 0 && newCol < NUM_ROWS &&
                                    entity_grid[newRow][newCol].type == plant) {
                                    // Eat the plant and gain energy
                                    entity.energy += 30;
                                    entity_grid[newRow][newCol] = {empty, 0, 0};
                                }
                            }
                        }
                    }

                    // Herbivores have a chance to reproduce
                    if (entity.energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION && random_action(HERBIVORE_REPRODUCTION_PROBABILITY)) {
                        pos_t newPos;
                        do {
                            int direction = random_int(0, 3); // 0: up, 1: down, 2: left, 3: right
                            newPos = {row, col};

                            if (direction == 0 && row > 0) {
                                newPos.i = row - 1;
                            } else if (direction == 1 && row < NUM_ROWS - 1) {
                                newPos.i = row + 1;
                            } else if (direction == 2 && col > 0) {
                                newPos.j = col - 1;
                            } else if (direction == 3 && col < NUM_ROWS - 1) {
                                newPos.j = col + 1;
                            }
                        } while (entity_grid[newPos.i][newPos.j].type != empty);

                        // Create a new herbivore in the adjacent cell
                        entity_t newHerbivore = {herbivore, MAXIMUM_ENERGY, 0};
                        entity_grid[newPos.i][newPos.j] = newHerbivore;
                        entity.energy -= 10; // Deduct energy for reproduction
                    }

                    // Check if herbivore dies of old age or starvation
                    if (entity.age >= HERBIVORE_MAXIMUM_AGE || entity.energy <= 0) {
                        entity_grid[row][col] = {empty, 0, 0}; // Herbivore dies and becomes empty cell
                    }
                } else if (entity.type == carnivore) {
                    // Rest of the logic for carnivores
                    entity.age++;

                    // Carnivores have a chance to move
                    if (random_action(CARNIVORE_MOVE_PROBABILITY)) {
                        // Find an empty neighboring cell to move to
                        pos_t newPos;
                        do {
                            int direction = random_int(0, 3); // 0: up, 1: down, 2: left, 3: right
                            newPos = {row, col};

                            if (direction == 0 && row > 0) {
                                newPos.i = row - 1;
                            } else if (direction == 1 && row < NUM_ROWS - 1) {
                                newPos.i = row + 1;
                            } else if (direction == 2 && col > 0) {
                                newPos.j = col - 1;
                            } else if (direction == 3 && col < NUM_ROWS - 1) {
                                newPos.j = col + 1;
                            }
                        } while (entity_grid[newPos.i][newPos.j].type != empty);

                        // Move the carnivore to the new position
                        entity_grid[newPos.i][newPos.j] = entity;
                        entity_grid[row][col] = {empty, 0, 0};
                    }

                    // Carnivores always try to eat (if adjacent herbivore)
                    if (random_action(CARNIVORE_EAT_PROBABILITY)) {
                        // Check neighboring cells for herbivores to eat
                        for (int i = -1; i <= 1; ++i) {
                            for (int j = -1; j <= 1; ++j) {
                                int newRow = row + i;
                                int newCol = col + j;

                                if (newRow >= 0 && newRow < NUM_ROWS && newCol >= 0 && newCol < NUM_ROWS &&
                                    entity_grid[newRow][newCol].type == herbivore) {
                                    // Eat the herbivore and gain energy
                                    entity.energy += 20;
                                    entity_grid[newRow][newCol] = {empty, 0, 0};
                                }
                            }
                        }
                    }

                    // Carnivores have a chance to reproduce
                    if (entity.energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION && random_action(CARNIVORE_REPRODUCTION_PROBABILITY)) {
                        pos_t newPos;
                        do {
                            int direction = random_int(0, 3); // 0: up, 1: down, 2: left, 3: right
                            newPos = {row, col};

                            if (direction == 0 && row > 0) {
                                newPos.i = row - 1;
                            } else if (direction == 1 && row < NUM_ROWS - 1) {
                                newPos.i = row + 1;
                            } else if (direction == 2 && col > 0) {
                                newPos.j = col - 1;
                            } else if (direction == 3 && col < NUM_ROWS - 1) {
                                newPos.j = col + 1;
                            }
                        } while (entity_grid[newPos.i][newPos.j].type != empty);

                        // Create a new carnivore in the adjacent cell
                        entity_t newCarnivore = {carnivore, MAXIMUM_ENERGY, 0};
                        entity_grid[newPos.i][newPos.j] = newCarnivore;
                        entity.energy -= 10; // Deduct energy for reproduction
                    }

                    // Check if carnivore dies of old age or starvation
                    if (entity.age >= CARNIVORE_MAXIMUM_AGE || entity.energy <= 0) {
                        entity_grid[row][col] = {empty, 0, 0}; // Carnivore dies and becomes empty cell
                    }
                }
            }
        }

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid;
        return json_grid.dump();
    });
    
    app.port(8080).run();

    return 0;
}