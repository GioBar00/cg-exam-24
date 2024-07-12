#define MAX_LIGHTS 256


/* Uniform buffers. */
struct ObjectUniform {
    alignas(16) glm::mat4   mvpMat;
    alignas(16) glm::mat4   mMat;
    alignas(16) glm::mat4   nMat;
};

struct LightUniform {
    // FIX: Array of uint32_t, instead of glm::vec3, with switch statement inside fragment shader.
    alignas(16) glm::vec3   TYPE[MAX_LIGHTS]; // i := DIRECT, j := POINT, k = SPOT.
    alignas(16) glm::vec3   lightPos[MAX_LIGHTS];
    alignas(16) glm::vec3   lightDir[MAX_LIGHTS];
    alignas(16) glm::vec4   lightCol[MAX_LIGHTS];
    alignas(4)  float       cosIn;
    alignas(4)  float       cosOut;
    alignas(4)  uint32_t    NUMBER;
    alignas(16) glm::vec3   eyePos;
};


/* Vertex formats. */
struct ToonVertex {
    glm::vec3   pos;
    glm::vec3   norm;
    glm::vec2   UV;
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
                    //oi->type = static_cast<SceneObjectType>(is[j]["type"]);
                    oi->type = SceneObjectType::SO_FLOOR; // TODO.
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
    ObjectInstance *player{};

    const float UNIT = 3.1f;

    const float zoom_speed = 0.1f;
    const float max_zoom = 10.0f;
    const float min_zoom = 1.0f;

    const float dampM = 1000.0f;
    const float dampR = 1000.0f;
    const float dampRp = 200.0f;

    void updateObjectBuffer(uint32_t currentImage, Instance *I, glm::mat4 ViewPrj, const std::vector<void *> &gubos) {
        ObjectUniform ubo{};

        ubo.mMat = I->Wm;
        ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
        ubo.mvpMat = ViewPrj * ubo.mMat;

        I->DS[1]->map(currentImage, &ubo, 0);
    }

public:
    void setScene(LevelScene *sc) {
        scene = sc;
    }

    void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) override {
        myMap[coords].push_back(obj);
        if (obj->type == SceneObjectType::SO_PLAYER)
            player = obj;
    }

    void localCleanup() override {
        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                delete obj;
            }
        }
    }

    static bool updatePlayerRotPos(glm::vec3 m, float projRot, float &playerRot, glm::vec3 &playerPos) {
        // TODO: implement the function
        // rotate input vector to match the camera rotation
//        glm::vec4 rotated = glm::rotate(glm::mat4(1.0f), glm::radians(90.f * rot), glm::vec3(0.0f, 1.0f, 0.0f)) *
//                            glm::vec4(m, 1.0f);
//        // get the direction
//        glm::vec2 xz = glm::normalize(glm::vec2(rotated.x, rotated.z));
//        if (glm::length(xz) <= 0.5f) return NONE;
//        if (glm::abs(xz.x) > glm::abs(xz.y)) {
//            return xz.x > 0 ? EAST : WEST;
//        } else {
//            return xz.y > 0 ? NORTH : SOUTH;
//        }
        return false;
    }

    static std::pair<int, int> getAdjacentCell(std::pair<int, int> playerCoords, float playerRot) {
        // TODO: implement the function
        return playerCoords;
    }

    void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire) override {
        static bool isCameraRotating = false;
        static bool isPlayerRotating = false;
        static bool isPlayerMoving = false;
        static bool debounce = false;

        // TODO: Add damping to zoom
        // Calculate Orthogonal Projection Matrix
        static float zoom = 0.5f;
        if (glm::abs(m.y) > 1e-5) {
            zoom = glm::clamp(zoom + m.y * zoom_speed, min_zoom, max_zoom);
            std::cout << "Zoom: " << zoom << "\n";
        }

        const float halfWidth = 10.0f / zoom;
        const float nearPlane = -100.f;
        const float farPlane = 100.0f;

        float Ar = scene->BP->getAr();
        glm::mat4 Prj = glm::scale(glm::mat4(1.0), glm::vec3(1, -1, 1)) *
                        glm::ortho(-halfWidth, halfWidth, -halfWidth / Ar, halfWidth / Ar, nearPlane, farPlane);

        // Calculate View Matrix
        static float currProjRot = 0;
        static float projRot = 0;
        if (!isCameraRotating) {
            // check r.z to move camera
            if (glm::abs(r.z) > 0.5f) {
                projRot += r.z > 0 ? 1 : -1;
                isCameraRotating = true;
                currProjRot = applyDamping(currProjRot, projRot, dampR, deltaT);
                std::cout << "Camera STARTED rotating to " << projRot << "\n";
            }
        } else if (projRot != currProjRot) {
            if (glm::abs(projRot - currProjRot) < 0.01f) {
                currProjRot = projRot;
                isCameraRotating = false;
                std::cout << "Camera ENDED rotating to " << projRot << "\n";
            } else {
                //currProjRot = projRot * exp(-dampR * deltaT) + currProjRot * (1 - exp(-dampR * deltaT));
                currProjRot = applyDamping(currProjRot, projRot, dampR, deltaT);
                //std::cout << "Camera still rotating\n";
            }
        }

        glm::mat4 View = glm::rotate(glm::mat4(1.0f), glm::radians(35.264f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(90.f * currProjRot), glm::vec3(0.0f, 1.0f, 0.0f));

        // FIXME: add player translation to the camera
        glm::mat4 ViewPrj = Prj * View;

        // Player attributes
        static std::pair<int, int> playerCoords = {0, 0};
        static float currPlayerRot = 0.0f;
        static float playerRot = 0.0f;

        static glm::vec3 currPos = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);
        static glm::vec3 playerPos = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);

        // TODO: add player movement controls
        if (!isPlayerMoving && !isPlayerRotating && !isCameraRotating && glm::length(glm::vec2(m.x, m.z)) > 0.5f) {
            if (updatePlayerRotPos(m, projRot, playerRot, playerPos)) {
                // player needs to rotate first
                isPlayerRotating = true;
                currPlayerRot = applyDamping(currPlayerRot, playerRot, dampRp, deltaT);
                std::cout << "Player STARTED rotating to " << playerRot << "\n";
            } else {
                bool canMove = true;
                for (ObjectInstance* obj : myMap[getAdjacentCell(playerCoords, playerRot)]) {
                    if (obj->type == SceneObjectType::SO_WALL) {
                        canMove = false;
                        std::cout << "Player CANNOT move to " << playerCoords.first << ", " << playerCoords.second << "\n";
                        break;
                    }
                }
                if (canMove) {
                    // player needs to move
                    isPlayerMoving = true;
                    playerCoords = getAdjacentCell(playerCoords, playerRot);
                    currPos = applyDamping(currPos, playerPos, dampM, deltaT);
                    std::cout << "Player STARTED moving to " << playerCoords.first << ", " << playerCoords.second << "\n";
                }
            }
        } else {
            if (isPlayerMoving) {
                if (glm::length(playerPos - currPos) < 0.01f) {
                    //player stopped moving to next cell
                    currPos = playerPos;
                    isPlayerMoving = false;
                    std::cout << "Player ENDED moving to " << playerCoords.first << ", " << playerCoords.second << "\n";
                } else {
                    // moving to the next cell
                    //currPos = playerPos * exp(-dampM * deltaT) + currPos * (1 - exp(-dampM * deltaT));
                    currPos = applyDamping(currPos, playerPos, dampM, deltaT);
                    //std::cout << "Player still moving to next cell\n";
                }
            } else if (isPlayerRotating) {
                if (glm::abs(playerRot - currPlayerRot) < 0.01f) {
                    // player stopped rotating
                    currPlayerRot = playerRot;
                    isPlayerRotating = false;
                    std::cout << "Player rotated to " << playerRot << "\n";
                } else {
                    // player rotating
                    //currPlayerRot = playerRot * exp(-dampRp * deltaT) + currPlayerRot * (1 - exp(-dampRp * deltaT));
                    currPlayerRot = applyDamping(currPlayerRot, playerRot, dampRp, deltaT);
                    //std::cout << "Player still rotating\n";
                }
            }
        }

        if (fire) {
            if (!isPlayerMoving && !isPlayerRotating && !debounce) {
                debounce = true;
                std::cout << "Fire\n";
                // TODO: check interactions with other objects
                for (ObjectInstance* obj : myMap[playerCoords]) {
                    if (obj->type == SceneObjectType::SO_LIGHT) {
                        // TODO: Interact with obj
                        break;
                    }
                }
            }
        } else if (debounce) {
            debounce = false;
            std::cout << "Debounce\n";
        }

        // TODO: global uniform buffers first

        // TODO: calculate objects world matrices
        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                switch (obj->type) {
                    case SceneObjectType::SO_FLOOR:
                    case SceneObjectType::SO_WALL:
                    case SceneObjectType::SO_PLAYER:
                    case SceneObjectType::SO_LIGHT:
                        // updateUniformBuffersEmission
                    case SceneObjectType::SO_OTHER:
                        updateObjectBuffer(currentImage, scene->I[scene->InstanceIds[obj->I_id]], ViewPrj, {}); // TODO: add global uniform buffers
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
