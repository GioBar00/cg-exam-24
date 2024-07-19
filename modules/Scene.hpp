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
    alignas(16) glm::vec3 TYPE[MAX_LIGHTS]; // i := DIRECT, j := POINT, k := SPOT.
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

struct BooleanUniform {
    alignas(4) bool isOn;
};

struct UIUniform {
    alignas(4) float x;
    alignas(4) float y;
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

struct ScreenVertex {
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
    bool isOn = false;

    std::string lType;
    glm::vec4 lColor;
    glm::vec3 lDirection;
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

    virtual void init() = 0;

    virtual void localCleanup() = 0;

    virtual void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire,
                                     double cursorX, double cursorY) = 0;
};

class Scene {
protected:
    void addModel(const std::string &id, const std::string &vid, std::vector<unsigned char> vertices,
                  std::vector<unsigned int> indices) {
        MeshIds[id] = ModelCount;
        M[ModelCount] = new Model();
        M[ModelCount]->vertices = std::move(vertices);
        M[ModelCount]->indices = std::move(indices);
        M[ModelCount]->initMesh(BP, VDIds[vid]);
        ModelCount++;
    }

    void addTexture(const std::string &id, const std::string &path) {
        TextureIds[id] = TextureCount;
        T[TextureCount] = new Texture();
        T[TextureCount]->init(BP, path);
        TextureCount++;
    }

    void addInstance(const std::string &id, const std::string &mid, const std::vector<std::string> &tids,
                     int &setsInPool, int &uniformBlocksInPool, int &texturesInPool) {
        int instanceIdx = PI[PipelineInstanceCount].InstanceCount;
        PI[PipelineInstanceCount].I[instanceIdx].id = new std::string(id);
        PI[PipelineInstanceCount].I[instanceIdx].Mid = MeshIds[mid];
        PI[PipelineInstanceCount].I[instanceIdx].NTx = tids.size();
        PI[PipelineInstanceCount].I[instanceIdx].Tid = (int *) calloc(PI[PipelineInstanceCount].I[instanceIdx].NTx,
                                                                      sizeof(int));

        for (int h = 0; h < PI[PipelineInstanceCount].I[instanceIdx].NTx; h++) {
            PI[PipelineInstanceCount].I[instanceIdx].Tid[h] = TextureIds[tids[h]];
        }

        PI[PipelineInstanceCount].I[instanceIdx].Wm = glm::mat4(1.0f);
        PI[PipelineInstanceCount].I[instanceIdx].PI = &PI[PipelineInstanceCount];
        PI[PipelineInstanceCount].I[instanceIdx].D = &PI[PipelineInstanceCount].P->P->D;
        PI[PipelineInstanceCount].I[instanceIdx].NDs = PI[PipelineInstanceCount].I[instanceIdx].D->size();
        setsInPool += PI[PipelineInstanceCount].I[instanceIdx].NDs;

        for (int h = 0; h < PI[PipelineInstanceCount].I[instanceIdx].NDs; h++) {
            DescriptorSetLayout *DSL = (*PI[PipelineInstanceCount].I[instanceIdx].D)[h];
            int DSLsize = DSL->Bindings.size();

            for (int l = 0; l < DSLsize; l++) {
                if (DSL->Bindings[l].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                    uniformBlocksInPool += 1;
                } else {
                    texturesInPool += 1;
                }
            }
        }

        PI[PipelineInstanceCount].InstanceCount++;
        InstanceCount++;
    }

    void addVertices(std::vector<unsigned char>& vertices, int stride, float factor = 0.0f, float ar = 0.0f) const {
        int old_size = vertices.size();
        vertices.resize(old_size + stride * 4);
        auto myVertices = (ScreenVertex*)(&vertices[old_size]);
        int idx = 0;
        for (int i = -1; i <= +1; i += 2) {
            for (int j = -1; j <= +1; j += 2) {
                if (factor == 0.0f)
                    myVertices[idx].pos = { i, j, 0 };
                else
                    myVertices[idx].pos = { i / factor, j / ((ar / BP->getAr()) * factor), 0 };
                myVertices[idx].UV = { (float)(i + 1) / 2, (float)(j + 1) / 2 };
                idx++;
            }
        }
    }

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
        std::cout << "Cleanup textures." << std::endl;
        for (int i = 0; i < TextureCount; i++) {
            T[i]->cleanup();
            delete T[i];
        }
        free(T);

        std::cout << "Cleanup models" << std::endl;
        for (int i = 0; i < ModelCount; i++) {
            M[i]->cleanup();
            delete M[i];
        }
        free(M);

        std::cout << "Cleanup instances" << std::endl;
        for (int i = 0; i < InstanceCount; i++) {
            delete I[i]->id;
            free(I[i]->Tid);
        }
        free(I);

        // To add: delete  also the datastructures relative to the pipeline
        std::cout << "Cleanup pipelines" << std::endl;
        for (int i = 0; i < PipelineInstanceCount; i++) {
            free(PI[i].I);
        }
        free(PI);
        std::cout << "Cleanup scene controller" << std::endl;
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
            {"PLAYER",   SceneObjectType::SO_PLAYER},
            {"GROUND",   SceneObjectType::SO_GROUND},
            {"WALL",     SceneObjectType::SO_WALL},
            {"LIGHT",    SceneObjectType::SO_LIGHT},
            {"TORCH",    SceneObjectType::SO_TORCH},
            {"LAMP",     SceneObjectType::SO_LAMP},
            {"BONFIRE",  SceneObjectType::SO_BONFIRE},
            {"TRAPDOOR", SceneObjectType::SO_TRAPDOOR},
            {"OTHER",    SceneObjectType::SO_OTHER}
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

            M = (Model **) calloc(ModelCount + 1, sizeof(Model *)); // +1 for the skybox
            for (int k = 0; k < ModelCount; k++) {
                MeshIds[ms[k]["id"]] = k;
                std::string MT = ms[k]["format"].template get<std::string>();
                std::string VDN = ms[k]["VD"].template get<std::string>();

                M[k] = new Model();
                M[k]->init(BP, VDIds[VDN], ms[k]["model"], (MT[0] == 'O') ? OBJ : ((MT[0] == 'G') ? GLTF : MGCG));
            }

            // Skybox model
            int mainStride = VDIds["skybox"]->Bindings[0].stride;
            std::vector<unsigned char> vertices{};
            std::vector<unsigned int> indices{};
            addVertices(vertices, mainStride);
            indices = {
                    0, 1, 2,
                    1, 2, 3
            };
            addModel("skybox-m", "skybox", vertices, indices);

            // TEXTURES
            nlohmann::json ts = js["textures"];
            TextureCount = ts.size();
            std::cout << "Textures count: " << TextureCount << "\n";

            T = (Texture **) calloc(TextureCount + 1, sizeof(Texture *)); // +1 for the skybox
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

            // Skybox texture
            addTexture("skybox-tex",
                       "textures/menu/deep-fold.png"); // Credits: https://deep-fold.itch.io/space-background-generator

            // INSTANCES
            nlohmann::json pis = js["instances"];
            PipelineInstanceCount = pis.size();
            std::cout << "Pipeline Instances count: " << PipelineInstanceCount << "\n";
            PI = (PipelineInstances *) calloc(PipelineInstanceCount + 1,
                                              sizeof(PipelineInstances)); // +1 for the skybox
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
                    if (oi->type == SceneObjectType::SO_TORCH || oi->type == SceneObjectType::SO_LAMP ||
                        oi->type == SceneObjectType::SO_BONFIRE) {
                        oi->lType = is[j]["type"];
                        oi->lColor = glm::vec4(is[j]["color"][0], is[j]["color"][1], is[j]["color"][2], 1.0f);
                        oi->lPower = is[j]["power"];
                        oi->lPosition = glm::vec3(is[j]["where"][0], is[j]["where"][1], is[j]["where"][2]);
                    }
                    if (oi->type == SceneObjectType::SO_TORCH)
                        oi->isOn = false;
                    if (oi->type == SceneObjectType::SO_LAMP || oi->type == SceneObjectType::SO_BONFIRE)
                        oi->isOn = true;
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

            // Skybox pipeline instance + object instance
            PI[PipelineInstanceCount].P = PipelineIds["skybox"];
            PI[PipelineInstanceCount].InstanceCount = 0;
            PI[PipelineInstanceCount].I = (Instance *) calloc(1, sizeof(Instance));

            // background instance
            addInstance("skybox-obj", "skybox-m", {"skybox-tex"}, setsInPool, uniformBlocksInPool, texturesInPool);
            PipelineInstanceCount++;

            // Request xInPool
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

        // Add static lights in scene
        auto light1 = new ObjectInstance();
        light1->type = SceneObjectType::SO_LIGHT;
        light1->lType = "SPOT";
        light1->lColor = glm::vec4(1, 1, 1, 1);
        light1->lDirection = glm::vec3(0, -1, 0);
        light1->lPower = 5.0f;
        light1->lPosition = glm::vec3(0, 3, 0);
        SC->addObjectToMap({0, 0}, light1);

        auto light2 = new ObjectInstance();
        light2->type = SceneObjectType::SO_LIGHT;
        light2->lType = "DIRECT";
        light2->lColor = glm::vec4(1, 1, 1, 1);
        light2->lDirection = glm::vec3(0, -0.7, -1);
        light2->lPower = 0.15f;
        SC->addObjectToMap({0, 0}, light2);

        SC->init();

        std::cout << "Leaving scene loading and creation\n";
        return 0;
    }
};

class ScreenScene : public Scene {
    bool isMenu = true;
public:
    void setIsMenu(bool _isMenu) {
        isMenu = _isMenu;
    }

    bool getIsMenu() {
        return isMenu;
    }

    int init(BaseProject *bp, std::vector<VertexDescriptorRef> &VDRs,
             std::vector<PipelineRef> &PRs, const std::string &file) override {
        BP = bp;

        for (auto &VDR: VDRs) {
            VDIds[*VDR.id] = VDR.VD;
        }
        for (auto &PR: PRs) {
            PipelineIds[*PR.id] = &PR;
        }

        // MODELS
        ModelCount = 0;
        M = (Model **) calloc(3, sizeof(Model *));
        int mainStride = VDIds["skybox"]->Bindings[0].stride;
        std::vector<unsigned char> vertices{};
        std::vector<unsigned int> indices{};
        // background model
        addVertices(vertices, mainStride);
        indices = {
                0, 1, 2,
                1, 2, 3
        };
        addModel("bg-m", "skybox", vertices, indices);
        // other models: cursor
        mainStride = VDIds["menu"]->Bindings[0].stride;
        vertices = {};
        float w = 28.0f, h = 28.0f, ar = w / h, factor = 32.0f;
        addVertices(vertices, mainStride, factor, ar);
        addModel("cursor-m", "menu", vertices, indices);
        // other models: buttons
        vertices = {};
        w = 590.0f, h = 260.0f, ar = w / h, factor = 4.0f;
        addVertices(vertices, mainStride, factor, ar);
        addModel("button-m", "menu", vertices, indices);

        // TEXTURES
        TextureCount = 0;
        T = (Texture **) calloc(4, sizeof(Texture *));
        // background texture
        addTexture("bg-tex", isMenu ? "textures/menu/menu-a.png"
                                    : "textures/menu/menu-b.png"); // Credits: https://deep-fold.itch.io/space-background-generator
        // other textures: cursor, buttons
        addTexture("cursor-tex", "textures/menu/cursor.png"); // Credits: https://leo-red.itch.io/lucid-icon-pack
        if (isMenu) {
            addTexture("play-tex-before", "textures/menu/play-btn-before.png");
            addTexture("play-tex-after", "textures/menu/play-btn-after.png");
            // Credits: https://npkuu.itch.io/pixelgui
        } else {
            addTexture("exit-tex-before", "textures/menu/exit-btn-before.png");
            addTexture("exit-tex-after", "textures/menu/exit-btn-after.png");
        }
        // Alternative: https://mounirtohami.itch.io/pixel-art-gui-elements

        // INSTANCES
        PipelineInstanceCount = 0;
        PI = (PipelineInstances *) calloc(2, sizeof(PipelineInstances));
        InstanceCount = 0;
        int setsInPool = 0;
        int uniformBlocksInPool = 0;
        int texturesInPool = 0;
        // background pipeline
        PI[PipelineInstanceCount].P = PipelineIds["skybox"];
        PI[PipelineInstanceCount].InstanceCount = 0;
        PI[PipelineInstanceCount].I = (Instance *) calloc(1,
                                                          sizeof(Instance)); // calculate the number of instances: 1 background, 1 cursor, 2 buttons (one active at each scene)
        // background instance
        addInstance("bg-obj", "bg-m", {"bg-tex"}, setsInPool, uniformBlocksInPool,
                    texturesInPool);

        PipelineInstanceCount++;

        // menu pipeline
        PI[PipelineInstanceCount].P = PipelineIds["menu"];
        PI[PipelineInstanceCount].InstanceCount = 0;
        PI[PipelineInstanceCount].I = (Instance *) calloc(2, sizeof(Instance)); // calculate the number of instances: 1 background, 1 cursor, 2 buttons (one active at each scene)

        // define the other instances: cursor, buttons
        std::vector<std::string> texIds;
        if (isMenu)
            texIds = {"play-tex-before", "play-tex-after"};
        else
            texIds = {"exit-tex-before", "exit-tex-after"};
        addInstance("button-obj", "button-m", texIds, setsInPool, uniformBlocksInPool, texturesInPool);
        auto oi = new ObjectInstance();
        oi->I_id = "button-obj";
        oi->type = SceneObjectType::SO_BUTTON;
        oi->isOn = false;
        SC->addObjectToMap({0, 0}, oi);

        addInstance("cursor-obj", "cursor-m", {"cursor-tex", "cursor-tex"}, setsInPool, uniformBlocksInPool,
                    texturesInPool);
        oi = new ObjectInstance();
        oi->I_id = "cursor-obj";
        oi->type = SceneObjectType::SO_CURSOR;
        SC->addObjectToMap({0, 0}, oi);

        PipelineInstanceCount++;

        // Request xInPool
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

        return 0;
    }
};

/* SCENE CONTROLLERS */
class LevelSceneController : public SceneController {
    LevelScene *scene{};
    std::map<std::pair<int, int>, std::vector<ObjectInstance *>> myMap = {};
    ObjectInstance *torchWithPlayer = nullptr;

    constexpr static const float UNIT = 3.0f;

    const float lightRenderDistance = 40.0f;

    const float zoom_speed = 1.0f;
    const float max_zoom = 8.0f;
    const float min_zoom = 1.5f;

    const float camRotDuration = 1.f;
    const float playerRotDuration = 0.3f;
    const float playerMoveDuration = 0.5f;
    const float playerMoveEaseInDuration = playerMoveDuration;
    const float playerMoveEaseOutDuration = playerMoveDuration;
    const float infoTextDuration = 2.0f;
    const float lightAnimDuration = 4.0f;

    const std::vector<float> lightPowerFactors = {0.90349378, 1.0, 0.89005709, 0.83026667, 0.96448817, 0.91940343,
                                                  0.87150715, 0.79691722, 0.83897976, 0.71733054, 0.78296689,
                                                  0.73759316, 0.5090553, 0.68889212, 0.54618451, 0.55351021, 0.35401893,
                                                  0.38169283, 0.29692011, 0.29330835, 0.37485463, 0.37761337,
                                                  0.20597098, 0.23552444, 0.30538038, 0.25490031, 0.14550135,
                                                  0.22760572, 0.17034215, 0.31292324, 0.37629342, 0.34939741,
                                                  0.44170797, 0.48384355, 0.50728683, 0.49999774, 0.58976117,
                                                  0.57345022, 0.62231734, 0.70364078, 0.81867557, 0.83183688,
                                                  0.82859017, 0.948519, 0.96955134, 0.86614905, 0.88532677, 0.78713641,
                                                  0.84083218, 0.95735339, 0.90842888, 0.98662138, 0.72541265,
                                                  0.97545904, 0.93948324, 0.89309977, 0.86549708, 0.82654344,
                                                  0.67434097, 0.73970837, 0.75666505, 0.65085962, 0.49935446,
                                                  0.65700794, 0.55786411, 0.41534514, 0.38588968, 0.33129121,
                                                  0.21273922, 0.37692862, 0.36292544, 0.25494925, 0.3042671, 0.29094114,
                                                  0.28025327, 0.0562532, 0.24699983, 0.24301981, 0.21985722, 0.26196244,
                                                  0.42582341, 0.43435639, 0.36770523, 0.56848837, 0.6095129, 0.48639306,
                                                  0.62991897, 0.6031358, 0.5060762, 0.83442878, 0.82949084, 0.75172799,
                                                  0.89596749, 1.0, 0.87537598, 0.88267387, 0.92284276, 0.82195823,
                                                  0.94065499, 0.96114116};

    const float playerFloatSpeed = 1.5f;
    const float torchRotationSpeed = 0.5f;

    unsigned int numLitTorches = 0;
    unsigned int numTorches = 0;

    bool isCameraRotating = false;
    bool isPlayerRotating = false;
    bool isPlayerMoving = false;
    bool isPlayerMovingEaseIn = false;
    bool isPlayerMovingEaseOut = false;
    bool debounce = false;

    float zoom = 0.2f;

    std::chrono::high_resolution_clock::time_point camStartTime = std::chrono::high_resolution_clock::now();
    float currProjRot = 0;
    float projRot_old = 0;
    float projRot = 0;

    // Player attributes
    std::chrono::high_resolution_clock::time_point playerStartTime = std::chrono::high_resolution_clock::now();
    std::pair<int, int> playerCoords = {0, 0};
    float currPlayerRot = 0.0f;
    float playerRot_old = 0.0f;
    float playerRot = 0.0f;

    glm::vec3 currPlayerPos = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);
    glm::vec3 playerPos_old = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);
    glm::vec3 playerPos = glm::vec3(playerCoords.first, 0.0f, playerCoords.second);

    bool bringingTorch = false;
    float torchRot = 0;
    glm::mat4 torchPlTr = glm::mat4(1.0f);

    float heightAnimDelta = 0.f;

    std::chrono::high_resolution_clock::time_point infoTextStartTime = std::chrono::high_resolution_clock::now();
    bool infoTextActive = false;

    std::chrono::high_resolution_clock::time_point lightAnimStartTime = std::chrono::high_resolution_clock::now();
    bool animatingLights = false;

    static void updateObjectBuffer(uint32_t currentImage, Instance *I, glm::mat4 ViewPrj, glm::mat4 baseTr,
                                   const std::vector<void *> &gubos, bool spec) {
        ObjectUniform oubo{};

        oubo.mMat = baseTr * I->Wm;
        oubo.nMat = glm::inverse(glm::transpose(oubo.mMat));
        oubo.mvpMat = ViewPrj * oubo.mMat;

        ArgsUniform aubo{};

        aubo.diffuse = true;
        aubo.specular = spec;

        I->DS[1]->map(currentImage, &oubo, 0);
        I->DS[1]->map(currentImage, &aubo, 2);
        I->DS[0]->map(currentImage, gubos[0], 0);
    }

    static void updateSourceBuffer(uint32_t currentImage, Instance *I, ObjectInstance *obj,
                                   glm::mat4 ViewPrj, glm::mat4 baseTr) {
        SourceUniform ubo{};

        ubo.mvpMat = ViewPrj * (baseTr * I->Wm);
        ubo.lightCol = obj->lColor + 0.1f * glm::vec4(1, 1, 0, 1);

        BooleanUniform pubo{};

        pubo.isOn = obj->isOn;

        I->DS[0]->map(currentImage, &ubo, 0);
        I->DS[0]->map(currentImage, &pubo, 2);
    }

    static void updateLightBuffer(uint32_t currentImage, ObjectInstance *obj,
                                  glm::vec3 lPosition, LightUniform *gubo, int idx, float powerFactor) {
        if (obj->lType == "DIRECT")
            gubo->TYPE[idx] = glm::vec3(1, 0, 0);
        else if (obj->lType == "POINT")
            gubo->TYPE[idx] = glm::vec3(0, 1, 0);
        else if (obj->lType == "SPOT")
            gubo->TYPE[idx] = glm::vec3(0, 0, 1);
        gubo->lightPos[idx] = lPosition;
        gubo->lightDir[idx] = glm::vec3(obj->lDirection);
        gubo->lightCol[idx] = obj->lColor;
        gubo->lightPow[idx] = glm::vec3(obj->lPower * powerFactor);
        gubo->NUMBER = idx + 1;
    }

    static auto ease_in_ease_out(auto start, auto target, float timeI) {
        timeI = (3.0f * timeI * timeI) - (2.0f * timeI * timeI * timeI);
        return glm::mix(start, target, timeI);
    }

    static auto ease_in(auto start, auto target, float timeI) {
        return glm::mix(start, target, timeI * timeI);
    }

    static auto ease_out(auto start, auto target, float timeI) {
        return glm::mix(start, target, 1 - (1 - timeI) * (1 - timeI));
    }

public:
    void setScene(LevelScene *sc) {
        scene = sc;
    }

    void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) override {
        myMap[coords].push_back(obj);
        if (obj->type == SceneObjectType::SO_TORCH) {
            numTorches++;
        }
    }

    void init() override {
        // set text for torches
        scene->BP->changeText("Lit Torches: " + std::to_string(numLitTorches) + "/" + std::to_string(numTorches), 0);
    }

    void localCleanup() override {
        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                delete obj;
            }
        }
    }

    static float updatePlayerRot(glm::vec3 m, float projRot, float playerRot) {
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

        return newPlayerRot;
    }

    static glm::vec3 updatePlayerPos(float playerRot, glm::vec3 playerPos, bool half = false) {
        float move = UNIT / 1.5f;
        if (half) {
            move /= 2;
        }

        if (playerRot == 0) {
            playerPos.z -= move;
        } else if (playerRot == glm::pi<float>() / 2) {
            playerPos.x -= move;
        } else if (playerRot == glm::pi<float>()) {
            playerPos.z += move;
        } else if (playerRot == 3 * glm::pi<float>() / 2) {
            playerPos.x += move;
        }

        return playerPos;
    }

    bool willPlayerContinueMoving(glm::vec3 m) {
        if (glm::length(glm::vec2(m.x, m.z)) < 0.5f) {
            return false;
        }

        float newPlayerRot = updatePlayerRot(m, projRot, playerRot);
        if (newPlayerRot != playerRot) {
            return false;
        }

        if (!canPlayerMove()) {
            return false;
        }

        return true;
    }

    bool canPlayerMove() {
        for (ObjectInstance *obj: myMap[getAdjacentCell(playerCoords, playerRot)]) {
            if (obj->type == SceneObjectType::SO_WALL || obj->type == SceneObjectType::SO_BONFIRE) {
                std::cout << "Player CANNOT move to " << getAdjacentCell(playerCoords, playerRot).first << ", "
                          << getAdjacentCell(playerCoords, playerRot).second << "\n";
                return false;
            }
        }
        return true;
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

    void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire, double cursorX,
                             double cursorY) override {
        // Calculate Orthogonal Projection Matrix
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
                currProjRot = ease_in_ease_out(projRot_old, projRot, elapsed / camRotDuration);
                //std::cout << "Camera still rotating\n";
            }
        }

        glm::mat4 View = glm::rotate(glm::mat4(1.0f), glm::radians(35.264f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(90.f * currProjRot), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        glm::mat4 ViewPrj = Prj * View;


        if (!isPlayerMoving && !isPlayerRotating && !isCameraRotating && glm::length(glm::vec2(m.x, m.z)) > 0.5f) {
            float newPlayerRot = updatePlayerRot(m, projRot, playerRot);
            if (newPlayerRot != playerRot) {
                // player needs to rotate first
                isPlayerRotating = true;
                playerRot_old = playerRot;
                playerRot = newPlayerRot;
                playerStartTime = std::chrono::high_resolution_clock::now();
                std::cout << "Player STARTED rotating to " << playerRot << "\n";
            } else {
                if (canPlayerMove()) {
                    // player needs to move
                    isPlayerMoving = true;
                    playerPos = updatePlayerPos(playerRot, playerPos, true);
                    playerStartTime = std::chrono::high_resolution_clock::now();
                    playerCoords = getAdjacentCell(playerCoords, playerRot);
                    std::cout << "Player STARTED moving.\n";
                    isPlayerMovingEaseIn = true;
                }
            }
        } else if (isPlayerMoving || isPlayerRotating) {
            auto currTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currTime - playerStartTime).count();
            if (isPlayerRotating) {
                if (elapsed > playerRotDuration) {
                    // player stopped rotating
                    // mod playerRot to 0-2pi
                    playerRot = glm::mod(playerRot, 2 * glm::pi<float>());
                    currPlayerRot = playerRot_old = playerRot;
                    isPlayerRotating = false;
                    std::cout << "Player ENDED rotating to " << playerRot << "\n";
                } else {
                    // player rotating
                    currPlayerRot = ease_in_ease_out(playerRot_old, playerRot, elapsed / playerRotDuration);
                    //std::cout << "Player still rotating\n";
                }
            } else if (isPlayerMoving) {
                if (isPlayerMovingEaseIn) {
                    if (elapsed >= playerMoveEaseInDuration) {
                        std::cout << "Player EASE IN ENDED\n";
                        isPlayerMovingEaseIn = false;
                        playerPos_old = playerPos;
                        playerStartTime = std::chrono::high_resolution_clock::now();
                        if (!willPlayerContinueMoving(m)) {
                            isPlayerMovingEaseOut = true;
                            playerPos = updatePlayerPos(playerRot, playerPos, true);
                            std::cout << "Player EASE OUT STARTED\n";
                        } else {
                            playerPos = updatePlayerPos(playerRot, playerPos);
                            playerCoords = getAdjacentCell(playerCoords, playerRot);
                            std::cout << "Player CONTINUED moving to " << playerCoords.first << ", "
                                      << playerCoords.second << "\n";
                        }
                    } else {
                        // easing in
                        currPlayerPos = ease_in(playerPos_old, playerPos, elapsed / playerMoveEaseInDuration);
                    }
                } else if (isPlayerMovingEaseOut) {
                    if (elapsed >= playerMoveEaseOutDuration) {
                        std::cout << "Player EASE OUT ENDED\n";
                        isPlayerMovingEaseOut = false;
                        playerPos_old = playerPos;
                        isPlayerMoving = false;
                        std::cout << "Player ENDED moving to " << playerCoords.first << ", " << playerCoords.second
                                  << "\n";
                        // Check end of level
                        bool changeLevel = false;
                        for (ObjectInstance *obj: myMap[playerCoords]) {
                            if (obj->type == SceneObjectType::SO_TRAPDOOR) {
                                changeLevel = true;
                                break;
                            }
                        }
                        if (changeLevel) {
                            // Check if all torches are lit
                            if (numLitTorches == numTorches) {
                                std::cout << "Changing level\n";
                                scene->BP->changeText(" ", 0);
                                SceneId nextScene;
                                if (scene->BP->currSceneId == SceneId::SCENE_LEVEL_1)
                                    nextScene = SceneId::SCENE_LEVEL_2;
                                else
                                    nextScene = SceneId::SCENE_GAME_OVER;
                                scene->BP->changeScene(nextScene);
                            } else {
                                std::cout << "Light all torches: " << numLitTorches << "/" << numTorches << "\n";
                                scene->BP->changeText("Light all torches!", 1);
                                infoTextStartTime = std::chrono::high_resolution_clock::now();
                                infoTextActive = true;
                            }
                        }
                    } else {
                        // easing out
                        currPlayerPos = ease_out(playerPos_old, playerPos, elapsed / playerMoveEaseOutDuration);
                    }
                } else {
                    if (elapsed >= playerMoveDuration) {
                        playerPos_old = playerPos;
                        playerStartTime = std::chrono::high_resolution_clock::now();
                        if (!willPlayerContinueMoving(m)) {
                            isPlayerMovingEaseOut = true;
                            playerPos = updatePlayerPos(playerRot, playerPos, true);
                            std::cout << "Player EASE OUT STARTED\n";
                        } else {
                            playerPos = updatePlayerPos(playerRot, playerPos);
                            playerCoords = getAdjacentCell(playerCoords, playerRot);
                            std::cout << "Player CONTINUED moving to " << playerCoords.first << ", "
                                      << playerCoords.second << "\n";
                        }
                    } else {
                        // player moving
                        currPlayerPos = glm::mix(playerPos_old, playerPos, elapsed / playerMoveDuration);
                    }
                }
            }
        }

        if (fire) {
            if (!isPlayerMoving && !isPlayerRotating && !debounce) {
                debounce = true;
                std::cout << "Fire\n";
                std::cout << "Objects in cell: " << getAdjacentCell(playerCoords, playerRot).first << ", "
                          << getAdjacentCell(playerCoords, playerRot).second << "\n";
                for (ObjectInstance *obj: myMap[getAdjacentCell(playerCoords, playerRot)]) {
                    std::cout << "Found object of type " << levelSceneObjectTypes[obj->type] << "\n";
                    if (obj->type == SceneObjectType::SO_TORCH) {
                        if (!bringingTorch) {
                            bringingTorch = true;
                            std::cout << "Bringing torch\n";
                            torchWithPlayer = obj;
                        } else if (torchWithPlayer->isOn && !obj->isOn) {
                            obj->isOn = true;
                            std::cout << "Lighting torch on wall\n";
                            numLitTorches++;
                            std::cout << "Lit torches: " << numLitTorches << "/" << numTorches << "\n";
                            scene->BP->changeText(
                                    "Lit Torches: " + std::to_string(numLitTorches) + "/" + std::to_string(numTorches),
                                    0);

                        }
                        break;
                    } else if (obj->type == SceneObjectType::SO_BONFIRE) {
                        if (bringingTorch && !torchWithPlayer->isOn) {
                            torchWithPlayer->isOn = true;
                            std::cout << "Lighting torch from bonfire\n";
                            numLitTorches++;
                            std::cout << "Lit torches: " << numLitTorches << "/" << numTorches << "\n";
                            scene->BP->changeText(
                                    "Lit Torches: " + std::to_string(numLitTorches) + "/" + std::to_string(numTorches),
                                    0);
                        }
                    }
                }
            }
        } else if (debounce) {
            debounce = false;
            std::cout << "Debounce\n";
        }

        // check info text
        if (infoTextActive) {
            auto currTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currTime - infoTextStartTime).count();
            if (elapsed > infoTextDuration) {
                infoTextActive = false;
                scene->BP->changeText("", 1);
            }
        }

        // check light animation
        float currPowerFactor = 1.0f;
        if (animatingLights) {
            auto currTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currTime - lightAnimStartTime).count();
            if (elapsed > lightAnimDuration) {
                animatingLights = false;
                lightAnimStartTime = std::chrono::high_resolution_clock::now();
                std::cout << "Light animation ENDED\n";
            } else {
                currPowerFactor = lightPowerFactors[(int) (elapsed / lightAnimDuration * lightPowerFactors.size())];
            }
        } else {
            auto currTime = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(currTime - lightAnimStartTime).count();
            // randomly animate lights with probability 0.01 after 5 seconds
            if (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) < 0.001 && elapsed > 5.0f) {
                animatingLights = true;
                lightAnimStartTime = std::chrono::high_resolution_clock::now();
                std::cout << "Light animation STARTED\n";
            }
        }

        // make camera follow player
        glm::mat4 playerPosTr = glm::translate(glm::mat4(1.0f), currPlayerPos);
        ViewPrj = ViewPrj * glm::inverse(playerPosTr);


        if (torchWithPlayer) {
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
                    !(obj->type == SceneObjectType::SO_TORCH && obj->isOn) &&
                    obj->type != SceneObjectType::SO_LAMP && obj->type != SceneObjectType::SO_BONFIRE)
                    continue;
                if (obj->type == SceneObjectType::SO_TORCH || obj->type == SceneObjectType::SO_LAMP ||
                    obj->type == SceneObjectType::SO_BONFIRE) {
                    if (obj->lType != "DIRECT" && obj != torchWithPlayer &&
                        glm::distance(currPlayerPos, obj->lPosition) > lightRenderDistance)
                        continue;
                }
                glm::vec3 lPosition;
                if (obj == torchWithPlayer)
                    lPosition = glm::vec3(torchPlTr * glm::vec4(obj->lPosition, 1.0f));
                else
                    lPosition = obj->lPosition;
                updateLightBuffer(currentImage, obj, lPosition, &lubo, idx,
                                  obj->type == SceneObjectType::SO_LIGHT ? 1.0f : currPowerFactor);
                idx++;
            }
        }
        lubo.cosIn = glm::cos(glm::radians(30.0f));
        lubo.cosOut = glm::cos(glm::radians(45.0f));

        for (auto &pair: myMap) {
            for (auto &obj: pair.second) {
                glm::mat4 baseTr = glm::mat4(1.0f);
                switch (obj->type) {
                    case SceneObjectType::SO_PLAYER:
                        // make player float up and down
                        heightAnimDelta = heightAnimDelta + playerFloatSpeed * deltaT;
                        // limit modulo to 2pi
                        heightAnimDelta = glm::mod(heightAnimDelta, 2 * glm::pi<float>());
                        baseTr = playerPosTr *
                                 glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.15f * glm::sin(heightAnimDelta), 0)) *
                                 glm::rotate(glm::mat4(1.0f), currPlayerRot, glm::vec3(0, 1, 0));
                    case SceneObjectType::SO_GROUND:
                    case SceneObjectType::SO_TRAPDOOR:
                    case SceneObjectType::SO_WALL:
                        updateObjectBuffer(currentImage, scene->I[scene->InstanceIds[obj->I_id]], ViewPrj, baseTr,
                                           {&lubo}, false);
                        break;
                    case SceneObjectType::SO_OTHER:
                        updateObjectBuffer(currentImage, scene->I[scene->InstanceIds[obj->I_id]], ViewPrj, baseTr,
                                           {&lubo}, true);
                        break;
                    case SceneObjectType::SO_TORCH:
                        if (obj == torchWithPlayer) {
                            baseTr = torchPlTr;
                        }
                    case SceneObjectType::SO_BONFIRE:
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

class ScreenSceneController : public SceneController {
protected:
    ScreenScene *scene{};
    ObjectInstance *cursor = nullptr;
    ObjectInstance *btn = nullptr;

    bool isInsideBtn(double cursorX, double cursorY) {
        auto vertex = scene->M[scene->I[scene->InstanceIds[btn->I_id]]->Mid]->vertices;
        auto myVertexes = (ScreenVertex *) (&vertex[0]);
        return cursorX >= myVertexes[0].pos.x && cursorX <= myVertexes[2].pos.x &&
               cursorY >= myVertexes[0].pos.y && cursorY <= myVertexes[1].pos.y;
    }

public:
    void setScene(ScreenScene *sc) {
        scene = sc;
    }

    void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) override {
        if (obj->type == SceneObjectType::SO_CURSOR) {
            cursor = obj;
        } else if (obj->type == SceneObjectType::SO_BUTTON) {
            btn = obj;
        }
    }

    void init() override {

    }

    void localCleanup() override {
        delete cursor;
        delete btn;
    }

    void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire, double cursorX,
                             double cursorY) override {
        static bool debounce = false;
        static bool isClicked = false;

        if (fire) {
            if (!debounce) {
                debounce = true;
                std::cout << "Fire\n";
                if (isInsideBtn(cursorX, cursorY)) {
                    std::cout << "Button Clicked\n";
                    isClicked = true;
                }
            } else if (!isInsideBtn(cursorX, cursorY)) {
                if (isClicked) {
                    std::cout << "Button Released\n";
                    isClicked = false;
                }
            }
        } else if (debounce) {
            debounce = false;
            std::cout << "Debounce\n";
            if (isInsideBtn(cursorX, cursorY)) {
                if (isClicked) {
                    std::cout << "Button Released\n";
                    isClicked = false;
                    std::cout << "Changing scene\n";
                    SceneId nextScene;
                    if (scene->getIsMenu()) {
                        nextScene = SceneId::SCENE_LEVEL_1;
                        scene->BP->changeScene(nextScene);
                    } else
                        scene->BP->closeWindow();
                }
            }
        }

        UIUniform ubo{};
        ubo.x = cursorX;
        ubo.y = cursorY;
        BooleanUniform pubo{};
        pubo.isOn = false;

        scene->I[scene->InstanceIds[cursor->I_id]]->DS[0]->map(currentImage, &ubo, 1);
        scene->I[scene->InstanceIds[cursor->I_id]]->DS[0]->map(currentImage, &pubo, 2);

        ubo.x = 0.0f;
        ubo.y = 0.0f;
        pubo.isOn = isClicked;

        scene->I[scene->InstanceIds[btn->I_id]]->DS[0]->map(currentImage, &ubo, 1);
        scene->I[scene->InstanceIds[btn->I_id]]->DS[0]->map(currentImage, &pubo, 2);
    }
};

static Scene *getNewSceneById(SceneId sceneId) {
    bool isMenu = true;
    switch (sceneId) {
        case SceneId::SCENE_GAME_OVER:
            isMenu = false;
        case SceneId::SCENE_MAIN_MENU: {
            auto mms = new ScreenScene();
            auto mmsc = new ScreenSceneController();
            mms->setSceneController(mmsc);
            mmsc->setScene(mms);
            mms->setIsMenu(isMenu);
            return mms;
        }
        case SceneId::SCENE_LEVEL_1:
        case SceneId::SCENE_LEVEL_2: {
            auto ls = new LevelScene();
            auto lsc = new LevelSceneController();
            ls->setSceneController(lsc);
            lsc->setScene(ls);
            return ls;
        }
            // Future expansion
//        case SceneId::SCENE_LEVEL_3:
//            return new LevelScene();
        default:
            return nullptr;
    }
}
