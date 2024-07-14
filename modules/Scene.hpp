#define MAX_LIGHTS 32


/* Uniform buffers. */
struct ObjectUniform {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct SourceUniform {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::vec4 lightCol;
};

struct LightUniform {
    // FIXME: Array of uint32_t, instead of glm::vec3, with switch statement inside fragment shader.
    alignas(16) glm::vec3 TYPE[MAX_LIGHTS]; // i := DIRECT, j := POINT, k = SPOT.
    alignas(16) glm::vec3 lightPos[MAX_LIGHTS];
    alignas(16) glm::vec3 lightDir[MAX_LIGHTS];
    alignas(16) glm::vec4 lightCol[MAX_LIGHTS];
    // FIXME: Array of uint32_t, instead of glm::vec3.
    alignas(16) glm::vec3 lightPow[MAX_LIGHTS];
    alignas(4) float cosIn;
    alignas(4) float cosOut;
    alignas(4)  uint32_t NUMBER;
    alignas(16) glm::vec3 eyeDir;
};

struct ArgsUniform {
    alignas(4) bool diffuse;
    alignas(4) bool specular;
};


/* Vertex formats. */
struct ObjectVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
};

struct SourceVertex {
    glm::vec3 pos;
    glm::vec2 UV;
};


/* SCENES */
struct PipelineInstances;

struct Instance {
    std::string *id;
    int Mid;
    int NTx;
    int Iid;
    int *Tid;
    DescriptorSet **DS;
    std::vector<DescriptorSetLayout *> *D;
    int NDs;

    glm::mat4 Wm;
    PipelineInstances *PI;
};

struct PipelineRef {
    std::string *id;
    Pipeline *P;

    void init(const char *_id, Pipeline *_P) {
        id = new std::string(_id);
        P = _P;
    }
};

struct VertexDescriptorRef {
    std::string *id;
    VertexDescriptor *VD;

    void init(const char *_id, VertexDescriptor *_VD) {
        id = new std::string(_id);
        VD = _VD;
    }
};

struct PipelineInstances {
    Instance *I;
    int InstanceCount;
    PipelineRef *P;
};

struct ObjectInstance {
    std::string I_id;
    SceneObjectType type;
    bool isOnFire = false;

    std::string lType;
    glm::vec4 lColor;
    float lPower;
    glm::vec3 lPosition;
};

class Scene;

class SceneController {
protected:
    Scene *scene{};
public:
    Scene *getScene() {
        return scene;
    }

    virtual void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) = 0;

    virtual void localCleanup() = 0;

    virtual void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire) = 0;
};

class Scene {
public:

    BaseProject *BP{};
    SceneController *SC{};

    // Models, textures and Descriptors (values assigned to the uniforms)
    // Please note that Model objects depends on the corresponding vertex structure
    // Models
    int ModelCount = 0;
    Model **M{};
    std::unordered_map<std::string, int> MeshIds;

    // Textures
    int TextureCount = 0;
    Texture **T{};
    std::unordered_map<std::string, int> TextureIds;

    // Descriptor sets and instances
    int InstanceCount = 0;

    Instance **I{};
    std::unordered_map<std::string, int> InstanceIds;

    // Pipelines, DSL and Vertex Formats
    std::unordered_map<std::string, PipelineRef *> PipelineIds;
    int PipelineInstanceCount = 0;
    PipelineInstances *PI{};
    std::unordered_map<std::string, VertexDescriptor *> VDIds;


    virtual int init(BaseProject *_BP, std::vector<VertexDescriptorRef> &VDRs,
                     std::vector<PipelineRef> &PRs, const std::string &file) = 0;

    void setSceneController(SceneController *sc) {
        SC = sc;
    }

    void pipelinesAndDescriptorSetsInit() const {
        std::cout << "Scene DS init\n";
        for (int i = 0; i < InstanceCount; i++) {
            std::cout << "I: " << i << ", NTx: " << I[i]->NTx << ", NDs: " << I[i]->NDs << "\n";
            //Texture ** Tids = (Texture **) calloc(I[i]->NTx, sizeof(Texture *));
            std::vector<Texture *> Tids = std::vector<Texture *>(I[i]->NTx);
            for (int j = 0; j < I[i]->NTx; j++) {
                Tids[j] = T[I[i]->Tid[j]];
            }

            I[i]->DS = (DescriptorSet **) calloc(I[i]->NDs, sizeof(DescriptorSet *));
            for (int j = 0; j < I[i]->NDs; j++) {
                I[i]->DS[j] = new DescriptorSet();
                I[i]->DS[j]->init(BP, (*I[i]->D)[j], Tids);
            }
        }
        std::cout << "Scene DS init Done\n";
    }

    void pipelinesAndDescriptorSetsCleanup() const {
        // Cleanup datasets
        for (int i = 0; i < InstanceCount; i++) {
            for (int j = 0; j < I[i]->NDs; j++) {
                I[i]->DS[j]->cleanup();
                delete I[i]->DS[j];
            }
            free(I[i]->DS);
        }
    }

    void localCleanup() const {
        // Cleanup textures
        for (int i = 0; i < TextureCount; i++) {
            T[i]->cleanup();
            delete T[i];
        }
        free(T);

        // Cleanup models
        for (int i = 0; i < ModelCount; i++) {
            M[i]->cleanup();
            delete M[i];
        }
        free(M);

        for (int i = 0; i < InstanceCount; i++) {
            delete I[i]->id;
            free(I[i]->Tid);
        }
        free(I);

        // To add: delete  also the datastructures relative to the pipeline
        for (int i = 0; i < PipelineInstanceCount; i++) {
            free(PI[i].I);
        }
        free(PI);
        SC->localCleanup();
        free(SC);
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) const {
        for (int k = 0; k < PipelineInstanceCount; k++) {
            for (int i = 0; i < PI[k].InstanceCount; i++) {
                Pipeline *P = PI[k].I[i].PI->P->P;
                P->bind(commandBuffer);

                //std::cout << "Drawing Instance " << i << "\n";
                M[PI[k].I[i].Mid]->bind(commandBuffer);
                //std::cout << "Binding DS: " << DS[i] << "\n";
                for (int j = 0; j < PI[k].I[i].NDs; j++) {
                    PI[k].I[i].DS[j]->bind(commandBuffer, *P, j, currentImage);
                }

                //std::cout << "Draw Call\n";
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M[PI[k].I[i].Mid]->indices.size()), 1, 0, 0, 0);
            }
        }
    }
};

class LevelScene : public Scene {
public:
    std::map<std::string, SceneObjectType> str2enum = {
            {"PLAYER", SceneObjectType::SO_PLAYER},
            {"GROUND", SceneObjectType::SO_GROUND},
            {"WALL",   SceneObjectType::SO_WALL},
            {"LIGHT",  SceneObjectType::SO_LIGHT},
            {"TORCH",  SceneObjectType::SO_TORCH},
            {"LAMP",   SceneObjectType::SO_LAMP},
            {"OTHER",  SceneObjectType::SO_OTHER}
    };

    int init(BaseProject *_BP, std::vector<VertexDescriptorRef> &VDRs,
             std::vector<PipelineRef> &PRs, const std::string &file) override {
        BP = _BP;

        for (auto &VDR: VDRs) {
            VDIds[*VDR.id] = VDR.VD;
        }
        for (auto &PR: PRs) {
            PipelineIds[*PR.id] = &PR;
        }

        // Models, textures and Descriptors (values assigned to the uniforms)
        nlohmann::json js;
        std::string path = "models/" + file;
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            std::cout << "Error! Scene file not found!";
            exit(-1);
        }
        try {
            std::cout << "Parsing JSON\n";
            ifs >> js;
            ifs.close();
            std::cout << "\nScene contains " << js.size() << " definitions sections\n\n";

            // MODELS
            nlohmann::json ms = js["models"];
            ModelCount = ms.size();
            std::cout << "Models count: " << ModelCount << "\n";

            M = (Model **) calloc(ModelCount, sizeof(Model *));
            for (int k = 0; k < ModelCount; k++) {
                MeshIds[ms[k]["id"]] = k;
                std::string MT = ms[k]["format"].template get<std::string>();
                std::string VDN = ms[k]["VD"].template get<std::string>();

                M[k] = new Model();
                M[k]->init(BP, VDIds[VDN], ms[k]["model"], (MT[0] == 'O') ? OBJ : ((MT[0] == 'G') ? GLTF : MGCG));
            }

            // TEXTURES
            nlohmann::json ts = js["textures"];
            TextureCount = ts.size();
            std::cout << "Textures count: " << TextureCount << "\n";

            T = (Texture **) calloc(ModelCount, sizeof(Texture *));
            for (int k = 0; k < TextureCount; k++) {
                TextureIds[ts[k]["id"]] = k;
                std::string TT = ts[k]["format"].template get<std::string>();

                T[k] = new Texture();
                if (TT[0] == 'C') {
                    T[k]->init(BP, ts[k]["texture"]);
                } else if (TT[0] == 'D') {
                    T[k]->init(BP, ts[k]["texture"], VK_FORMAT_R8G8B8A8_UNORM);
                } else {
                    std::cout << "FORMAT UNKNOWN: " << TT << "\n";
                }
                std::cout << ts[k]["id"] << "(" << k << ") " << TT << "\n";
            }

            // INSTANCES TextureCount
            nlohmann::json pis = js["instances"];
            PipelineInstanceCount = pis.size();
            std::cout << "Pipeline Instances count: " << PipelineInstanceCount << "\n";
            PI = (PipelineInstances *) calloc(PipelineInstanceCount, sizeof(PipelineInstances));
            InstanceCount = 0;
            int setsInPool = 0;
            int uniformBlocksInPool = 0;
            int texturesInPool = 0;

            for (int k = 0; k < PipelineInstanceCount; k++) {
                std::string Pid = pis[k]["pipeline"].template get<std::string>();

                PI[k].P = PipelineIds[Pid];
                nlohmann::json is = pis[k]["elements"];
                PI[k].InstanceCount = is.size();
                std::cout << "Pipeline: " << Pid << "(" << k << "), Instances count: " << PI[k].InstanceCount << "\n";
                PI[k].I = (Instance *) calloc(PI[k].InstanceCount, sizeof(Instance));

                for (int j = 0; j < PI[k].InstanceCount; j++) {
                    auto *oi = new ObjectInstance();
                    oi->I_id = is[j]["id"];
                    oi->type = static_cast<SceneObjectType>(str2enum[is[j]["label"]]);
                    // FIXME: Load SO_LIGHT
                    if (oi->type == SceneObjectType::SO_TORCH || oi->type == SceneObjectType::SO_LAMP) {
                        oi->lType = is[j]["type"];
                        oi->lColor = glm::vec4(is[j]["color"][0], is[j]["color"][1], is[j]["color"][2], 1.0f);
                        oi->lPower = is[j]["power"];
                        oi->lPosition = glm::vec3(is[j]["where"][0], is[j]["where"][1], is[j]["where"][2]);
                    }
                    std::pair<int, int> coords = {is[j]["coordinates"][0], is[j]["coordinates"][1]};
                    SC->addObjectToMap(coords, oi);
                    std::cout << k << "." << j << "\t" << is[j]["id"] << ", " << is[j]["model"] << "("
                              << MeshIds[is[j]["model"]] << "), {";
                    PI[k].I[j].id = new std::string(is[j]["id"]);
                    PI[k].I[j].Mid = MeshIds[is[j]["model"]];
                    int NTextures = is[j]["texture"].size();
                    PI[k].I[j].NTx = NTextures;
                    PI[k].I[j].Tid = (int *) calloc(NTextures, sizeof(int));
                    std::cout << "#" << NTextures;
                    for (int h = 0; h < NTextures; h++) {
                        PI[k].I[j].Tid[h] = TextureIds[is[j]["texture"][h]];
                        std::cout << " " << is[j]["texture"][h] << "(" << PI[k].I[j].Tid[h] << ")";
                    }
                    std::cout << "}\n";
                    nlohmann::json TMjson = is[j]["transform"];
                    float TMj[16];
                    for (int h = 0; h < 16; h++) { TMj[h] = TMjson[h]; }
                    PI[k].I[j].Wm = glm::mat4(TMj[0], TMj[4], TMj[8], TMj[12], TMj[1], TMj[5], TMj[9], TMj[13], TMj[2],
                                              TMj[6], TMj[10], TMj[14], TMj[3], TMj[7], TMj[11], TMj[15]);

                    PI[k].I[j].PI = &PI[k];
                    PI[k].I[j].D = &PI[k].P->P->D;
                    PI[k].I[j].NDs = PI[k].I[j].D->size();
                    setsInPool += PI[k].I[j].NDs;

                    for (int h = 0; h < PI[k].I[j].NDs; h++) {
                        DescriptorSetLayout *DSL = (*PI[k].I[j].D)[h];
                        int DSLsize = DSL->Bindings.size();

                        for (int l = 0; l < DSLsize; l++) {
                            if (DSL->Bindings[l].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                                uniformBlocksInPool += 1;
                            } else {
                                texturesInPool += 1;
                            }
                        }
                    }
                    InstanceCount++;
                }
            }

            // Request  xInPool
            BP->requestSetsInPool(setsInPool);
            BP->requestUniformBlocksInPool(uniformBlocksInPool);
            BP->requestTexturesInPool(texturesInPool);

            std::cout << "Creating instances\n";
            I = (Instance **) calloc(InstanceCount, sizeof(Instance *));

            int i = 0;
            for (int k = 0; k < PipelineInstanceCount; k++) {
                for (int j = 0; j < PI[k].InstanceCount; j++) {
                    I[i] = &PI[k].I[j];
                    InstanceIds[*I[i]->id] = i;
                    I[i]->Iid = i;

                    i++;
                }
            }
            std::cout << i << " instances created\n";


        } catch (const nlohmann::json::exception &e) {
            std::cout << "\n\n\nException while parsing JSON file: " << file << "\n";
            std::cout << e.what() << '\n' << '\n';
            std::cout << std::flush;
            return 1;
        }
        std::cout << "Leaving scene loading and creation\n";
        return 0;
    }
};

class MainMenuScene : public Scene {
public:
    int init(BaseProject *bp, std::vector<VertexDescriptorRef> &VDRs,
             std::vector<PipelineRef> &PRs, const std::string &file) override {
        BP = bp;
        // TODO: Implement the MainMenuScene
        return 0;
    }
};

/* SCENE CONTROLLERS */
class LevelSceneController : public SceneController {
    LevelScene *scene{};
    std::map<std::pair<int, int>, std::vector<ObjectInstance *>> myMap;
    ObjectInstance *torchWithPlayer = nullptr;

    constexpr static const float UNIT = 3.1f;

    const float lightRenderDistance = 30.0f;

    const float zoom_speed = 1.0f;
    const float max_zoom = 8.0f;
    const float min_zoom = 0.4f;

    const float camRotDuration = 1.f;
    const float playerRotDuration = 0.3f;
    const float playerMoveDuration = 0.7f;

    const float playerFloatSpeed = 1.5f;
    const float torchRotationSpeed = 0.5f;


    static void updateObjectBuffer(uint32_t currentImage, Instance *I, glm::mat4 ViewPrj, glm::mat4 baseTr,
                                   const std::vector<void *> &gubos) {
        ObjectUniform oubo{};

        oubo.mMat = baseTr * I->Wm;
        oubo.nMat = glm::inverse(glm::transpose(oubo.mMat));
        oubo.mvpMat = ViewPrj * oubo.mMat;

        ArgsUniform aubo{};

        aubo.diffuse = true;
        aubo.specular = false;

        I->DS[1]->map(currentImage, &oubo, 0);
        I->DS[1]->map(currentImage, &aubo, 2);
        I->DS[0]->map(currentImage, gubos[0], 0);
    }

    static void updateSourceBuffer(uint32_t currentImage, Instance *I, ObjectInstance *obj,
                                   glm::mat4 ViewPrj, glm::mat4 baseTr) {
        SourceUniform ubo{};

        ubo.mvpMat = ViewPrj * (baseTr * I->Wm);
        ubo.lightCol = obj->lColor + 0.1f * glm::vec4(1, 1, 0, 1);

        I->DS[0]->map(currentImage, &ubo, 0);
    }

    static void updateLightBuffer(uint32_t currentImage, ObjectInstance *obj,
                                  glm::vec3 lPosition, LightUniform *gubo, int idx) {
        if (obj->lType == "DIRECT")
            gubo->TYPE[idx] = glm::vec3(1, 0, 0);
        else if (obj->lType == "POINT")
            gubo->TYPE[idx] = glm::vec3(0, 1, 0);
        else if (obj->lType == "SPOT")
            gubo->TYPE[idx] = glm::vec3(0, 0, 1);
        gubo->lightPos[idx] = lPosition;
        gubo->lightCol[idx] = obj->lColor;
        gubo->lightPow[idx] = glm::vec3(obj->lPower);
        gubo->NUMBER = idx + 1;
    }

    static auto interpolate(auto start, auto target, float timeI) {
        timeI = (3.0f * timeI * timeI) - (2.0f * timeI * timeI * timeI);
        return glm::mix(start, target, timeI);
    }

public:
    void setScene(LevelScene *sc) {
        scene = sc;
    }

    void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) override {
        myMap[coords].push_back(obj);
    }

    void localCleanup() override {
        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                delete obj;
            }
        }
    }

    static bool updatePlayerRotPos(glm::vec3 m, float projRot, float &playerRot, glm::vec3 &playerPos) {
        // rotate m by projRot
        glm::vec4 rotatedM = glm::rotate(glm::mat4(1.f), glm::radians(-90 * projRot),
                                         glm::vec3(0, 1, 0)) * glm::vec4(m, 1);
        // choose m.x or m.z movement
        glm::vec2 input = glm::normalize(glm::vec2(rotatedM.x, rotatedM.z));
        float newPlayerRot = -1;
        if (input.y > 0.5f) {
            newPlayerRot = glm::pi<float>();
        } else if (input.y < -0.5f) {
            if (playerRot == 3 * glm::pi<float>() / 2)
                newPlayerRot = 2 * glm::pi<float>();
            else
                newPlayerRot = 0.f;
        } else if (input.x > 0.5f) {
            if (playerRot == 0.0f)
                newPlayerRot = -glm::pi<float>() / 2;
            else
                newPlayerRot = 3 * glm::pi<float>() / 2;
        } else if (input.x < -0.5f) {
            newPlayerRot = glm::pi<float>() / 2;
        }

        if (newPlayerRot != playerRot) {
            playerRot = newPlayerRot;
            return true;
        }

        float move = UNIT / 1.5f;

        if (input.y > 0.5f) {
            playerPos.z += move;
        } else if (input.y < -0.5f) {
            playerPos.z -= move;
        } else if (input.x > 0.5f) {
            playerPos.x += move;
        } else if (input.x < -0.5f) {
            playerPos.x -= move;
        }
        return false;
    }

    static std::pair<int, int> getAdjacentCell(std::pair<int, int> playerCoords, float playerRot) {
        int x = playerCoords.first;
        int z = playerCoords.second;
        if (playerRot == 0) {
            z++;
        } else if (playerRot == glm::pi<float>() / 2) {
            x--;
        } else if (playerRot == glm::pi<float>()) {
            z--;
        } else if (playerRot == 3 * glm::pi<float>() / 2) {
            x++;
        }
        return {x, z};
    }

    void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire) override {
        static bool isCameraRotating = false;
        static bool isPlayerRotating = false;
        static bool isPlayerMoving = false;
        static bool debounce = false;

        // Calculate Orthogonal Projection Matrix
        static float zoom = 0.2f;
        if (glm::abs(m.y) > 1e-5) {
            zoom = glm::clamp(zoom + m.y * zoom_speed * deltaT, 0.0f, 1.0f);
            std::cout << "Zoom: " << zoom << "\n";
        }

        const float halfWidth = 10.0f / ((zoom * zoom * (max_zoom - min_zoom)) + min_zoom);
        const float nearPlane = -100.f;
        const float farPlane = 100.0f;

        float Ar = scene->BP->getAr();
        glm::mat4 Prj = glm::scale(glm::mat4(1.0), glm::vec3(1, -1, 1)) *
                        glm::ortho(-halfWidth, halfWidth, -halfWidth / Ar, halfWidth / Ar, nearPlane, farPlane);

        // Calculate View Matrix
        static std::chrono::time_point camStartTime = std::chrono::high_resolution_clock::now();
        static float currProjRot = 0;
        static float projRot_old = 0;
        static float projRot = 0;
        if (!isCameraRotating) {
            // check r.z to move camera
            if (glm::abs(r.z) > 0.5f) {
                projRot += r.z > 0 ? 1 : -1;
                isCameraRotating = true;
                camStartTime = std::chrono::high_resolution_clock::now();
                std::cout << "Camera STARTED rotating to " << projRot << "\n";
            }
        } else {
            auto currTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currTime - camStartTime).count();
            if (elapsed > camRotDuration) {
                currProjRot = projRot_old = projRot;
                isCameraRotating = false;
                std::cout << "Camera ENDED rotating to " << projRot << "\n";
            } else {
                currProjRot = interpolate(projRot_old, projRot, elapsed / camRotDuration);
                //std::cout << "Camera still rotating\n";
            }
        }

        glm::mat4 View = glm::rotate(glm::mat4(1.0f), glm::radians(35.264f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(90.f * currProjRot), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        glm::mat4 ViewPrj = Prj * View;

        // Player attributes
        static std::chrono::time_point playerStartTime = std::chrono::high_resolution_clock::now();
        static std::pair<int, int> playerCoords = {0, 0};
        static float currPlayerRot = 0.0f;
        static float playerRot_old = 0.0f;
        static float playerRot = 0.0f;

        static glm::vec3 currPlayerPos = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);
        static glm::vec3 playerPos_old = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);
        static glm::vec3 playerPos = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);

        if (!isPlayerMoving && !isPlayerRotating && !isCameraRotating && glm::length(glm::vec2(m.x, m.z)) > 0.5f) {
            if (updatePlayerRotPos(m, projRot, playerRot, playerPos)) {
                // player needs to rotate first
                isPlayerRotating = true;
                playerStartTime = std::chrono::high_resolution_clock::now();
                std::cout << "Player STARTED rotating to " << playerRot << "\n";
            } else {
                bool canMove = true;
                for (ObjectInstance *obj: myMap[getAdjacentCell(playerCoords, playerRot)]) {
                    if (obj->type == SceneObjectType::SO_WALL) {
                        canMove = false;
                        std::cout << "Player CANNOT move to " << playerCoords.first << ", " << playerCoords.second
                                  << "\n";
                        break;
                    }
                }
                if (canMove) {
                    // player needs to move
                    isPlayerMoving = true;
                    playerStartTime = std::chrono::high_resolution_clock::now();
                    playerCoords = getAdjacentCell(playerCoords, playerRot);
                    std::cout << "Player STARTED moving to " << playerCoords.first << ", " << playerCoords.second
                              << "\n";
                } else {
                    // reset player position
                    playerPos = currPlayerPos;
                    playerRot = currPlayerRot;
                }
            }
        } else if (isPlayerMoving || isPlayerRotating) {
            auto currTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currTime - playerStartTime).count();
            if (isPlayerMoving) {
                if (elapsed > playerMoveDuration) {
                    //player stopped moving to next cell
                    currPlayerPos = playerPos_old = playerPos;
                    isPlayerMoving = false;
                    std::cout << "Player ENDED moving to " << playerCoords.first << ", " << playerCoords.second << "\n";
                } else {
                    // moving to the next cell
                    currPlayerPos = interpolate(playerPos_old, playerPos, elapsed / playerMoveDuration);
                    //std::cout << "Player still moving to next cell\n";
                }
            } else if (isPlayerRotating) {
                if (elapsed > playerRotDuration) {
                    // player stopped rotating
                    // mod playerRot to 0-2pi
                    playerRot = glm::mod(playerRot, 2 * glm::pi<float>());
                    currPlayerRot = playerRot_old = playerRot;
                    isPlayerRotating = false;
                    std::cout << "Player ENDED rotating to " << playerRot << "\n";
                } else {
                    // player rotating
                    currPlayerRot = interpolate(playerRot_old, playerRot, elapsed / playerRotDuration);
                    //std::cout << "Player still rotating\n";
                }
            }
        }

        static bool bringingTorch = false;

        if (fire) {
            if (!isPlayerMoving && !isPlayerRotating && !debounce) {
                debounce = true;
                std::cout << "Fire\n";
                for (ObjectInstance *obj: myMap[getAdjacentCell(playerCoords, playerRot)]) {
                    if (obj->type == SceneObjectType::SO_TORCH) {
                        if (!bringingTorch && obj->isOnFire) {
                            bringingTorch = true;
                            std::cout << "Bringing torch\n";
                            torchWithPlayer = obj;
                        } else if (!obj->isOnFire) {
                            obj->isOnFire = true;
                            std::cout << "Lighting torch\n";
                        }
                        break;
                    }
                }
            }
        } else if (debounce) {
            debounce = false;
            std::cout << "Debounce\n";
        }

        // make camera follow player
        glm::mat4 playerPosTr = glm::translate(glm::mat4(1.0f), currPlayerPos);
        ViewPrj = ViewPrj * glm::inverse(playerPosTr);

        static glm::mat4 torchPlTr = glm::mat4(1.0f);
        if (torchWithPlayer) {
            static float torchRot = 0;
            torchRot += torchRotationSpeed * deltaT;
            // limit modulo to 2pi
            torchRot = glm::mod(torchRot, 2 * glm::pi<float>());
            torchPlTr = playerPosTr *
                        glm::rotate(glm::mat4(1.0f), torchRot, glm::vec3(0, 1, 0)) *
                        glm::translate(glm::mat4(1.0f), glm::vec3(0, 1, -1)) *
                        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0)) *
                        glm::inverse(scene->I[scene->InstanceIds[torchWithPlayer->I_id]]->Wm);
        }

        LightUniform lubo{};
        lubo.eyeDir = glm::vec3(glm::inverse(View) * glm::vec4(0, 0, 1, 1));
        // std::cout << "EyeDir: " << lubo.eyeDir.x << ", " << lubo.eyeDir.y << ", " << lubo.eyeDir.z << "\n";
        int idx = 0;
        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                if (obj->type != SceneObjectType::SO_LIGHT &&
                    !(obj->type == SceneObjectType::SO_TORCH && obj->isOnFire) && obj->type != SceneObjectType::SO_LAMP)
                    continue;
                if (obj->type == SceneObjectType::SO_TORCH || obj->type == SceneObjectType::SO_LAMP) {
                    if (glm::distance(currPlayerPos, obj->lPosition) > lightRenderDistance)
                        continue;
                }
                glm::vec3 lPosition;
                if (obj == torchWithPlayer)
                    lPosition = glm::vec3(torchPlTr * glm::vec4(obj->lPosition, 1.0f));
                else
                    lPosition = obj->lPosition;
                updateLightBuffer(currentImage, obj, lPosition, &lubo, idx);
                idx++;
            }
        }
//        lubo.TYPE[idx] = glm::vec3(0, 0, 1);
//        lubo.lightPos[idx] = glm::vec3(0, 3, 0);
//        lubo.lightCol[idx] = glm::vec4(1, 1, 1, 1);
//        lubo.lightDir[idx] = glm::vec3(0, -1, 0);
//        lubo.cosIn = glm::cos(glm::radians(60.0f));
//        lubo.cosOut = glm::cos(glm::radians(120.0f));
//        lubo.NUMBER++;

        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                glm::mat4 baseTr = glm::mat4(1.0f);
                switch (obj->type) {
                    case SceneObjectType::SO_PLAYER:
                        static float heightAnimDelta = 0.f;
                        // make player float up and down
                        heightAnimDelta = heightAnimDelta + playerFloatSpeed * deltaT;
                        // limit modulo to 2pi
                        heightAnimDelta = glm::mod(heightAnimDelta, 2 * glm::pi<float>());
                        baseTr = playerPosTr *
                                 glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.15f * glm::sin(heightAnimDelta), 0)) *
                                 glm::rotate(glm::mat4(1.0f), currPlayerRot, glm::vec3(0, 1, 0));
                    case SceneObjectType::SO_GROUND:
                    case SceneObjectType::SO_WALL:
                    case SceneObjectType::SO_OTHER:
                        updateObjectBuffer(currentImage, scene->I[scene->InstanceIds[obj->I_id]], ViewPrj, baseTr,
                                           {&lubo});
                        break;
                    case SceneObjectType::SO_TORCH:
                        if (obj == torchWithPlayer) {
                            baseTr = torchPlTr;
                        }
                    case SceneObjectType::SO_LAMP:
                        updateSourceBuffer(currentImage, scene->I[scene->InstanceIds[obj->I_id]], obj, ViewPrj, baseTr);
                        break;
                    case SceneObjectType::SO_LIGHT:
                        // Light has no model
                        break;
                }
            }
        }
    }
};

class MainMenuSceneController : public SceneController {
protected:
    MainMenuScene *scene{};
public:
    void setScene(MainMenuScene *sc) {
        scene = sc;
    }

    void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) override {

    }

    void localCleanup() override {

    }

    void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire) override {
        // TODO: Implement the MainMenuSceneController
    }
};

static Scene *getNewSceneById(SceneId sceneId) {
    switch (sceneId) {
        case SceneId::SCENE_MAIN_MENU: {
            auto mms = new MainMenuScene();
            auto mmsc = new MainMenuSceneController();
            mms->setSceneController(mmsc);
            mmsc->setScene(mms);
            return mms;
        }
        case SceneId::SCENE_LEVEL_1: {
            auto ls = new LevelScene();
            auto lsc = new LevelSceneController();
            ls->setSceneController(lsc);
            lsc->setScene(ls);
            return ls;
        }
            // Future expansion
//        case SceneId::SCENE_LEVEL_2:
//            return new LevelScene();
        default:
            return nullptr;
    }
}
