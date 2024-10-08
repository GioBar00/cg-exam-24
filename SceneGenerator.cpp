#include <iostream>
#include <cmath>
using namespace std;
#include "modules/Starter.hpp"
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

#define FILE_PATH string("levels/tutorial/")
#define CONFIG_PATH "levels/properties.json"
#define MODEL_PATH string("models/dungeon/")
#define TEMPLATE_PATH "models/scene.json"
#define RETURN_PATH "scenes/level-01.json"

enum {
    NO_ID = -1,
    LIGHT_MODE = -2
};

const float_t UNIT = 3.0f, OFFSET = 0.1f, RAND = 0.2f;

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

void parseConfig(ordered_json* data) {
    std::ifstream f(CONFIG_PATH);
    try {
        *data = ordered_json::parse(f);
    }
    catch (const ordered_json::parse_error& e) {
        std::cout << e.what() << std::endl;
    }
    f.close();
}

glm::mat4 transform(ordered_json root, string type, uint8_t id, tuple<int16_t, int16_t> dist, glm::mat4* trans, float_t* rot) {
    ordered_json branch;
    if (type == "SPAWN")
        branch = root;
    else
        branch = root[type];
    ordered_json ops = branch["transform"], flag = branch["variant"];
    if (flag != nullptr && flag.get<bool>())
        ops = ops[id]["operations"];
    vector<ordered_json> lst = ops.get<vector<ordered_json>>();
    glm::mat4 T = glm::mat4(1);
    string t;
    for (auto x : lst) {
        t = x.begin().key();
        if (t == "translate") {
            array<float_t, 3> v = x[t].get<array<float_t, 3>>();
            if (x["unit"] == nullptr || x["unit"] == "U") {
                flag = x["factor"];
                if (flag == nullptr || flag.get<bool>())
                    T = glm::translate(T, UNIT * glm::vec3(get<0>(dist) * v[0], v[1], get<1>(dist) * v[2]));
                else if (flag != nullptr && !flag.get<bool>())
                    T = glm::translate(T, UNIT * glm::vec3(v[0], v[1], v[2]));
            }
            else if (x["unit"] == "ALT") {
                flag = x["mask"];
                if (flag == nullptr || !flag.get<bool>())
                    T = glm::translate(T, OFFSET * glm::vec3(v[0], v[1], v[2]));
                else if (flag != nullptr && flag.get<bool>())
                    T = glm::translate(T, OFFSET * glm::vec3(glm::cos(glm::radians(*rot + 90)) * v[0], v[1], glm::sin(glm::radians(*rot + 90)) * v[2]));
            }
            else if (x["unit"] == "NONE")
                    T = glm::translate(T, glm::vec3(v[0], v[1], v[2]));
            if (trans != nullptr)
                *trans = T;
        }
        else if (t == "rotate") {
            float_t ang = x[t].get<float_t>();
            T = glm::rotate(T, glm::radians(ang), glm::vec3(0, 1, 0));
            if (rot != nullptr)
                *rot = ang;
        }
        else if (t == "scale") {
            array<float_t, 3> v = x[t].get<array<float_t, 3>>();
            T = glm::scale(T, glm::vec3(v[0], v[1], v[2]));
        }
        else if (t == "random") {
            string f = x[t].get<string>();
            if (f == "TRANSLATION")
                T = glm::translate(T, RAND * glm::vec3(-1.0f + 2 * ((float)rand() / RAND_MAX), 0.0f, -1.0 + 2 * ((float)rand() / RAND_MAX)));
            else if (f == "ROTATION")
                T = glm::rotate(T, glm::radians(360.0f * ((float)rand() / RAND_MAX)), glm::vec3(0, 1, 0));
        }
    }
    return T;
}

void saveEntry(ordered_json jt, vector<ordered_json>* js, ordered_json root, const string& label, glm::mat4 T, tuple<int16_t, int16_t> coords, int8_t id, float_t orient) {
    ordered_json j = ordered_json::object();
    vector<float_t> vec;
    for (uint8_t i = 0; i < 4; i++)
        for (uint8_t j = 0; j < 4; j++)
            vec.push_back(T[j][i]);
    j["transform"] = vec;
    j["coordinates"] = { get<0>(coords), get<1>(coords) };
    switch (id) {
        case NO_ID:
            j["orientation"] = 0;
            break;
        case LIGHT_MODE:
            j["orientation"] = orient;
            break;
        default:
            ordered_json tmp = root["transform"][id]["operations"];
            for (auto x : tmp.get<vector<ordered_json>>())
                if (x.begin().key() == "rotate") {
                    j["orientation"] = x.begin().value();
                    break;
                }
            break;
    }
    for (auto x : jt["models"].get<vector<ordered_json>>())
        if (x["model"].get<string>() == MODEL_PATH + root["model"].get<string>() + ".mgcg") {
            j["model"] = x["id"];
            break;
        }
    j["texture"] = ordered_json::array({ jt["textures"][0]["id"] });
    j["id"] = j["model"].get<string>() + "-" + (std::signbit(static_cast<float_t>(get<0>(coords))) ? "n" : "p") + to_string(std::abs(get<0>(coords))) + (std::signbit(static_cast<float_t>(get<1>(coords))) ? "n" : "p") + to_string(std::abs(get<1>(coords)));
    j["label"] = label;
    if (id == LIGHT_MODE) {
        j["type"] = root["type"];
        j["color"] = root["color"];
        j["power"] = root["power"];
        j["where"] = array<float_t, 3> { T[3][0] + glm::cos(glm::radians(orient + 90)) * root["lumen-transform"][0].get<float_t>(), T[3][1] + root["lumen-transform"][1].get<float_t>(), T[3][2] + glm::sin(glm::radians(orient + 90)) * root["lumen-transform"][2].get<float_t>() };
    }
    (*js).push_back(j);
}

void applyConfig(vector<vector<string>> LEVEL, vector<vector<string>> LIGHT, vector<vector<string>> ADDON, uint8_t mod, ordered_json data, tuple<uint16_t, uint16_t> O, bool reset) {
    std::ifstream ft(TEMPLATE_PATH);
    ordered_json jtemplate = ordered_json::parse(ft);
    ft.close();
    vector<ordered_json> objs;
    ordered_json structure = data["structure"], light = data["light"], addon = data["addon"];
    glm::mat4 inheritTrans;
    float_t inheritRot;
    string test;
    glm::mat4 M;
    uint16_t rows = LEVEL.size(), columns = LEVEL[0].size();
    for (uint16_t i = 0; i < rows; i++) {
        for (uint16_t j = 0; j < columns; j++) {
            test = LEVEL[i][j];
            if (test == "S") {
                M = transform(structure["SPAWN"][0], "SPAWN", NO_ID, tuple(0, 0), nullptr, nullptr);
                saveEntry(jtemplate, &objs, structure["SPAWN"][0], "SPAWN", M, tuple(0, 0), NO_ID, (float_t)NULL);
                M = transform(structure["SPAWN"][1], "SPAWN", NO_ID, tuple(0, 0), nullptr, nullptr);
                saveEntry(jtemplate, &objs, structure["SPAWN"][1], "GROUND", M, tuple(0, 0), NO_ID, (float_t)NULL);
            }
            else if (test == "G") {
                M = transform(structure, "GROUND", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr); // Stack sublevels on rows.
                saveEntry(jtemplate, &objs, structure["GROUND"], "GROUND", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test == "E") {
                M = transform(structure, "TRAPDOOR", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, structure["TRAPDOOR"], "TRAPDOOR", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test[0] == 'W') {
                if (test[1] == 'L') {
                    M = transform(structure, "WALL_LINE", test[2] - '0', tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), &inheritTrans, &inheritRot);
                    saveEntry(jtemplate, &objs, structure["WALL_LINE"], "WALL", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), test[2] - '0', (float_t)NULL);
                }
                else if (test[1] == 'A') {
                    M = transform(structure, "WALL_ANGLE", test[2] - '0', tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), &inheritTrans, &inheritRot);
                    saveEntry(jtemplate, &objs, structure["WALL_ANGLE"], "WALL", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), test[2] - '0', (float_t)NULL);
                }
            }
            test = LIGHT[i][j];
            if (test == "T") {
                M = transform(light, "TORCH", NO_ID, tuple(INT16_MAX, INT16_MAX), nullptr, &inheritRot) * inheritTrans;
                M = glm::rotate(M, glm::radians(inheritRot), glm::vec3(0, 1, 0));
                saveEntry(jtemplate, &objs, light["TORCH"], "TORCH", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), LIGHT_MODE, inheritRot);
            }
            else if (test[0] == 'L') {
                M = transform(light, "LAMP_" + string(1, test[1]), NO_ID, tuple(INT16_MAX, INT16_MAX), nullptr, &inheritRot) * inheritTrans;
                M = glm::rotate(M, glm::radians(inheritRot), glm::vec3(0, 1, 0));
                saveEntry(jtemplate, &objs, light["LAMP_" + string(1, test[1])], "LAMP", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), LIGHT_MODE, inheritRot);
            } else if(test == "B") {
                M = transform(light, "BONFIRE", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, light["BONFIRE"], "BONFIRE", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), LIGHT_MODE, (float_t)NULL);
            }
            test = ADDON[i][j];
            if (test == "R") {
                M = transform(addon, "ROAD", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, addon["ROAD"], "OTHER", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test == "D") {
                M = transform(addon, "BARREL", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, addon["BARREL"], "OTHER", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test == "C") {
                M = transform(addon, "CHEST", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, addon["CHEST"], "OTHER", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test == "M") {
                M = transform(addon, "CRYSTAL", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, addon["CRYSTAL"], "OTHER", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test == "P") {
                M = transform(addon, "POT", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, addon["POT"], "OTHER", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
            else if (test == "F") {
                M = transform(addon, "SKULL", NO_ID, tuple(j - get<1>(O), -mod * rows + i - get<0>(O)), nullptr, nullptr);
                saveEntry(jtemplate, &objs, addon["SKULL"], "OTHER", M, tuple(j - get<1>(O), -(-mod * rows + i - get<0>(O))), NO_ID, (float_t)NULL);
            }
        }
    }
    std::ifstream fin(reset ? TEMPLATE_PATH : RETURN_PATH);
    ordered_json out = ordered_json::parse(fin);
    fin.close();
    for (auto x : objs) {
        for (auto y : out["models"].get<vector<ordered_json>>()) {
            if (x["model"].get<string>() == y["id"].get<string>()) {
                vector<ordered_json> tmp = out["instances"].get<vector<ordered_json>>();
                ordered_json z;
                for (uint32_t it = 0; it < tmp.size(); it++) {
                    z = tmp[it];
                    if (y["P"].get<string>() == z["pipeline"].get<string>()) {
                        out["instances"][it]["elements"].push_back(x);
                        break;
                    }
                }
                break;
            }
        }
    }
    std::ofstream fout(RETURN_PATH, std::ofstream::trunc);
    fout << out.dump(4) << endl;
    fout.close();
}

int main() {
    tuple<uint16_t, uint16_t> O;
    vector<vector<string>> LEVEL = loadMap(FILE_PATH + "ROOM-00.txt", &O), LIGHT = loadMap(FILE_PATH + "LIGHT-00.txt", nullptr), ADDON = loadMap(FILE_PATH + "ADDON-00.txt", nullptr);

    ordered_json data = NULL;
    parseConfig(&data);

    srand(42);

    applyConfig(LEVEL, LIGHT, ADDON, 0, data, O, true);

    uint8_t ROOMS = 2;
    string k;
    for (uint8_t n = 1; n <= ROOMS; n++) {
        k = std::to_string(n);
        if (n < 10)
            k = string(1, '0') + k;
        LEVEL = loadMap(FILE_PATH + "ROOM-" + k + ".txt", nullptr);
        LIGHT = loadMap(FILE_PATH + "LIGHT-" + k + ".txt", nullptr);
        ADDON = loadMap(FILE_PATH + "ADDON-" + k + ".txt", nullptr);
        applyConfig(LEVEL, LIGHT, ADDON, n, data, O, false);
    }
    return 0;
}
