#include "modules/Starter.hpp"
#include "modules/AppCommon.hpp"
#include "modules/Scene.hpp"


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


class App : public BaseProject {
protected:

    /* Descriptor set layouts. */
    DescriptorSetLayout ToonDSL, LightDSL;


    /* Vertex descriptors. */
    VertexDescriptor ToonVD;


    /* Pipelines. */
    Pipeline IlluminationP;


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


        ToonDSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_ALL_GRAPHICS,   sizeof(ObjectUniform),  1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  VK_SHADER_STAGE_FRAGMENT_BIT,   0,                      1}
        });
        LightDSL.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          VK_SHADER_STAGE_FRAGMENT_BIT,   sizeof(LightUniform),   1}
		});

        ToonVD.init(this, {
                {0, sizeof(ToonVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ToonVertex, pos),  sizeof(glm::vec3),  POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ToonVertex, norm), sizeof(glm::vec3),  NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,     offsetof(ToonVertex, UV),   sizeof(glm::vec2),  UV}
            }
        );

        IlluminationP.init(this, &ToonVD, "shaders/brdf/Toon.vert.spv", "shaders/brdf/Toon.frag.spv", {&LightDSL, &ToonDSL});


        // Define vertex descriptor references per scene.
        VertexDescriptorRef ToonVDR;
        ToonVDR.init("ToonVD", &ToonVD);
        std::vector<VertexDescriptorRef> SL1VDRs = { ToonVDR };

        // Define pipeline references per scene.
        PipelineRef IlluminationPR;
        IlluminationPR.init("illumination", &IlluminationP);
        std::vector<PipelineRef> SL1PRs = { IlluminationPR };

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
        IlluminationP.create();
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
