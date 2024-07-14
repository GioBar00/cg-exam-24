#include "modules/Starter.hpp"
#include "modules/AppCommon.hpp"
#include "modules/Scene.hpp"


class App : public BaseProject {
protected:

    /* Descriptor set layouts. */
    DescriptorSetLayout ObjectDSL, SourceDSL, LightDSL;


    /* Vertex descriptors. */
    VertexDescriptor ObjectVD, SourceVD;


    /* Pipelines. */
    Pipeline ToonP, PhongP, SourceP;


    /* Scenes */
    std::unordered_map<SceneId, Scene *> scenes;
    std::unordered_map<SceneId, std::vector<VertexDescriptorRef>> SceneVDRs;
    std::unordered_map<SceneId, std::vector<PipelineRef>> ScenePRs;


    // Application config.
    SceneId currSceneId;

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

        ToonP.init(this, &ObjectVD, "shaders/Shader.vert.spv", "shaders/Toon.frag.spv", {&LightDSL, &ObjectDSL});
        PhongP.init(this, &ObjectVD, "shaders/Shader.vert.spv", "shaders/Phong.frag.spv", {&LightDSL, &ObjectDSL});
        SourceP.init(this, &SourceVD, "shaders/Emission.vert.spv", "shaders/Emission.frag.spv", {&SourceDSL});


        // Define vertex descriptor references per scene.
        VertexDescriptorRef ObjectVDR{}, SourceVDR{};
        ObjectVDR.init("object", &ObjectVD);
        SourceVDR.init("source", &SourceVD);
        std::vector<VertexDescriptorRef> SL1VDRs = { ObjectVDR, SourceVDR };

        // Define pipeline references per scene.
        PipelineRef ToonPR{}, PhongPR{}, SourcePR{};
        ToonPR.init("toon", &ToonP);
        PhongPR.init("phong", &PhongP);
        SourcePR.init("emission", &SourceP);
        std::vector<PipelineRef> SL1PRs = { ToonPR, PhongPR, SourcePR };

        // TODO: Add all SceneVDRs and ScenePRs to the maps
        SceneVDRs[SceneId::SCENE_LEVEL_1] = SL1VDRs;
        ScenePRs[SceneId::SCENE_LEVEL_1] = SL1PRs;
//        SceneVDRs[SceneId::SCENE_LEVEL_2] = SL2VDRs;
//        ScenePRs[SceneId::SCENE_LEVEL_2] = SL2PRs;


        // Set the first scene
        currSceneId = SceneId::SCENE_LEVEL_1;
        changeScene(currSceneId);
    }

    void pipelinesAndDescriptorSetsInit() override {
        ToonP.create();
        PhongP.create();
        SourceP.create();
        scenes[currSceneId]->pipelinesAndDescriptorSetsInit();
    }

    void pipelinesAndDescriptorSetsCleanup() override {
        ToonP.cleanup();
        PhongP.cleanup();
        SourceP.cleanup();
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

        ToonP.destroy();
        PhongP.destroy();
        SourceP.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) override {
        scenes[currSceneId]->populateCommandBuffer(commandBuffer, currentImage);
    }

    void updateUniformBuffer(uint32_t currentImage) override {
        float deltaT;
        auto m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire;

        getSixAxis(deltaT, m, r, fire);

        scenes[currSceneId]->SC->updateUniformBuffer(currentImage, deltaT, m, r, fire);
    }

    void changeScene(SceneId newSceneId) {
        if (scenes[currSceneId] != nullptr) {
            scenes[currSceneId]->pipelinesAndDescriptorSetsCleanup();
        }

        if (scenes[newSceneId] == nullptr) {
            scenes[newSceneId] = getNewSceneById(newSceneId);
            scenes[newSceneId]->init(this, SceneVDRs.find(newSceneId)->second, ScenePRs.find(newSceneId)->second,
                                     sceneFiles.find(newSceneId)->second);
        }
        currSceneId = newSceneId;
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
