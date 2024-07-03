#include "StartLib.hpp"


/* Uniform buffers. */
struct Uniform {
	alignas(16)	glm::mat4 mvpMat;
};


/* Vertex formats. */
struct Vertex {
	glm::vec3	pos;
	glm::vec2	UV;
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
		Model M;
		Texture T;
		DescriptorSet DS;


		// Application config.
		glm::vec3 CamPos = glm::vec3(0.0, 0.1, 5.0);
		glm::mat4 ViewMatrix;

		float Ar;

		void setWindowParameters() {
			windowWidth = 800;
			windowHeight = 600;
			windowTitle = "CG24 @ PoliMi";
			windowResizable = GLFW_FALSE;
			initialBackgroundColor = { 0.05f, 0.05f, 0.05f, 1.0f };

			Ar = (float)windowWidth / (float)windowHeight;
		}

		void onWindowResize(int w, int h) {
			Ar = (float)w / (float)h;
		}


		void localInit() {
			DSL.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(Uniform), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
			});

			VD.init(this, {
					{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
					{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
					{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV}
				}
			);

			P.init(this, &VD, "shaders/ShaderVert.spv", "shaders/ShaderFrag.spv", {&DSL});

			M.init(this, &VD, "models/Cube.obj", OBJ);

			T.init(this, "textures/Checker.png");

			DPSZs.uniformBlocksInPool = 1;
			DPSZs.texturesInPool = 1;
			DPSZs.setsInPool = 1;


			ViewMatrix = glm::translate(glm::mat4(1), -CamPos);
		}

		void pipelinesAndDescriptorSetsInit() {
			P.create();

			DS.init(this, &DSL, {&T});
		}

		void pipelinesAndDescriptorSetsCleanup() {
			P.cleanup();

			DS.cleanup();
		}

		void localCleanup() {
			M.cleanup();
			T.cleanup();

			DSL.cleanup();

			P.destroy();
		}

		void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
			P.bind(commandBuffer);
			M.bind(commandBuffer);
			DS.bind(commandBuffer, P, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M.indices.size()), 1, 0, 0, 0);
		}

		void updateUniformBuffer(uint32_t currentImage) {
			if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, GL_TRUE);
			}


			// Test.
			const float FOVy = glm::radians(90.0f);
			const float nearPlane = 0.1f;
			const float farPlane = 100.0f;

			glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
			Prj[1][1] *= -1;
			glm::vec3 camTarget = glm::vec3(0, 0, 0);
			glm::vec3 camPos = camTarget + glm::vec3(6, 3, 10) / 2.0f;
			glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

			glm::mat4 World = glm::translate(glm::mat4(1), glm::vec3(-3, 0, 0));
			Uniform ubo;
			ubo.mvpMat = Prj * View * World;
			DS.map(currentImage, &ubo, 0);
		}
	};


int main() {
	CG app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
