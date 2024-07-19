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

struct MenuVertex {
    glm::vec2 pos;
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

    virtual void updateUniformBuffer(uint32_t currentImage, float deltaT, glm::vec3 m, glm::vec3 r, bool fire) = 0;
};

class Scene {
protected:
    void addModel(const std::string &id, const std::string &vid, std::vector<unsigned char> vertices, std::vector<unsigned int> indices) {
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

    void addInstance(const std::string &id, const std::string &mid, const std::string &tid, int &setsInPool, int &uniformBlocksInPool, int &texturesInPool) {
        int instanceIdx = PI[PipelineInstanceCount].InstanceCount;
        PI[PipelineInstanceCount].I[instanceIdx].id = new std::string(id);
        PI[PipelineInstanceCount].I[instanceIdx].Mid = MeshIds[mid];
        PI[PipelineInstanceCount].I[instanceIdx].NTx = 1;
        PI[PipelineInstanceCount].I[instanceIdx].Tid = (int *) calloc(PI[0].I[instanceIdx].NTx, sizeof(int));
        PI[PipelineInstanceCount].I[instanceIdx].Tid[0] = TextureIds[tid];
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
            std::vector<unsigned char> vertices{};
            std::vector<unsigned int> indices{};

            // background model
            int mainStride = VDIds["skybox"]->Bindings[0].stride;
            std::vector<unsigned char> vertex(mainStride, 0);
            auto myVertex = (MenuVertex *) (&vertex[0]);
            for (int i = -1; i <= +1; i += 2) {
                for (int j = -1; j <= +1; j += 2) {
                    myVertex->pos = {i, j};
                    myVertex->UV = {(float) (i + 1) / 2, (float) (j + 1) / 2};
                    vertices.insert(vertices.end(), vertex.begin(), vertex.end());
                }
            }
            indices = {
                    0, 1, 2,
                    1, 2, 3
            };
            addModel("skybox", "skybox", vertices, indices);

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
            addTexture("skybox-tex", "textures/menu/deep-fold.png"); // Credits: https://deep-fold.itch.io/space-background-generator

            // INSTANCES
            nlohmann::json pis = js["instances"];
            PipelineInstanceCount = pis.size();
            std::cout << "Pipeline Instances count: " << PipelineInstanceCount << "\n";
            PI = (PipelineInstances *) calloc(PipelineInstanceCount + 1, sizeof(PipelineInstances)); // +1 for the skybox
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
                    // TODO: Load SO_LIGHT maybe (?)
                    if (oi->type == SceneObjectType::SO_TORCH || oi->type == SceneObjectType::SO_LAMP ||
                        oi->type == SceneObjectType::SO_BONFIRE) {
                        oi->lType = is[j]["type"];
                        oi->lColor = glm::vec4(is[j]["color"][0], is[j]["color"][1], is[j]["color"][2], 1.0f);
                        oi->lPower = is[j]["power"];
                        oi->lPosition = glm::vec3(is[j]["where"][0], is[j]["where"][1], is[j]["where"][2]);
                    }
                    if (oi->type == SceneObjectType::SO_TORCH)
                        oi->isOnFire = false;
                    if (oi->type == SceneObjectType::SO_LAMP || oi->type == SceneObjectType::SO_BONFIRE)
                        oi->isOnFire = true;
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
            addInstance("skybox-obj", "skybox", "skybox", setsInPool, uniformBlocksInPool, texturesInPool);
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

class MainMenuScene : public Scene {
    bool isMenu = true;
public:
    void setIsMenu(bool _isMenu) {
        isMenu = _isMenu;
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
        std::vector<unsigned char> vertices{};
        std::vector<unsigned int> indices{};
        int mainStride = VDIds["menu"]->Bindings[0].stride;
        // background model
        std::vector<unsigned char> vertex(mainStride, 0);
        auto myVertex = (MenuVertex *) (&vertex[0]);
        for (int i = -1; i <= +1; i += 2) {
            for (int j = -1; j <= +1; j += 2) {
                myVertex->pos = {i, j};
                myVertex->UV = {(float) (i + 1) / 2, (float) (j + 1) / 2};
                vertices.insert(vertices.end(), vertex.begin(), vertex.end());
            }
        }
        indices = {
                0, 1, 2,
                1, 2, 3
        };
        addModel("background", "menu", vertices, indices);
        // TODO: Add helper function.
        // other models: cursor
        vertices = {};
        vertex = std::vector<unsigned char>(mainStride, 0);
        myVertex = (MenuVertex*)(&vertex[0]);
        float w = 28.0f, h = 28.0f, ar = w / h, factor = 16.0f;
        for (int i = -1; i <= +1; i += 2) {
            for (int j = -1; j <= +1; j += 2) {
                myVertex->pos = { i / factor, j / ((ar / BP->getAr()) * factor) };
                myVertex->UV = { (float)(i + 1) / 2, (float)(j + 1) / 2 };
                vertices.insert(vertices.end(), vertex.begin(), vertex.end());
            }
        }
        addModel("cursor", "menu", vertices, indices);
        // other models: buttons
        vertices = {};
        vertex = std::vector<unsigned char>(mainStride, 0);
        myVertex = (MenuVertex*)(&vertex[0]);
        w = 590.0f, h = 260.0f, ar = w / h, factor = 4.0f;
        for (int i = -1; i <= +1; i += 2) {
            for (int j = -1; j <= +1; j += 2) {
                myVertex->pos = { i / factor, j / ((ar / BP->getAr()) * factor) };
                myVertex->UV = { (float)(i + 1) / 2, (float)(j + 1) / 2 };
                vertices.insert(vertices.end(), vertex.begin(), vertex.end());
            }
        }
        addModel("button", "menu", vertices, indices);

        // TEXTURES
        TextureCount = 0;
        T = (Texture **) calloc(3, sizeof(Texture *));
        // background texture
        addTexture("background-tex", isMenu ? "textures/menu/menu-a.png" : "textures/menu/menu-b.png"); // Credits: https://deep-fold.itch.io/space-background-generator
        // other textures: cursor, buttons
        addTexture("cursor-tex", "textures/menu/cursor.png"); // Credits: https://leo-red.itch.io/lucid-icon-pack
        if (isMenu) {
            addTexture("play-tex-before", "textures/menu/play-btn-before.png");
            addTexture("play-tex-after", "textures/menu/play-btn-after.png");
            // Credits: https://npkuu.itch.io/pixelgui
        }
        else {
            addTexture("exit-tex-before", "textures/menu/exit-btn-before.png");
            addTexture("exit-tex-after", "textures/menu/exit-btn-after.png");
        }
        // Alternative: https://mounirtohami.itch.io/pixel-art-gui-elements

        // INSTANCES
        PipelineInstanceCount = 0;
        PI = (PipelineInstances *) calloc(1, sizeof(PipelineInstances));
        InstanceCount = 0;
        int setsInPool = 0;
        int uniformBlocksInPool = 0;
        int texturesInPool = 0;
        // background instance
        PI[PipelineInstanceCount].P = PipelineIds["menu"];
        PI[PipelineInstanceCount].InstanceCount = 0;
        PI[PipelineInstanceCount].I = (Instance *) calloc(3, sizeof(Instance)); // calculate the number of instances: 1 background, 1 cursor, 2 buttons (one active at each scene)

        // background instance
        addInstance("background-obj", "background", "background-tex", setsInPool, uniformBlocksInPool, texturesInPool);
        // define the other instances: cursor, buttons
        addInstance("btn-obj", "button", isMenu ? "play-tex-before" : "exit-tex-before", setsInPool, uniformBlocksInPool, texturesInPool);
        addInstance("cursor-obj", "cursor", "cursor-tex", setsInPool, uniformBlocksInPool, texturesInPool);

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
    const float playerMoveDuration = 0.7f;
    const float infoTextDuration = 2.0f;

    const float playerFloatSpeed = 1.5f;
    const float torchRotationSpeed = 0.5f;

    unsigned int numLitTorches = 0;
    unsigned int numTorches = 0;

    bool isCameraRotating = false;
    bool isPlayerRotating = false;
    bool isPlayerMoving = false;
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

        pubo.isOn = obj->isOnFire;

        I->DS[0]->map(currentImage, &ubo, 0);
        I->DS[0]->map(currentImage, &pubo, 2);
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
        gubo->lightDir[idx] = glm::vec3(obj->lDirection);
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
                currProjRot = interpolate(projRot_old, projRot, elapsed / camRotDuration);
                //std::cout << "Camera still rotating\n";
            }
        }

        glm::mat4 View = glm::rotate(glm::mat4(1.0f), glm::radians(35.264f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::rotate(glm::mat4(1.0f), glm::radians(90.f * currProjRot), glm::vec3(0.0f, 1.0f, 0.0f)) *
                         glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        glm::mat4 ViewPrj = Prj * View;


        if (!isPlayerMoving && !isPlayerRotating && !isCameraRotating && glm::length(glm::vec2(m.x, m.z)) > 0.5f) {
            if (updatePlayerRotPos(m, projRot, playerRot, playerPos)) {
                // player needs to rotate first
                isPlayerRotating = true;
                playerStartTime = std::chrono::high_resolution_clock::now();
                std::cout << "Player STARTED rotating to " << playerRot << "\n";
            } else {
                bool canMove = true;
                for (ObjectInstance *obj: myMap[getAdjacentCell(playerCoords, playerRot)]) {
                    if (obj->type == SceneObjectType::SO_WALL || obj->type == SceneObjectType::SO_BONFIRE) {
                        canMove = false;
                        std::cout << "Player CANNOT move to " << getAdjacentCell(playerCoords, playerRot).first << ", "
                                  << getAdjacentCell(playerCoords, playerRot).second << "\n";
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
                            // FIXME: change to end screen if level 2
                            SceneId nextScene;
                            if (scene->BP->currSceneId == SceneId::SCENE_LEVEL_1)
                                nextScene = SceneId::SCENE_LEVEL_2;
                            else
                                nextScene = SceneId::SCENE_LEVEL_1;
                            scene->BP->changeScene(nextScene);
                        } else {
                            std::cout << "Light all torches: " << numLitTorches << "/" << numTorches << "\n";
                            scene->BP->changeText("Light all torches!", 1);
                            infoTextStartTime = std::chrono::high_resolution_clock::now();
                            infoTextActive = true;
                        }
                    }
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

        if (fire) {
            if (!isPlayerMoving && !isPlayerRotating && !debounce) {
                debounce = true;
                std::cout << "Fire\n";
                std::cout << "Objects in cell: " << getAdjacentCell(playerCoords, playerRot).first << ", "
                          << getAdjacentCell(playerCoords, playerRot).second << "\n";
                for (ObjectInstance *obj: myMap[getAdjacentCell(playerCoords, playerRot)]) {
                    std::cout << "Found object of type " << sceneObjectTypes[obj->type] << "\n";
                    if (obj->type == SceneObjectType::SO_TORCH) {
                        if (!bringingTorch) {
                            bringingTorch = true;
                            std::cout << "Bringing torch\n";
                            torchWithPlayer = obj;
                        } else if (torchWithPlayer->isOnFire && !obj->isOnFire) {
                            obj->isOnFire = true;
                            std::cout << "Lighting torch on wall\n";
                            numLitTorches++;
                            std::cout << "Lit torches: " << numLitTorches << "/" << numTorches << "\n";
                            scene->BP->changeText("Lit Torches: " + std::to_string(numLitTorches) + "/" + std::to_string(numTorches), 0);

                        }
                        break;
                    } else if (obj->type == SceneObjectType::SO_BONFIRE) {
                        if (bringingTorch && !torchWithPlayer->isOnFire) {
                            torchWithPlayer->isOnFire = true;
                            std::cout << "Lighting torch from bonfire\n";
                            numLitTorches++;
                            std::cout << "Lit torches: " << numLitTorches << "/" << numTorches << "\n";
                            scene->BP->changeText("Lit Torches: " + std::to_string(numLitTorches) + "/" + std::to_string(numTorches), 0);
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
                    !(obj->type == SceneObjectType::SO_TORCH && obj->isOnFire) &&
                    obj->type != SceneObjectType::SO_LAMP && obj->type != SceneObjectType::SO_BONFIRE)
                    continue;
                if (obj->type == SceneObjectType::SO_TORCH || obj->type == SceneObjectType::SO_LAMP ||
                    obj->type == SceneObjectType::SO_BONFIRE) {
                    if (obj->lType != "DIRECT" && obj != torchWithPlayer && glm::distance(currPlayerPos, obj->lPosition) > lightRenderDistance)
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

class MainMenuSceneController : public SceneController {
protected:
    MainMenuScene *scene{};
public:
    void setScene(MainMenuScene *sc) {
        scene = sc;
    }

    void addObjectToMap(std::pair<int, int> coords, ObjectInstance *obj) override {
        throw std::runtime_error("MainMenuSceneController::addObjectToMap not implemented");
    }

    void init() override {

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
