#include "modules/Starter.hpp"
using json = nlohmann::json;
using namespace std;

#define UNIT 3.1f
#define OFFSET 0.1f

vector<vector<string>> loadMap(string file, tuple<uint16_t, uint16_t>* O) {
    std::ifstream lf(file);
    uint16_t rows, columns;
    lf >> rows >> columns;
    vector<vector<string>> LEVEL(rows, std::vector<std::string>(columns, ".")); // Map.
    for (uint16_t i = 0; i < rows; i++) {
        for (uint16_t j = 0; j < columns; j++) {
            lf >> LEVEL[i][j];
            if (LEVEL[i][j] == "S")
                *O = std::tuple(i, j); // Spawn origin.
        }
    }
    lf.close();
    return LEVEL;
}

void parseConfig(json* data) {
    std::ifstream pf("level/properties.json");
    try {
        *data = json::parse(pf);
    }
    catch (const json::parse_error& e) {
        std::cout << e.what() << std::endl;
    }
    pf.close();
}

glm::mat4 transform(json root, string type, int id, tuple<uint16_t, uint16_t> dist) {
    json branch = root[type], ops = branch["transform"], flag = branch["variant"];
    if (flag.get<bool>())
        ops = ops[id]["operations"];
    vector<json> lst = ops.get<vector<json>>();
    glm::mat4 T = glm::mat4(1);
    string t;
    for (auto x : lst) {
        t = x.begin().key();
        if (t == "translate") {
            array<float, 3> v = x[t].get<array<float, 3>>();
            T = glm::translate(T, glm::vec3(get<0>(dist) * UNIT * v[0], UNIT * v[1], get<1>(dist) * UNIT * v[2]));
            // TODO: Handle multiplication by OFFSET.
        }
        else if (t == "rotate") {
            float ang = x[t].get<float>();
            T = glm::rotate(T, glm::radians(ang), glm::vec3(0, 1, 0));
        }
        else if (t == "scale") {
            array<float, 3> v = x[t].get<array<float, 3>>();
            T = glm::scale(T, glm::vec3(v[0], v[1], v[2]));
        }
    }
    return T;
}

vector<vector<glm::mat4>> applyConfig(vector<vector<string>> LEVEL, json data, tuple<uint16_t, uint16_t> O) {
    uint16_t rows = LEVEL.size(), columns = LEVEL[0].size();
    json structure = data["structure"], light = data["light"];
    string test;
    vector<vector<glm::mat4>> ans;
    ans.resize(rows);
    for (uint16_t i = 0; i < rows; i++) {
        ans[i].resize(columns);
        for (uint16_t j = 0; j < columns; j++) {
            test = LEVEL[i][j];
            if (test == "S") {
                //transform(structure, "SPAWN", -1);
                continue; // TODO.
            }
            else if (test == "G") {
                ans[i][j] = transform(structure, "GROUND", -1, tuple(i - get<0>(O), j - get<1>(O)));
            }
            else if (test[0] == 'W') {
                if (test[1] == 'L') {
                    ans[i][j] = transform(structure, "WALL_LINE", test[2] - '0', tuple(i - get<0>(O), j - get<1>(O)));
                }
                else if (test[1] == 'A') {
                    ans[i][j] = transform(structure, "WALL_ANGLE", test[2] - '0', tuple(i - get<0>(O), j - get<1>(O)));
                }
            }
        }
    }
    return ans;
}

void main() {
    tuple<uint16_t, uint16_t> O;
    vector<vector<string>> LEVEL = loadMap("level/ROOM-00.txt", &O);

    json data = nullptr;
    parseConfig(&data);

    vector<vector<glm::mat4>> transforms = applyConfig(LEVEL, data, O);
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            cout << transforms[8][6][j][i] << ' ';
        }
        cout << endl;
    }
}
