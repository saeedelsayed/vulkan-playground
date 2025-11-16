#include "first_app.hpp"
#include "lve_texture.hpp"
#include "keyboard_movement_controller.hpp"
#include "simple_render_system.hpp"
#include "lve_camera.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE


#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <chrono>
#include <cassert>
#include <stdexcept>

#include<iostream>

namespace lve {

    struct GlobalUbo {
        glm::mat4 projectionView{ 1.0f };
        glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, 1.f});
    };

    FirstApp::FirstApp() {
        globalPool = LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
       
        loadGameObjects();

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(lveDevice.phDevice(), &properties);
        std::cout << "Max push constants size: " << properties.limits.maxPushConstantsSize << "bytes" << std::endl;
    }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        std::shared_ptr<LveTexture> texture1 = std::make_unique<LveTexture>(lveDevice, "../vulkan-playground/textures/nature.jpg");
        std::shared_ptr<LveTexture> texture2 = std::make_unique<LveTexture>(lveDevice, "../vulkan-playground/textures/background.jpg");
        std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<LveBuffer>(
                lveDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSet(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSet.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();

            VkDescriptorImageInfo imageInfo[2];
            imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[0].imageView = texture1->textureImageView;
            imageInfo[0].sampler = texture1->textureSampler;

            imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[1].imageView = texture2->textureImageView;
            imageInfo[1].sampler = texture2->textureSampler;

            LveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, imageInfo)
                .build(globalDescriptorSet[i]);
        }

        SimpleRenderSystem simpleRenderSystem{ lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
        LveCamera camera{};
        //camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
        //camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = LveGameObject::createGameObject();
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();
        
        while (!lveWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            
            float aspect = lveRenderer.getAspectRatio();
            //camera.setOrthographicProjection(-aspect, aspect, -1., 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

            if (auto commandBuffer = lveRenderer.beginFrame()) {
                int frameIndex = lveRenderer.getFrameIndex();

                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSet[frameIndex]
                };


                GlobalUbo ubo{};
                ubo.projectionView = camera.getProjection() * camera.getView();
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                lveRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
                lveRenderer.endSwapChainRenderPass(commandBuffer);
   /*             vkDestroyImage(lveDevice.device(), textureImage, nullptr);
                vkFreeMemory(lveDevice.device(), textureImageMemory, nullptr);*/
                lveRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(lveDevice.device());
    }


    void FirstApp::loadGameObjects() {
        LveModel::Builder cube1{};
        cube1.vertices = {
            // Front face
            {{-1.0f, -1.0f,  0.2f}, {0.2f, 0.8f, 1.f}, {0.f, 0.f, 1.f}, {1.f, 0.f}},
            {{1.0f, -1.0f,  0.2f}, {0.6f, 0.3f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f}},
            {{1.0f,  1.0f,  0.2f}, {0.2f, 0.6f, 0.8f}, {0.f, 0.f, 1.f}, {0.f, 1.f}},
            {{-1.0f, 1.0f,  0.2f}, {0.9f, 0.4f, 0.6f}, {0.f, 0.f, 1.f}, {1.f, 1.f}},

            //// Back face
            //{{1.0f, -0.5f, 0.6f}, {0.2f, 0.9f, 0.4f}, {0.f, 0.f, -1.f}, {1.f, 0.f}},
            //{{2.0f, -0.5f, 0.6f}, {0.8f, 0.2f, 0.3f}, {0.f, 0.f, -1.f}, {0.f, 0.f}},
            //{{2.0f,  0.5f, 0.6f}, {0.3f, 0.7f, 0.9f}, {0.f, 0.f, -1.f}, {0.f, 1.f}},
            //{{1.0f,  0.5f, 0.6f}, {0.7f, 0.5f, 0.2f}, {0.f, 0.f, -1.f}, {1.f, 1.f}},
        };

        cube1.indices = {
        0, 1, 2, 2, 3, 0
        };
        std::shared_ptr<LveModel> lvemodel = std::make_unique<LveModel>(lveDevice, cube1);
        auto tri = LveGameObject::createGameObject();
        tri.model = lvemodel;
        //tri.transform.scale = { .2f, .2f, .2f };
        gameObjects.push_back(std::move(tri));

        //LveModel::Builder cube2{};
        //cube2.vertices = {
        //    // Front face
        //    {{1.0f, -0.5f,  0.5f}, {0.2f, 0.8f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f}},
        //    {{2.0f, -0.5f,  0.5f}, {0.6f, 0.3f, 1.f}, {0.f, 0.f, 1.f}, {1.f, 0.f}},
        //    {{2.0f,  0.5f,  0.5f}, {0.2f, 0.6f, 0.8f}, {0.f, 0.f, 1.f}, {1.f, 1.f}},
        //    {{1.0f,  0.5f,  0.5f}, {0.9f, 0.4f, 0.6f}, {0.f, 0.f, 1.f}, {0.f, 1.f}},

        //    // Back face
        //    {{1.0f, -0.5f, 1.0f}, {0.2f, 0.9f, 0.4f}, {0.f, 0.f, -1.f}, {1.f, 0.f}},
        //    {{2.0f, -0.5f, 1.0f}, {0.8f, 0.2f, 0.3f}, {0.f, 0.f, -1.f}, {0.f, 0.f}},
        //    {{2.0f,  0.5f, 1.0f}, {0.3f, 0.7f, 0.9f}, {0.f, 0.f, -1.f}, {0.f, 1.f}},
        //    {{1.0f,  0.5f, 1.0f}, {0.7f, 0.5f, 0.2f}, {0.f, 0.f, -1.f}, {1.f, 1.f}},
        //};
        //cube2.indices = {
        //    // Front face
        //    0, 1, 2, 2, 3, 0,
        //    // Back face
        //    4, 5, 6, 6, 7, 4,
        //    // Left face
        //    4, 0, 3, 3, 7, 4,
        //    // Right face
        //    1, 5, 6, 6, 2, 1,
        //    // Top face
        //    3, 2, 6, 6, 7, 3,
        //    // Bottom face
        //    4, 5, 1, 1, 0, 4
        //};

        //std::shared_ptr<LveModel> lvemodel2 = std::make_unique<LveModel>(lveDevice, cube2);
        //auto tri2 = LveGameObject::createGameObject();
        //tri2.model = lvemodel2;
        //tri2.transform.scale = { .2f, .2f, .2f };
        //gameObjects.push_back(std::move(tri2));
    }  // namespace lve


}


