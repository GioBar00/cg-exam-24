#include "modules/Starter.hpp"
#include "modules/Scene.hpp"

#define OBJS 48
#define MY_OBJS 30
#define UNIT 3.1f
#define OFFSET 0.1f


/* Uniform buffers. */
struct ObjectUniform {
    alignas(16) glm::mat4   mvpMat;
    alignas(16) glm::mat4   mMat;
    alignas(16) glm::mat4   nMat;
};

struct LightUniform {
    alignas(16) glm::vec3   lightPos;
    alignas(16) glm::vec3   lightDir;
    alignas(16) glm::vec4   lightCol;
    alignas(4)  float       cosIn;
    alignas(4)  float       cosOut;
    alignas(4)  int         TYPE; // 0 := DIRECT, 1 := POINT, 2 = SPOT.
    alignas(16) glm::vec3   eyePos;
};


/* Vertex formats. */
struct ToonVertex {
    glm::vec3   pos;
    glm::vec3   norm;
    glm::vec2   UV;
};


class CG : public BaseProject {
protected:

    /* Descriptor set layouts. */
    DescriptorSetLayout ToonDSL, LightDSL;


    /* Vertex descriptors. */
    VertexDescriptor ToonVD;


    /* Pipelines. */
    Pipeline IlluminationP;


    /* Models, textures, descriptor sets. */
    Model Ms[OBJS];
    Texture T;
    DescriptorSet ToonDs[OBJS], LightDS;


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

        IlluminationP.init(this, &ToonVD, "shaders/brdf/ToonVert.spv", "shaders/brdf/ToonFrag.spv", {&LightDSL, &ToonDSL});
        //IlluminationP.init(this, &ToonVD, "shaders/brdf/PhongVert.spv", "shaders/brdf/PhongFrag.spv", {&LightDSL, &ToonDSL});
        //IlluminationP.init(this, &ToonVD, "shaders/brdf/BlinnVert.spv", "shaders/brdf/BlinnFrag.spv", {&LightDSL, &ToonDSL});

        Ms[0].init(this, &ToonVD, "models/axis.obj", OBJ);
        Ms[1].init(this, &ToonVD, "models/dungeon/cast_Mesh.6268.mgcg", MGCG);
        static const std::string GROUND = "models/dungeon/tunnel.003_Mesh.4991.mgcg", WALL_LINE = "models/dungeon/tunnel_Mesh.4672.mgcg", WALL_ANGLE = "models/dungeon/tunnel.028_Mesh.7989.mgcg";
        for (int i = 2; i < 2 + 9; i++) Ms[i].init(this, &ToonVD, GROUND, MGCG);
        for (int i = 11; i < 11 + 12; i++) Ms[i].init(this, &ToonVD, WALL_LINE, MGCG);
        for (int i = 23; i < 23 + 4; i++) Ms[i].init(this, &ToonVD, WALL_ANGLE, MGCG);
        Ms[27].init(this, &ToonVD, "models/dungeon/light.002_Mesh.6811.mgcg", MGCG);
        Ms[28].init(this, &ToonVD, "models/dungeon/light.009_Mesh.6851.mgcg", MGCG);
        Ms[29].init(this, &ToonVD, "models/dungeon/light.013_Mesh.7136.mgcg", MGCG);
        for (int i = MY_OBJS; i < OBJS; i++) Ms[i].init(this, &ToonVD, "models/axis.obj", OBJ);

        T.init(this, "textures/dungeon/Textures_Dungeon.png");

        DPSZs.uniformBlocksInPool = OBJS + 1;
        DPSZs.texturesInPool = OBJS;
        DPSZs.setsInPool = OBJS + 1;


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
        IlluminationP.create();

        for (int i = 0; i < OBJS; i++)
            ToonDs[i].init(this, &ToonDSL, {&T});
        LightDS.init(this, &LightDSL, {});
    }

    void pipelinesAndDescriptorSetsCleanup() override {
        IlluminationP.cleanup();

        for (int i = 0; i < OBJS; i++)
            ToonDs[i].cleanup();
        LightDS.cleanup();
    }

    void localCleanup() override {
        for (int i = 0; i < OBJS; i++)
            Ms[i].cleanup();
        T.cleanup();

        ToonDSL.cleanup();
        LightDSL.cleanup();

        IlluminationP.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) override {
        for (int i = 0; i < MY_OBJS; i++) {
            IlluminationP.bind(commandBuffer);
            Ms[i].bind(commandBuffer);
            LightDS.bind(commandBuffer, IlluminationP, 0, currentImage);
            ToonDs[i].bind(commandBuffer, IlluminationP, 1, currentImage);
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
        for (int i = -1; i <= +1; i++) // z.
            for (int j = -1; j <= +1; j++) // x.
                Worlds[2 + (3 * (i + 1) + (j + 1))] = glm::scale(glm::translate(BaseT, glm::vec3(+j * UNIT / 1.5f, 0, +i * UNIT / 1.5f)), glm::vec3(0.5f)); // Ground. (3x3 after scaling.)
        for (int i = 0; i <= 3; i++) {
            float dirX = glm::cos(glm::radians(+(i + 1) * 90.0f)), dirZ = -glm::sin(glm::radians(+(i + 1) * 90.0f));
            glm::vec3 pivot = glm::vec3(dirX * (UNIT + UNIT / 1.5f), 0, dirZ * (UNIT + UNIT / 1.5f)); // FIX: Different units between ground-wall.
            for (int j = -1; j <= +1; j++) {
                Worlds[11 + (3 * i + (j + 1))] = glm::scale(glm::rotate(glm::translate(glm::mat4(1), pivot + glm::vec3(j * dirZ * UNIT / 1.5f, 0, j * dirX * UNIT / 1.5f)), glm::radians(+i * 90.0f), glm::vec3(0, 1, 0)), glm::vec3(0.5f, 1, 1));
            }
        }
        for (int i = 0; i <= 1; i++)
            for (int j = 0; j <= 1; j++)
                Worlds[23 + (2 * i + j)] = glm::rotate(glm::translate(BaseT, glm::vec3(+(2 * j - 1) * (UNIT + UNIT / 1.5f), 0, +(2 * i - 1) * (UNIT + UNIT / 1.5f))), glm::radians(-(i == 1 ? 2 + int(!bool(j)) : j) * 90.0f), glm::vec3(0, 1, 0));
        Worlds[27] = glm::rotate(glm::translate(BaseT, glm::vec3(-(UNIT - 5 * OFFSET + UNIT / 1.5f), UNIT / 2.0f, 0)), glm::radians(+90.0f), glm::vec3(0, 1, 0));
        Worlds[28] = glm::rotate(glm::translate(BaseT, glm::vec3(+(UNIT - 7.75 * OFFSET + UNIT / 1.5f), UNIT / 2.0f, 0)), glm::radians(-90.0f), glm::vec3(0, 1, 0));
        Worlds[29] = glm::translate(BaseT, glm::vec3(0, UNIT / 2.0f, -(UNIT - 9 * OFFSET + UNIT / 1.5f)));
        ObjectUniform ubos[MY_OBJS];
        for (int i = 0; i < MY_OBJS; i++) {
            ubos[i].mvpMat = Prj * View * Worlds[i];
            ubos[i].mMat = Worlds[i];
            ubos[i].nMat = glm::inverse(glm::transpose(Worlds[i]));
            ToonDs[i].map(currentImage, &ubos[i], 0);
        }
        LightUniform lubo;
        lubo.lightPos = glm::vec3(0, +4, 0);
        lubo.lightDir = glm::vec3(0, -1, 0);
        lubo.lightCol = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        lubo.cosIn = glm::cos(glm::radians(60.0f));
        lubo.cosOut = glm::cos(glm::radians(150.0f));
        lubo.TYPE = 1;
        lubo.eyePos = glm::vec3(glm::inverse(View) * glm::vec4(0, 0, 0, 1));
        LightDS.map(currentImage, &lubo, 0);
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
