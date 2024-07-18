#include "modules/Starter.hpp"
#include "modules/AppCommon.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"

#define HIDE_TEXT false


class App : public BaseProject {
protected:

    /* Descriptor set layouts. */
    DescriptorSetLayout ObjectDSL, SourceDSL, LightDSL, MenuDSL;


    /* Vertex descriptors. */
    VertexDescriptor ObjectVD, SourceVD, MenuVD;


    /* Pipelines. */
    Pipeline ToonP, PhongP, SourceP, MenuP;


    /* Texts. */
    std::vector<SingleText> out = {
        { 0, {nullptr, nullptr, nullptr, nullptr}, 0, 0 }
    };
    TextMaker txt;


    /* Scenes */
    std::unordered_map<SceneId, Scene *> scenes;
    std::unordered_map<SceneId, std::vector<VertexDescriptorRef>> SceneVDRs;
    std::unordered_map<SceneId, std::vector<PipelineRef>> ScenePRs;


    // Application config.
    SceneId newSceneId;

    bool changingScene = false;
    bool updateText = false;

    void setWindowParameters() override {
        windowWidth = 1200;
        windowHeight = 900;
        windowTitle = "CG24 @ PoliMi";
        windowResizable = GLFW_FALSE;
        initialBackgroundColor = {0.05f, 0.05f, 0.05f, 1.0f};

        Ar = (float) windowWidth / (float) windowHeight;
    }

    void onWindowResize(int w, int h) override {
        Ar = (float) w / (float) h;
    }


    void localInit() override {
        // Set scenes map to nullptr for each sceneId
        for (const auto &sceneId: sceneIds) {
            scenes[sceneId] = nullptr;
        }


        ObjectDSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_VERTEX_BIT,     sizeof(ObjectUniform),  1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT,   0,                      1},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_FRAGMENT_BIT,   sizeof(ArgsUniform),    1}
        });
        SourceDSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_ALL_GRAPHICS,   sizeof(SourceUniform),  1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT,   0,                      1},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_FRAGMENT_BIT,   sizeof(BooleanUniform), 1}
        });
        LightDSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_FRAGMENT_BIT,   sizeof(LightUniform),   1}
		});
        MenuDSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT,   0,  1},
        });

        ObjectVD.init(this, {
                {0, sizeof(ObjectVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ObjectVertex, pos),    sizeof(glm::vec3),  POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ObjectVertex, norm),   sizeof(glm::vec3),  NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,     offsetof(ObjectVertex, UV),     sizeof(glm::vec2),  UV}
            }
        );
        SourceVD.init(this, {
                {0, sizeof(SourceVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,  offsetof(SourceVertex, pos),    sizeof(glm::vec3),  POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT,     offsetof(SourceVertex, UV),     sizeof(glm::vec2),  UV}
            }
        );
        MenuVD.init(this, {
                {0, sizeof(MenuVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(MenuVertex, pos),  sizeof(glm::vec2),  POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(MenuVertex, UV),   sizeof(glm::vec2),  UV}
            }
        );

        ToonP.init(this, &ObjectVD, "shaders/Shader.vert.spv", "shaders/Toon.frag.spv", {&LightDSL, &ObjectDSL});
        PhongP.init(this, &ObjectVD, "shaders/Shader.vert.spv", "shaders/Phong.frag.spv", {&LightDSL, &ObjectDSL});
        SourceP.init(this, &SourceVD, "shaders/Emission.vert.spv", "shaders/Emission.frag.spv", {&SourceDSL});
        MenuP.init(this, &MenuVD, "shaders/Menu.vert.spv", "shaders/Menu.frag.spv", {&MenuDSL});
        MenuP.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, true);

        std::string s = " ";
        out[0].l[0] = (char *) malloc(s.size() + 1);
        strcpy(out[0].l[0], s.c_str());
        out[0].usedLines = 1;
        txt.init(this, &out);

        // Define vertex descriptor references per scene.
        VertexDescriptorRef ObjectVDR{}, SourceVDR{}, MenuVDR{};
        ObjectVDR.init("object", &ObjectVD);
        SourceVDR.init("source", &SourceVD);
        std::vector<VertexDescriptorRef> LevelSceneVDRs = {ObjectVDR, SourceVDR };

        MenuVDR.init("menu", &MenuVD);
        std::vector<VertexDescriptorRef> MenuVDRs = { MenuVDR };

        // Define pipeline references per scene.
        PipelineRef ToonPR{}, PhongPR{}, SourcePR{}, MenuPR{};
        ToonPR.init("toon", &ToonP);
        PhongPR.init("phong", &PhongP);
        SourcePR.init("emission", &SourceP);
        std::vector<PipelineRef> LevelScenePRs = {ToonPR, PhongPR, SourcePR };

        MenuPR.init("menu", &MenuP);
        std::vector<PipelineRef> MenuPRs = { MenuPR };

        SceneVDRs[SceneId::SCENE_MAIN_MENU] = MenuVDRs;
        ScenePRs[SceneId::SCENE_MAIN_MENU] = MenuPRs;
        SceneVDRs[SceneId::SCENE_LEVEL_1] = LevelSceneVDRs;
        ScenePRs[SceneId::SCENE_LEVEL_1] = LevelScenePRs;
        SceneVDRs[SceneId::SCENE_LEVEL_2] = LevelSceneVDRs;
        ScenePRs[SceneId::SCENE_LEVEL_2] = LevelScenePRs;


        // Set the first scene
        currSceneId = SceneId::SCENE_MAIN_MENU;
        changeScene(currSceneId);
        changingScene = false;
    }

    void pipelinesAndDescriptorSetsInit() override {
        ToonP.create();
        PhongP.create();
        SourceP.create();
        txt.pipelinesAndDescriptorSetsInit();
        MenuP.create();
        scenes[currSceneId]->pipelinesAndDescriptorSetsInit();
    }

    void pipelinesAndDescriptorSetsCleanup() override {
        ToonP.cleanup();
        PhongP.cleanup();
        SourceP.cleanup();
        txt.pipelinesAndDescriptorSetsCleanup();
        MenuP.cleanup();
        scenes[currSceneId]->pipelinesAndDescriptorSetsCleanup();
    }

    void localCleanup() override {
        for (const auto &sceneId: sceneIds) {
            if (scenes[sceneId] != nullptr) {
                scenes[sceneId]->localCleanup();
                free(scenes[sceneId]);
            }
        }

        ObjectDSL.cleanup();
        SourceDSL.cleanup();
        LightDSL.cleanup();
        MenuDSL.cleanup();

        ToonP.destroy();
        PhongP.destroy();
        SourceP.destroy();
        MenuP.destroy();

        txt.localCleanup();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) override {
        txt.populateCommandBuffer(commandBuffer, currentImage);
        MenuP.bind(commandBuffer);
        scenes[currSceneId]->populateCommandBuffer(commandBuffer, currentImage);
    }

    void updateUniformBuffer(uint32_t currentImage) override {
        float deltaT;
        auto m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire;

        getSixAxis(deltaT, m, r, fire);

        scenes[currSceneId]->SC->updateUniformBuffer(currentImage, deltaT, m, r, fire);
    }

    void changeScene(SceneId _newSceneId) override {
        changingScene = true;
        if (scenes[currSceneId] != nullptr) {
            framebufferResized = true;
        }
        scenes[_newSceneId] = getNewSceneById(_newSceneId);
        scenes[_newSceneId]->init(this, SceneVDRs.find(_newSceneId)->second, ScenePRs.find(_newSceneId)->second,
                                  sceneFiles.find(_newSceneId)->second);
        newSceneId = _newSceneId;
    }

    void changeText(std::string newText, int line) override {
        if (HIDE_TEXT)
            return;
        if (line >= 4 || line < 0)
            throw std::runtime_error("Line number out of bounds.");
        if (newText.empty()) {
            if (out[0].l[line] != nullptr) {
                free(out[0].l[line]);
                out[0].l[line] = nullptr;
            }
            if (line < out[0].usedLines - 1) {
                // move all lines
                for (int i = line; i < out[0].usedLines - 1; i++) {
                    out[0].l[i] = out[0].l[i + 1];
                    out[0].l[out[0].usedLines - 1] = nullptr;
                }
            }
            if (out[0].usedLines > 1)
                out[0].usedLines--;
        } else {
            if (line > out[0].usedLines) {
                line = out[0].usedLines;
            }
            if (out[0].l[line] != nullptr) {
                free(out[0].l[line]);
                out[0].l[line] = nullptr;
            }
            out[0].l[line] = (char *) malloc(newText.size() + 1);
            strcpy(out[0].l[line], newText.c_str());
            if (line == out[0].usedLines) {
                out[0].usedLines++;
            }
        }
        updateText = true;
        framebufferResized = true;
    }

protected:
    void recreateSwapChain() override {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        if (changingScene) {
            scenes[currSceneId]->localCleanup();
            free(scenes[currSceneId]);
            scenes[currSceneId] = nullptr;
            currSceneId = newSceneId;
            changingScene = false;
        }

        if (updateText) {
            txt.updateTexts();
            updateText = false;
        }

        createSwapChain();
        createImageViews();
        createRenderPass();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        createDescriptorPool();

        pipelinesAndDescriptorSetsInit();

        createCommandBuffers();
    }

};


int main() {
    App app;

    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
