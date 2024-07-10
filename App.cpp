#include "modules/Starter.hpp"
#include "modules/AppCommon.hpp"
#include "modules/Scene.hpp"

/* Uniform buffers. */
// TODO: Add a uniform buffers
//struct UniformBufferObject {
//    alignas(16) glm::mat4 mvpMat;
//    alignas(16) glm::mat4 mMat;
//    alignas(16) glm::mat4 nMat;
//};
//
//struct GlobalUniformBufferObject {
//    alignas(16) glm::vec3 lightDir;
//    alignas(16) glm::vec4 lightColor;
//    alignas(16) glm::vec3 eyePos;
//};


/* Vertex formats. */
// TODO: Add a vertex formats
//struct VertexNormMap {
//    glm::vec3 pos;
//    glm::vec3 normal;
//    glm::vec4 tangent;
//    glm::vec2 texCoord;
//};


class App : public BaseProject {
protected:

    /* Descriptor set layouts. */
    // TODO: Add a descriptor set layouts
//    DescriptorSetLayout DSL;


    /* Vertex descriptors. */
    // TODO: Add a vertex descriptors
//    VertexDescriptor VD;


    /* Pipelines. */
    // TODO: Add a pipelines
//    Pipeline P;


    /* Scenes */
    std::unordered_map<SceneId, Scene *> scenes;
    std::unordered_map<SceneId, std::vector<VertexDescriptorRef>> SceneVDRs;
    std::unordered_map<SceneId, std::vector<PipelineRef>> ScenePRs;


    // Application config.
    SceneId currSceneId;

    void setWindowParameters() override {
        windowWidth = 800;
        windowHeight = 600;
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

        // TODO: Init all Descriptor set layouts
//        DSL.init(this, {
//                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(Uniform), 1},
//                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,               1}
//        });

        // TODO: Init all Vertex descriptors
//        VD.init(this, {
//                        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
//                }, {
//                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
//                        {0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, UV),  sizeof(glm::vec2), UV}
//                }
//        );

        // TODO: Init all Pipelines
//        P.init(this, &VD, "shaders/Axis.vert.spv", "shaders/Axis.frag.spv", {&DSL});

        // TODO: Define all Vertex Descriptor Ref per Scene
        std::vector<VertexDescriptorRef> SL1VDRs = {
//                {"VD", &VD},
        };
        // ...
        // TODO: Define Pipeline Ref per Scene
        std::vector<PipelineRef> SL1PRs = {
//                {"P", &P}
        };
        // ...

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
        scenes[currSceneId]->pipelinesAndDescriptorSetsInit();
    }

    void pipelinesAndDescriptorSetsCleanup() override {
        scenes[currSceneId]->pipelinesAndDescriptorSetsCleanup();
    }

    void localCleanup() override {
        for (const auto &sceneId: sceneIds) {
            if (scenes[sceneId] != nullptr) {
                scenes[sceneId]->localCleanup();
                free(scenes[sceneId]);
            }
        }
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) override {
        scenes[currSceneId]->populateCommandBuffer(commandBuffer, currentImage);
    }

    void updateUniformBuffer(uint32_t currentImage) override {
        /*if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }


        const float FOVy = glm::radians(90.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.0f;

        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;
        glm::vec3 camTarget = glm::vec3(0, 0, 0);
        glm::vec3 camPos = camTarget + glm::vec3(3, 2, 5) / 4.0f; // Green y, blue x, red z (inverted).
        glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

        glm::mat4 World = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));
        Uniform ubo{};
        ubo.mvpMat = Prj * View * World;
        DS.map(currentImage, &ubo, 0);*/

        float deltaT;
        auto m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire;

        getSixAxis(deltaT, m, r, fire);

        scenes[currSceneId]->SC->updateUniformBuffer(deltaT, m, r, fire);
    }

    void changeScene(SceneId newSceneId) {
        if (scenes[currSceneId] != nullptr) {
            scenes[currSceneId]->pipelinesAndDescriptorSetsCleanup();
        }

        scenes[newSceneId] = getNewSceneById(newSceneId);
        scenes[newSceneId]->init(this, SceneVDRs.find(newSceneId)->second, ScenePRs.find(newSceneId)->second,
                                 sceneFiles.find(newSceneId)->second);
        currSceneId = newSceneId;
        // FIXME: maybe this is called twice
        //scenes[currSceneId]->pipelinesAndDescriptorSetsInit();
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
