#include "modules/Starter.hpp"
using json = nlohmann::json;
using namespace std;

#define UNIT 3.1f
#define OFFSET 0.1f

vector<vector<string>> loadMap(string file, tuple<uint16_t, uint16_t>* O) {
    std::ifstream f(file);
    uint16_t rows, columns;
    f >> rows >> columns;
    vector<vector<string>> LEVEL(rows, std::vector<std::string>(columns, "."));
    for (uint16_t i = 0; i < rows; i++) {
        for (uint16_t j = 0; j < columns; j++) {
            f >> LEVEL[i][j];
            if (LEVEL[i][j] == "S")
                *O = std::tuple(i, j); // Spawn origin.
        }
    }
    f.close();
    return LEVEL;
}

void parseConfig(json* data) {
    std::ifstream f("level/properties.json");
    try {
        *data = json::parse(f);
    }
    catch (const json::parse_error& e) {
        std::cout << e.what() << std::endl;
    }
    f.close();
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
            T = glm::translate(T, UNIT * glm::vec3(get<0>(dist) * v[0], v[1], get<1>(dist) * v[2]));
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

void saveEntry(vector<json>* js, json root, glm::mat4 T, tuple<uint16_t, uint16_t> coords, int id) {
    json j;
    j["model"] = root["model"];
    vector<float> vec;
    for (uint8_t i = 0; i < 4; i++)
        for (uint8_t j = 0; j < 4; j++)
            vec.push_back(T[j][i]);
    j["transform"] = vec;
    j["coordinates"] = { get<0>(coords), get<1>(coords) };
    if (id != -1) {
        json tmp = root["transform"][id]["operations"];
        for (auto x : tmp.get<vector<json>>())
            if (x.begin().key() == "rotate") {
                j["orientation"] = x.begin().value();
                break;
            }
    }
    else
        j["orientation"] = 0;
    (*js).push_back(j);
}

void applyConfig(vector<vector<string>> LEVEL, json data, tuple<uint16_t, uint16_t> O) {
    std::ofstream f("level/out.json", std::ofstream::trunc);
    vector<json> objs;
    json structure = data["structure"], light = data["light"];
    string test;
    glm::mat4 M;
    for (uint16_t i = 0; i < LEVEL.size(); i++) {
        for (uint16_t j = 0; j < LEVEL[0].size(); j++) {
            test = LEVEL[i][j];
            if (test == "S") {
                //transform(structure, "SPAWN", -1);
                continue; // TODO.
            }
            else if (test == "G") {
                M = transform(structure, "GROUND", -1, tuple(i - get<0>(O), j - get<1>(O)));
                saveEntry(&objs, structure["GROUND"], M, tuple(i, j), -1);
            }
            else if (test[0] == 'W') {
                if (test[1] == 'L') {
                    M = transform(structure, "WALL_LINE", test[2] - '0', tuple(i - get<0>(O), j - get<1>(O)));
                    saveEntry(&objs, structure["WALL_LINE"], M, tuple(i, j), test[2] - '0');
                }
                else if (test[1] == 'A') {
                    M = transform(structure, "WALL_ANGLE", test[2] - '0', tuple(i - get<0>(O), j - get<1>(O)));
                    saveEntry(&objs, structure["WALL_ANGLE"], M, tuple(i, j), test[2] - '0');
                }
            }
        }
    }
    json out = json::object();
    out["objs"] = objs;
    f << out.dump(4) << endl;
    f.close();
}

void main() {
    tuple<uint16_t, uint16_t> O;
    vector<vector<string>> LEVEL = loadMap("level/ROOM-00.txt", &O);

    json data = nullptr;
    parseConfig(&data);

    applyConfig(LEVEL, data, O);
    // TODO: "on"/"off" flag at "light"?
}
