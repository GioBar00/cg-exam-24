#include "modules/Starter.hpp"
#include "modules/Scene.hpp"

#define OBJS 16
#define MY_OBJS 5
#define UNIT 3.1f


/* Uniform buffers. */
struct Uniform {
    alignas(16) glm::mat4 mvpMat;
};


/* Vertex formats. */
struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
};


class CG : public BaseProject {
protected:

    /* Descriptor set layouts. */
    DescriptorSetLayout DSL;


    /* Vertex descriptors. */
    VertexDescriptor VD;


    /* Pipelines. */
    Pipeline P;


    /* Models, textures, descriptor sets. */
    Model Ms[OBJS];
    Texture T;
    DescriptorSet Ds[OBJS];


    // Application configs.
    float Ar;

    void setWindowParameters() override {
        windowWidth = 1200;
        windowHeight = 900;
        windowTitle = "CG24 @ PoliMi";
        windowResizable = GLFW_FALSE;
        initialBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};

        Ar = (float) windowWidth / (float) windowHeight;
    }

    void onWindowResize(int w, int h) override {
        Ar = (float) w / (float) h;
    }

    const float FOVy = glm::radians(90.0f);
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    glm::mat4 Prj;
    glm::vec3 camTarget, camPos;
    glm::mat4 View;


    void localInit() override {
        DSL.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(Uniform), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,               1}
        });

        VD.init(this, {
                        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
                }, {
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
                        {0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, UV),  sizeof(glm::vec2), UV}
                }
        );

        P.init(this, &VD, "shaders/AxisVert.spv", "shaders/AxisFrag.spv", {&DSL});

        Ms[0].init(this, &VD, "models/axis.obj", OBJ);
        Ms[1].init(this, &VD, "models/dungeon/cast_Mesh.6268.mgcg", MGCG);
        Ms[2].init(this, &VD, "models/dungeon/tunnel.003_Mesh.4991.mgcg", MGCG);
        Ms[3].init(this, &VD, "models/dungeon/tunnel_Mesh.4672.mgcg", MGCG);
        Ms[4].init(this, &VD, "models/dungeon/tunnel.028_Mesh.7989.mgcg", MGCG);
        for (int i = MY_OBJS; i < OBJS; i++) Ms[i].init(this, &VD, "models/axis.obj", OBJ);

        T.init(this, "textures/dungeon/Textures_Dungeon.png");

        DPSZs.uniformBlocksInPool = OBJS;
        DPSZs.texturesInPool = OBJS;
        DPSZs.setsInPool = OBJS;


        Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;
        /*
        camTarget = glm::vec3(0, 0, 0);
        camPos = camTarget + glm::vec3(0, 3, 2); // Green y, blue x, red z (inverted).
        View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
        */
        camPos = glm::vec3(0, 3, 2); // Green y, blue x, red z (inverted).
        View = glm::translate(glm::mat4(1), -camPos);
    }

    void pipelinesAndDescriptorSetsInit() override {
        P.create();

        for (int i = 0; i < OBJS; i++)
            Ds[i].init(this, &DSL, {&T});
    }

    void pipelinesAndDescriptorSetsCleanup() override {
        P.cleanup();

        for (int i = 0; i < OBJS; i++)
            Ds[i].cleanup();
    }

    void localCleanup() override {
        for (int i = 0; i < OBJS; i++)
            Ms[i].cleanup();
        T.cleanup();

        DSL.cleanup();

        P.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) override {
        for (int i = 0; i < OBJS; i++) {
            P.bind(commandBuffer);
            Ms[i].bind(commandBuffer);
            Ds[i].bind(commandBuffer, P, 0, currentImage);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Ms[i].indices.size()), 1, 0, 0, 0);
        }
    }

    void updateUniformBuffer(uint32_t currentImage) override {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);

        const float ROT_SPEED = glm::radians(120.0f);
        const float MOV_SPEED = 2.0f;

        View = glm::rotate(glm::mat4(1), ROT_SPEED * r.x * deltaT, glm::vec3(1, 0, 0)) * View;
        View = glm::rotate(glm::mat4(1), ROT_SPEED * r.y * deltaT, glm::vec3(0, 1, 0)) * View;
        View = glm::rotate(glm::mat4(1), -ROT_SPEED * r.z * deltaT, glm::vec3(0, 0, 1)) * View;
        View = glm::translate(glm::mat4(1), -glm::vec3(MOV_SPEED * m.x * deltaT, MOV_SPEED * m.y * deltaT, MOV_SPEED * m.z * deltaT)) * View;


        glm::mat4 Worlds[MY_OBJS], BaseT = glm::mat4(1);
        Worlds[0] = glm::translate(BaseT, glm::vec3(0, 0, 0)); // Axis.
        Worlds[1] = glm::translate(BaseT, glm::vec3(0, 1, 0)); // Character.
        Worlds[2] = glm::scale(glm::translate(BaseT, glm::vec3(0, 0, 0)), glm::vec3(0.5f)); // Ground. (3x3 after scaling.)
        Worlds[3] = glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(-UNIT, 0, 0)), glm::radians(90.0f), glm::vec3(0, 1, 0)), glm::vec3(0.5f, 1, 1));
        Worlds[4] = glm::translate(BaseT, glm::vec3(-UNIT, 0, -UNIT));
        Uniform ubos[MY_OBJS];
        for (int i = 0; i < MY_OBJS; i++) {
            ubos[i].mvpMat = Prj * View * Worlds[i];
            Ds[i].map(currentImage, &ubos[i], 0);
        }
    }
};


int main() {
    CG app;

    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
