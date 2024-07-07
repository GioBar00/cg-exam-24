#include "modules/Starter.hpp"
using json = nlohmann::json;
using namespace std;

void main() {
	std::ifstream pf("level/properties.json");
    json data = nullptr;
    try {
        data = json::parse(pf);
    } catch (const json::parse_error& e) {
        std::cout << e.what() << std::endl;
    }
    pf.close();
    std::cout << data["structure"]["SPAWN"] << std::endl;

    std::ifstream lf("level/ROOM-00.txt");
    uint16_t rows, columns;
    lf >> rows >> columns;
    vector<vector<string>> LEVEL(rows, std::vector<std::string>(columns, ".")); // Map.
    tuple<uint16_t, uint16_t> O; // Spawn origin.
    for (uint16_t i = 0; i < rows; i++) {
        for (uint16_t j = 0; j < columns; j++) {
            lf >> LEVEL[i][j];
            if (LEVEL[i][j] == "S")
                O = std::tuple(i, j);
        }
    }
    cout << get<0>(O) << ' ' << get<1>(O);
}
