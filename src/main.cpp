#include <vulkan/vulkan_raii.hpp>

#define VK_USE_PLATFORM_WIN32_KHR

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string_view>
#include <expected>
#include <map>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>


#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tiny_obj_loader.h"

constexpr std::uint32_t WIDTH = 800, HEIGHT = 600;
constexpr std::uint32_t APP_VERSION = VK_MAKE_VERSION(0, 1, 0);
constexpr std::uint32_t ENGINE_VERSION = VK_MAKE_VERSION(0, 1, 0);
constexpr const char* APP_NAME = "Untitled game W.I.P";
constexpr const char* ENGINE_NAME = "Untitled engine W.I.P";
constexpr int MAX_FRAMES_IN_FLIGHT = 2;


const std::vector<char const*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
constexpr bool enable_validation_layers = false;
#else
constexpr bool enable_validation_layers = true;
#endif

auto required_instance_extensions() -> std::vector<const char*>;

struct vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texture_coord;

    static vk::VertexInputBindingDescription binding_description() {
        return { 0, sizeof(vertex), vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions() {
        return {{
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(vertex, position)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(vertex, color)},
            {2, 0, vk::Format::eR32G32Sfloat, offsetof(vertex, texture_coord)}
            }};
    }
};

struct uniform_buffer_object {
    glm::mat4x4 model;
    glm::mat4x4 view;
    glm::mat4x4 projection;
};

class vulkan_instance {
public:
    void run() {
        this->initialize_glfw3();
        this->initialize_vulkan();
        this->loop();
        this->cleanup();
    }
private:
    GLFWwindow* window = nullptr;

   
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphics_queue = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::SwapchainKHR swap_chain = nullptr;
    vk::raii::PipelineLayout pipeline_layout = nullptr;
    vk::raii::Pipeline graphics_pipeline = nullptr;
    vk::raii::CommandPool command_pool = nullptr;
    vk::raii::Buffer vertex_buffer = nullptr;
    vk::raii::DeviceMemory vertex_memory = nullptr;
    vk::raii::Buffer index_buffer = nullptr;
    vk::raii::DeviceMemory index_memory = nullptr;
    vk::raii::DescriptorSetLayout descriptor_layout = nullptr;
    vk::raii::DescriptorPool descriptor_pool = nullptr;
    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory image_memory = nullptr;
    vk::raii::ImageView image_view = nullptr;
    vk::raii::Sampler sampler = nullptr;
    vk::raii::Image depth_image = nullptr;
    vk::raii::DeviceMemory depth_image_memory = nullptr;
    vk::raii::ImageView depth_image_view = nullptr;
    vk::raii::Image color_image = nullptr;
    vk::raii::DeviceMemory color_memory = nullptr;
    vk::raii::ImageView color_view = nullptr;

    

    vk::Extent2D swap_chain_extent;
    vk::SurfaceFormatKHR swap_chain_surface_format;
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;

    std::uint32_t queue_index = ~0;
    std::uint32_t frame_index = 0;
    std::uint32_t mip_levels = 0;
   
    std::vector<vk::Image> swap_chain_images;
    std::vector<vk::raii::ImageView> swap_chain_image_view;
    std::vector<vk::raii::CommandBuffer> command_buffers;
    std::vector<vk::raii::Semaphore> present_semaphores;
    std::vector<vk::raii::Semaphore> render_finished_semaphores;
    std::vector<vk::raii::Fence> in_flight_fences;
    std::vector<vk::raii::Buffer> uniform_buffers;
    std::vector<vk::raii::DeviceMemory> uniform_buffers_memory;
    std::vector<vk::raii::DescriptorSet> descriptor_sets;
    std::vector<void*> uniform_buffers_mapped;
    std::vector<vertex> vertices;
    std::vector<std::uint32_t> indices;



    bool frame_buffer_resized = false;

    std::vector<const char*> required_device_extension = { vk::KHRSwapchainExtensionName };

    void initialize_glfw3() {
        if (!glfwInit()) {
            throw std::runtime_error("glfw3 initialization error");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebuffer_resize__);
    }

    static void framebuffer_resize__(GLFWwindow* window, int width, int height) {
        vulkan_instance* app = reinterpret_cast<vulkan_instance*>(glfwGetWindowUserPointer(window));
        app->frame_buffer_resized = true;
    }

    void initialize_vulkan() {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();

        create_swap_chain();
        create_image_views();

        create_descriptor_set_layout();

        create_graphics_pipeline();

        create_command_pool();

        create_color_resources();
        create_depth_resources();

        create_texture_image();
        create_texture_image_view();
        create_texture_sampler();

        load_model();

        create_vertex_buffer();
        create_index_buffer();

        create_uniform_buffers();

        create_descriptor_pool();
        create_descriptor_sets();

        create_command_buffer();
        create_sync_objects();
    }

    void create_instance() {
        constexpr vk::ApplicationInfo app_info{APP_NAME, APP_VERSION, ENGINE_NAME, ENGINE_VERSION, vk::ApiVersion14};

        auto extensios = required_instance_extensions();

       
        auto properties = context.enumerateInstanceExtensionProperties();
        auto unsupported_property_iterator = std::ranges::find_if(extensios, [&properties](auto const& required) { return std::ranges::none_of(properties, [required](auto const& extensionProperty) { return strcmp(extensionProperty.extensionName, required) == 0; });});
        
        if (unsupported_property_iterator != extensios.end())
        {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupported_property_iterator));
        }


        std::vector<char const*> required_layers;
        if (enable_validation_layers) {
            required_layers.assign(validation_layers.begin(), validation_layers.end());
        }

        std::vector<vk::LayerProperties> layer_properties = this->context.enumerateInstanceLayerProperties();
        auto unsupported_layer_iterator = std::ranges::find_if(required_layers, [&layer_properties](auto const& required) {return std::ranges::none_of(layer_properties, [required](auto const& property) { return std::strcmp(property.layerName, required) == 0; }); });

        if (unsupported_layer_iterator != required_layers.end()) {
            throw std::runtime_error("Required layer not supported: " + std::string(*unsupported_layer_iterator));
        }

        vk::InstanceCreateInfo create_info;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensios.size());
        create_info.ppEnabledExtensionNames = extensios.data();
        create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
        create_info.ppEnabledLayerNames = required_layers.data();


        this->instance = vk::raii::Instance(this->context, create_info);

    }

    void setup_debug_messenger() {
        if (!enable_validation_layers) return;

        vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT ext_info;
        ext_info.messageSeverity = severity_flags;
        ext_info.messageType = type_flags;
        ext_info.pfnUserCallback = &debug_callback;
        this->debug_messenger = instance.createDebugUtilsMessengerEXT(ext_info);
    }

    void loop() {
        while (!glfwWindowShouldClose(this->window)) {
            glfwPollEvents();
            draw_frame();
        }

        device.waitIdle();
    }

    void cleanup() {
        cleanup_swap_chain();
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }

    void pick_physical_device() {
        std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
        auto const iterator = std::ranges::find_if(devices, [&](auto const& device) { return this->is_device_suitable(device); });
        if (iterator == devices.end())
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        std::cout << "Using " << iterator->getProperties().deviceName << " GPU" << std::endl;
        this->physical_device = *iterator;
        msaa_samples = get_max_usable_samples();
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
    {
        if (severity & (vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning))
        {
            std::cerr
                << "validation layer: type "
                << to_string(type)
                << " msg: "
                << pCallbackData->pMessage
                << std::endl;
        }

        return vk::False;
    }

    bool is_device_suitable(vk::raii::PhysicalDevice const& device) {
        bool supports_vk_1_3 = device.getProperties().apiVersion >= vk::ApiVersion13;
        std::vector<vk::QueueFamilyProperties> queue_families = device.getQueueFamilyProperties();
        bool supports_graphics = std::ranges::any_of(queue_families, [](auto const& qfp) {return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

        std::vector<vk::ExtensionProperties> available_extensions = device.enumerateDeviceExtensionProperties();
        bool supports_all_extensions =
            std::ranges::all_of(required_device_extension,
                [&available_extensions](auto const& required)
                {
                    return std::ranges::any_of(available_extensions,
                        [required](auto const& available)
                        { return strcmp(available.extensionName, required) == 0; });
                });

        auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supports_required_features =
            features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
        return supports_vk_1_3 && supports_graphics && supports_all_extensions && supports_required_features;
    }

    void create_logical_device() {
        std::vector<vk::QueueFamilyProperties> properties = this->physical_device.getQueueFamilyProperties();

        for (std::uint32_t index = 0; index < properties.size(); index++) {
            if ((properties[index].queueFlags & vk::QueueFlagBits::eGraphics) && this->physical_device.getSurfaceSupportKHR(index, *this->surface)) {
                queue_index = index;
                break;
            }


        }

        if (queue_index == ~0) {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }

        float priority = 0.5f;
        vk::DeviceQueueCreateInfo queue_info;
        queue_info.queueFamilyIndex = queue_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;

        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        > chain;

        chain.get<vk::PhysicalDeviceFeatures2>()
            .features.samplerAnisotropy = vk::True;

        chain.get<vk::PhysicalDeviceVulkan11Features>()
            .shaderDrawParameters = vk::True;

        chain.get<vk::PhysicalDeviceVulkan13Features>()
            .dynamicRendering = vk::True;

        chain.get<vk::PhysicalDeviceVulkan13Features>()
            .synchronization2 = vk::True;

        chain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
            .extendedDynamicState = vk::True;

        vk::DeviceCreateInfo device_info;
        device_info.pNext = &chain.get<vk::PhysicalDeviceFeatures2>();
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = static_cast<std::uint32_t>(required_device_extension.size());
        device_info.ppEnabledExtensionNames = required_device_extension.data();

        this->device = vk::raii::Device(this->physical_device, device_info);
        this->graphics_queue = vk::raii::Queue(this->device, queue_index, 0);

       

    }

    void create_surface() {
        VkSurfaceKHR _surface;
        
        if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
            throw std::runtime_error("failed to create window surface!");
        }

        this->surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    vk::SurfaceFormatKHR choose_swap_sufrace_format(std::vector<vk::SurfaceFormatKHR> const& formats) {
        const auto iterator = std::ranges::find_if(formats, [](const auto& format) {return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
        return iterator != formats.end() ? *iterator : formats[0];
    }

    vk::PresentModeKHR choose_swap_present_mode(std::vector<vk::PresentModeKHR> const& available) {
        assert(std::ranges::any_of(available, [](auto mode) { return mode == vk::PresentModeKHR::eFifo; }));
        return std::ranges::any_of(available,[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D choose_swap_extent(vk::SurfaceCapabilitiesKHR const& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return { std::clamp<std::uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width), std::clamp<std::uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
    }
    void create_swap_chain() {
        vk::SurfaceCapabilitiesKHR capabilities = this->physical_device.getSurfaceCapabilitiesKHR(*this->surface);
        this->swap_chain_extent = choose_swap_extent(capabilities);
        std::uint32_t min_image_count = choose_swap_min_image_count(capabilities);
        std::vector<vk::SurfaceFormatKHR> formats = this->physical_device.getSurfaceFormatsKHR(*this->surface);
        this->swap_chain_surface_format = choose_swap_sufrace_format(formats);

        std::vector<vk::PresentModeKHR> present_modes = this->physical_device.getSurfacePresentModesKHR(*surface);
        vk::PresentModeKHR present_mode = choose_swap_present_mode(present_modes);

        std::uint32_t image_count = capabilities.minImageCount + 1;
        
        vk::SwapchainCreateInfoKHR swap_chain_info;
        swap_chain_info.surface = *this->surface;
        swap_chain_info.minImageCount = min_image_count;
        swap_chain_info.imageFormat = swap_chain_surface_format.format;
        swap_chain_info.imageColorSpace = swap_chain_surface_format.colorSpace;
        swap_chain_info.imageExtent = swap_chain_extent;
        swap_chain_info.imageArrayLayers = 1;
        swap_chain_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swap_chain_info.imageSharingMode = vk::SharingMode::eExclusive;
        swap_chain_info.preTransform = capabilities.currentTransform;
        swap_chain_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swap_chain_info.presentMode = present_mode;
        swap_chain_info.clipped = true;

        this->swap_chain = vk::raii::SwapchainKHR(this->device, swap_chain_info);
        this->swap_chain_images = this->swap_chain.getImages();
    }

    std::uint32_t choose_swap_min_image_count(vk::SurfaceCapabilitiesKHR const& capabilities) {
        auto min_image_count = std::max(3u, capabilities.minImageCount);
        if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < min_image_count)) {
            min_image_count = capabilities.maxImageCount;
        }
        return min_image_count;
    }

    void create_graphics_pipeline() {
        vk::raii::ShaderModule shader_module = create_shader_module(read_file("shaders/slang.spv"));
        vk::PipelineShaderStageCreateInfo vertex_info;
        vertex_info.stage = vk::ShaderStageFlagBits::eVertex;
        vertex_info.module = shader_module;
        vertex_info.pName = "vertMain";

        vk::PipelineShaderStageCreateInfo fragment_info;
        fragment_info.stage = vk::ShaderStageFlagBits::eFragment;
        fragment_info.module = shader_module;
        fragment_info.pName = "fragMain";

        vk::PipelineShaderStageCreateInfo stages[] = { vertex_info, fragment_info };

        auto binding = vertex::binding_description();
        auto attribute = vertex::attribute_descriptions();

        vk::PipelineVertexInputStateCreateInfo vertex_input_info;
        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.pVertexBindingDescriptions = &binding;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attribute.size());
        vertex_input_info.pVertexAttributeDescriptions = attribute.data();


        vk::PipelineInputAssemblyStateCreateInfo assembly;
        vk::PipelineViewportStateCreateInfo viewport_info;

        assembly.topology = vk::PrimitiveTopology::eTriangleList;
        viewport_info.viewportCount = 1;
        viewport_info.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.depthClampEnable = vk::False;
        rasterizer.rasterizerDiscardEnable = vk::False;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = vk::False;
        rasterizer.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisampling;
        multisampling.rasterizationSamples = msaa_samples;
        multisampling.sampleShadingEnable = vk::False;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil;
        depth_stencil.depthTestEnable = vk::True;
        depth_stencil.depthWriteEnable = vk::True;
        depth_stencil.depthCompareOp = vk::CompareOp::eLess;
        depth_stencil.depthBoundsTestEnable = vk::False;
        depth_stencil.stencilTestEnable = vk::False;

        vk::PipelineColorBlendAttachmentState blend_attachment;
        blend_attachment.blendEnable = vk::False;
        blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo blend_info;
        blend_info.logicOpEnable = vk::False;
        blend_info.logicOp = vk::LogicOp::eCopy;
        blend_info.attachmentCount = 1;
        blend_info.pAttachments = &blend_attachment;

        std::vector<vk::DynamicState> dynamic_states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamic_info;
        dynamic_info.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_info.pDynamicStates = dynamic_states.data();

        vk::PipelineLayoutCreateInfo pipeline_info;
        pipeline_info.setLayoutCount = 1;
        pipeline_info.pushConstantRangeCount = 0;
        pipeline_info.pSetLayouts = &*descriptor_layout;
        this->pipeline_layout = vk::raii::PipelineLayout(this->device, pipeline_info);

        vk::GraphicsPipelineCreateInfo graphics_info;

        graphics_info.stageCount = 2;
        graphics_info.pDepthStencilState = &depth_stencil;
        graphics_info.pStages = stages;
        graphics_info.pVertexInputState = &vertex_input_info;
        graphics_info.pInputAssemblyState = &assembly;
        graphics_info.pViewportState = &viewport_info;
        graphics_info.pRasterizationState = &rasterizer;
        graphics_info.pMultisampleState = &multisampling;
        graphics_info.pColorBlendState = &blend_info;
        graphics_info.pDynamicState = &dynamic_info;
        graphics_info.layout = pipeline_layout;
        graphics_info.renderPass = nullptr;

        vk::PipelineRenderingCreateInfo rendering_info;

        vk::Format depth_format = find_depth_format();

        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachmentFormats = &swap_chain_surface_format.format;
        rendering_info.depthAttachmentFormat = depth_format;

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_chain = {graphics_info, rendering_info};

        this->graphics_pipeline = vk::raii::Pipeline(this->device, nullptr, pipeline_chain.get<vk::GraphicsPipelineCreateInfo>());
    }

    [[nodiscard]] vk::raii::ShaderModule create_shader_module(const std::vector<char>& code) const {
        vk::ShaderModuleCreateInfo module_info;
        module_info.codeSize = code.size() * sizeof(char);
        module_info.pCode = reinterpret_cast<const std::uint32_t*>(code.data());

        vk::raii::ShaderModule shader_module{ this->device, module_info };
        return shader_module;
    }

    void create_image_views() {
        assert(swap_chain_image_view.empty());

        vk::ImageViewCreateInfo image_view_info;
        image_view_info.viewType = vk::ImageViewType::e2D;
        image_view_info.format = this->swap_chain_surface_format.format;
        image_view_info.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };


        for (auto& image : swap_chain_images)
        {
            image_view_info.image = image;
            swap_chain_image_view.emplace_back(device, image_view_info);
        }
    }

    void create_command_pool() {
        vk::CommandPoolCreateInfo pool_info;
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = this->queue_index;

        this->command_pool = vk::raii::CommandPool(this->device, pool_info);
    }

    void create_command_buffer() {
        vk::CommandBufferAllocateInfo allocation_info;
        allocation_info.commandPool = this->command_pool;
        allocation_info.level = vk::CommandBufferLevel::ePrimary;
        allocation_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        this->command_buffers = vk::raii::CommandBuffers(this->device, allocation_info);
    }

    void record_command_buffer(std::uint32_t index) {
        auto& command_buffer = command_buffers[frame_index];
        command_buffer.begin({});

        transition_image_layout(
            *depth_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::ImageAspectFlagBits::eDepth);

        transition_image_layout(
            *color_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor);

        transition_image_layout(swap_chain_images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);

        vk::ClearValue clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        
        vk::RenderingAttachmentInfo attachment_info;
        attachment_info.imageView = swap_chain_image_view[index];
        attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_info.clearValue = clear_color;

        vk::ClearValue color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.0f, 0);

        vk::RenderingAttachmentInfo depth_info;
        depth_info.imageView = depth_image_view;
        depth_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depth_info.loadOp = vk::AttachmentLoadOp::eClear;
        depth_info.storeOp = vk::AttachmentStoreOp::eDontCare;
        depth_info.clearValue = clear_depth;

        vk::RenderingAttachmentInfo color_info;
        color_info.imageView = color_view;
        color_info.imageLayout = vk::ImageLayout::eAttachmentOptimal;
        color_info.resolveMode = vk::ResolveModeFlagBits::eAverage;
        color_info.resolveImageView = swap_chain_image_view[index];
        color_info.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        color_info.loadOp = vk::AttachmentLoadOp::eClear;
        color_info.storeOp = vk::AttachmentStoreOp::eStore;
        color_info.clearValue = clear_color;

        vk::RenderingInfo rendering_info;
        rendering_info.renderArea = vk::Rect2D{ {0, 0}, swap_chain_extent };
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &color_info;
        rendering_info.pDepthAttachment = &depth_info;

     

        command_buffer.beginRendering(rendering_info);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
        command_buffer.setViewport(0, vk::Viewport(0.0f, static_cast<float>(swap_chain_extent.height), static_cast<float>(swap_chain_extent.width), -static_cast<float>(swap_chain_extent.height), 0.0f, 1.0f));
        command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swap_chain_extent));

        command_buffer.bindVertexBuffers(0, *vertex_buffer, {0});
        command_buffer.bindIndexBuffer(*index_buffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
        
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, *descriptor_sets[frame_index], nullptr);

        command_buffer.drawIndexed(static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);

        command_buffer.endRendering();

        transition_image_layout(swap_chain_images[index], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe, vk::ImageAspectFlagBits::eColor);

      

        command_buffer.end();
    }

    void transition_image_layout(vk::Image image, vk::ImageLayout old, vk::ImageLayout _new, vk::AccessFlags2 src_access, vk::AccessFlags2 dst_access, vk::PipelineStageFlags2 src_stage, vk::PipelineStageFlags2 dst_stage, vk::ImageAspectFlags image_aspect_flags) {
        vk::ImageMemoryBarrier2 barrier;
        barrier.srcStageMask = src_stage;
        barrier.srcAccessMask = src_access;
        barrier.dstStageMask = dst_stage;
        barrier.dstAccessMask = dst_access;
        barrier.oldLayout = old;
        barrier.newLayout = _new;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = { image_aspect_flags, 0, 1, 0, 1 };

        vk::DependencyInfo dependency_info;
        dependency_info.dependencyFlags = {};
        dependency_info.imageMemoryBarrierCount = 1;
        dependency_info.pImageMemoryBarriers = &barrier;

        this->command_buffers[frame_index].pipelineBarrier2(dependency_info);
    }

    void draw_frame() {
        vk::Result fence_result = device.waitForFences(*in_flight_fences[frame_index], vk::True, UINT64_MAX);
        

        if (fence_result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence!");
        }

        auto [result, index] = swap_chain.acquireNextImage(UINT64_MAX, *present_semaphores[frame_index], nullptr);
        
        if (result == vk::Result::eErrorOutOfDateKHR) {
            frame_buffer_resized = false;
            recreate_swap_chain();
            return;
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        device.resetFences(*in_flight_fences[frame_index]);
        
        record_command_buffer(index);
        update_uniform_buffer(frame_index);

        vk::PipelineStageFlags wait_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submit_info;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &*present_semaphores[frame_index];
        submit_info.pWaitDstStageMask = &wait_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &*command_buffers[frame_index];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &*render_finished_semaphores[index];

        graphics_queue.submit(submit_info, *in_flight_fences[frame_index]);

        vk::PresentInfoKHR present_info;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &*render_finished_semaphores[index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &*swap_chain;
        present_info.pImageIndices = &index;

        present_info.pResults = nullptr;

        try {
            result = graphics_queue.presentKHR(present_info);
        }
        catch (const vk::OutOfDateKHRError&) {
            recreate_swap_chain();
            return;
        }

        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR)) {
            recreate_swap_chain();
        }
        else assert(result == vk::Result::eSuccess);

        frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void create_sync_objects() {
        assert(present_semaphores.empty() && render_finished_semaphores.empty() && in_flight_fences.empty());
        
        for (size_t i = 0; i < swap_chain_images.size(); i++) 
        {
            render_finished_semaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            present_semaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
            in_flight_fences.emplace_back(device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
        }
    }

    void cleanup_swap_chain() {
        swap_chain_image_view.clear();
        swap_chain = nullptr;
    }

    void recreate_swap_chain() {

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        while ((width == 0 || height == 0) && !glfwWindowShouldClose(window)) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        if (glfwWindowShouldClose(window)) return;

        device.waitIdle();

        cleanup_swap_chain();

        create_swap_chain();
        create_image_views();
        create_color_resources();
        create_depth_resources();
    }

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo buffer_info;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = vk::SharingMode::eExclusive;

        vk::raii::Buffer buffer = vk::raii::Buffer(device, buffer_info);
        vk::MemoryRequirements memory = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo alloc_info;
        alloc_info.allocationSize = memory.size;
        alloc_info.memoryTypeIndex = find_memory_type(memory.memoryTypeBits, properties);

        vk::raii::DeviceMemory device_memory = vk::raii::DeviceMemory(device, alloc_info);
        
        buffer.bindMemory(*device_memory, 0);
        return {std::move(buffer), std::move(device_memory)};
    }

    void create_vertex_buffer() {
        vk::DeviceSize size = sizeof(vertices[0]) * vertices.size();

        auto [staging_buffer, staging_memory] = create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data = staging_memory.mapMemory(0, size);
        std::memcpy(data, vertices.data(), size);
        staging_memory.unmapMemory();

        std::tie(vertex_buffer, vertex_memory) = create_buffer(size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copy_buffer(staging_buffer, vertex_buffer, size);
    }

    void copy_buffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size) {
        vk::CommandBufferAllocateInfo alloc_info;
        alloc_info.commandPool = command_pool;
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandBufferCount = 1;

        vk::raii::CommandBuffer buffer = std::move(device.allocateCommandBuffers(alloc_info).front());
        buffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        buffer.copyBuffer(*src, *dst, vk::BufferCopy{ 0, 0, size });
        buffer.end();
        vk::SubmitInfo info;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &*buffer;
        graphics_queue.submit(info, nullptr);
        graphics_queue.waitIdle();
    }

    std::uint32_t find_memory_type(std::uint32_t filter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memory_properties = physical_device.getMemoryProperties();

        for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            if ((filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) return i;
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    void create_index_buffer() {
        vk::DeviceSize size = sizeof(indices[0]) * indices.size();
        auto [staging_buffer, staging_memory] = create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data = staging_memory.mapMemory(0, size);
        std::memcpy(data, indices.data(), static_cast<std::size_t>(size));
        staging_memory.unmapMemory();

        std::tie(index_buffer, index_memory) = create_buffer(size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copy_buffer(staging_buffer, index_buffer, size);
    }

    void create_descriptor_set_layout() {
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { {
            {
                0,
                vk::DescriptorType::eUniformBuffer,
                1,
                vk::ShaderStageFlagBits::eVertex
            },
            {
                1,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                vk::ShaderStageFlagBits::eFragment
            }
        } };

        vk::DescriptorSetLayoutCreateInfo info;
        info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        info.pBindings = bindings.data();

        descriptor_layout = vk::raii::DescriptorSetLayout(device, info);
    }

    void create_uniform_buffers() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DeviceSize size = sizeof(uniform_buffer_object);
            auto [buffer, memory] = create_buffer(size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            uniform_buffers.emplace_back(std::move(buffer));
            uniform_buffers_memory.emplace_back(std::move(memory));
            uniform_buffers_mapped.emplace_back(uniform_buffers_memory.back().mapMemory(0, size));
        }
    }

    void update_uniform_buffer(std::uint32_t image) {
        static auto start = std::chrono::high_resolution_clock::now();
        auto current = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();

        uniform_buffer_object ubo{};
        ubo.model =
            glm::rotate(
                glm::scale(
                    glm::mat4x4{ 1.0f },
                    glm::vec3{ 0.02f }
                ),
                time * 10 * glm::radians(90.0f),
                glm::vec3{ 0.0f, 0.0f, 1.0f }
            );
        ubo.view = glm::lookAt(glm::vec3{ 2.0f, 2.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, 0.1f });
        ubo.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(swap_chain_extent.width) / static_cast<float>(swap_chain_extent.height), 0.1f, 10.0f);

        std::memcpy(uniform_buffers_mapped[image], &ubo, sizeof(ubo));
    }

    void create_descriptor_pool() {
    std::array<vk::DescriptorPoolSize, 2> pool_sizes = {
        {
            {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
            {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT}
        }
    };

    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();

    descriptor_pool = vk::raii::DescriptorPool(device, pool_info);
}

    void create_descriptor_sets() {
        std::vector<vk::DescriptorSetLayout> layouts(
            MAX_FRAMES_IN_FLIGHT,
            *descriptor_layout
        );

        vk::DescriptorSetAllocateInfo allocation_info;
        allocation_info.descriptorPool = descriptor_pool;
        allocation_info.descriptorSetCount =
            static_cast<std::uint32_t>(layouts.size());
        allocation_info.pSetLayouts = layouts.data();

        descriptor_sets = device.allocateDescriptorSets(allocation_info);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            vk::DescriptorBufferInfo buffer_info;
            buffer_info.buffer = uniform_buffers[i];
            buffer_info.offset = 0;
            buffer_info.range = sizeof(uniform_buffer_object);

            vk::DescriptorImageInfo image_info;
            image_info.sampler = sampler;
            image_info.imageView = image_view;
            image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            std::array<vk::WriteDescriptorSet, 2> writes = {
                vk::WriteDescriptorSet{
                    descriptor_sets[i],
                    0,
                    0,
                    1,
                    vk::DescriptorType::eUniformBuffer,
                    nullptr,
                    &buffer_info
                },

                vk::WriteDescriptorSet{
                    descriptor_sets[i],
                    1,
                    0,
                    1,
                    vk::DescriptorType::eCombinedImageSampler,
                    &image_info,
                    nullptr
                }
            };

            device.updateDescriptorSets(writes, {});
        }
    }

    void create_texture_image() {
        int width, height, channels;

        stbi_uc* pixels = stbi_load("textures/oiia.png", &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels) {
            throw std::runtime_error("failed to load image!");
        }

        mip_levels = static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        vk::DeviceSize size = static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4;

        auto [buffer, memory] = create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data = memory.mapMemory(0, size);
        std::memcpy(data, pixels, size);
        memory.unmapMemory();

        stbi_image_free(pixels);

        std::tie(image, image_memory) = create_image(
            width,
            height,
            mip_levels,
            vk::SampleCountFlagBits::e1,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        auto command_buffer = begin_single_time_commands();

        transition_tex_image_layout(command_buffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mip_levels);

        copy_buffer_to_image(command_buffer, buffer, image, width, height);

        generate_mipmaps(command_buffer, image, vk::Format::eR8G8B8A8Srgb, width, height, mip_levels);

        end_single_time_commands(std::move(command_buffer));
    }

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> create_image(std::uint32_t width, std::uint32_t height, std::uint32_t mip_levels, vk::SampleCountFlagBits samples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags property) {
        vk::ImageCreateInfo info;
        info.imageType = vk::ImageType::e2D;
        info.format = format;
        info.extent = vk::Extent3D{width, height, 1};
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = vk::SampleCountFlagBits::e1;
        info.tiling = tiling;
        info.usage = usage;
        info.sharingMode = vk::SharingMode::eExclusive;
        info.mipLevels = mip_levels;
        info.samples = samples;

        vk::raii::Image image = vk::raii::Image(device, info);
        vk::MemoryRequirements requirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocation;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, property);
        vk::raii::DeviceMemory image_memory = vk::raii::DeviceMemory(device, allocation);
        image.bindMemory(image_memory, 0);
        return { std::move(image), std::move(image_memory) };
    }

    vk::raii::CommandBuffer begin_single_time_commands() {
        vk::CommandBufferAllocateInfo info;
        info.commandPool = command_pool;
        info.level = vk::CommandBufferLevel::ePrimary;
        info.commandBufferCount = 1;

        vk::raii::CommandBuffer buffer = std::move(vk::raii::CommandBuffers(device, info).front());
        vk::CommandBufferBeginInfo begin_info;
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        buffer.begin(begin_info);
        return buffer;
    }

    void end_single_time_commands(vk::raii::CommandBuffer&& buffer) {
        buffer.end();

        vk::SubmitInfo info;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &*buffer;
        graphics_queue.submit(info, nullptr);
        graphics_queue.waitIdle();
    }

    void transition_tex_image_layout(vk::raii::CommandBuffer& buffer, vk::raii::Image& image, vk::ImageLayout old, vk::ImageLayout _new, std::uint32_t mip_levels) {
        vk::ImageMemoryBarrier barrier;
        vk::ImageSubresourceRange range;

        range.aspectMask = vk::ImageAspectFlagBits::eColor;
        range.levelCount = mip_levels;
        range.layerCount = 1;

        barrier.oldLayout = old;
        barrier.newLayout = _new;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange = range;

        vk::PipelineStageFlags source, dest;
        if (old == vk::ImageLayout::eUndefined && _new == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            source = vk::PipelineStageFlagBits::eTopOfPipe;
            dest = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (old == vk::ImageLayout::eTransferDstOptimal && _new == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            source = vk::PipelineStageFlagBits::eTransfer;
            dest = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        buffer.pipelineBarrier(source, dest, {}, {}, {}, barrier);
    }

    void copy_buffer_to_image(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
    {
        vk::BufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
        region.imageOffset = vk::Offset3D{ 0, 0, 0 };
        region.imageExtent = vk::Extent3D{ width, height, 1 };

        command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    }

    void create_texture_image_views() {
        assert(swap_chain_image_view.empty());
        swap_chain_image_view.reserve(swap_chain_images.size());

        for (auto& image : swap_chain_images) {
            swap_chain_image_view.emplace_back(create_image_view(image, swap_chain_surface_format.format, vk::ImageAspectFlagBits::eColor, 1));
        }
    }

    void create_texture_sampler() {
        vk::PhysicalDeviceProperties properties = physical_device.getProperties();
        vk::SamplerCreateInfo info;
        info.magFilter = vk::Filter::eLinear;
        info.minFilter = vk::Filter::eLinear;
        info.mipmapMode = vk::SamplerMipmapMode::eLinear;
        info.addressModeU = vk::SamplerAddressMode::eRepeat;
        info.addressModeV = vk::SamplerAddressMode::eRepeat;
        info.addressModeW = vk::SamplerAddressMode::eRepeat;
        info.anisotropyEnable = vk::True;
        info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        info.compareEnable = vk::False;
        info.compareOp = vk::CompareOp::eAlways;
        info.mipLodBias = 0.0f;
        info.minLod = 0.0f;
        info.maxLod = vk::LodClampNone;

        sampler = vk::raii::Sampler(device, info);
    }

    vk::raii::ImageView create_image_view(vk::Image const& image, vk::Format format, vk::ImageAspectFlags flags, std::uint32_t mip_levels) {
        vk::ImageViewCreateInfo info;
        vk::ImageSubresourceRange range;

        range.aspectMask = flags;
        range.levelCount = mip_levels;
        range.layerCount = 1;

        info.image = image;
        info.viewType = vk::ImageViewType::e2D;
        info.format = format;
        info.subresourceRange = range;
        
        return vk::raii::ImageView(device, info);
    }

    void create_texture_image_view() {
        image_view = create_image_view(*image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mip_levels);
    }

    void create_depth_resources() {
        vk::Format depth_format = find_depth_format();

        std::tie(depth_image, depth_image_memory) = create_image(
            swap_chain_extent.width,
            swap_chain_extent.height,
            1,
            msaa_samples,
            depth_format,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        depth_image_view = create_image_view(
            *depth_image,
            depth_format,
            vk::ImageAspectFlagBits::eDepth, 1
        );
    }

    vk::Format find_supported_format(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
        for (const auto& format : candidates) {
            vk::FormatProperties properties = physical_device.getFormatProperties(format);

            if (((tiling == vk::ImageTiling::eLinear) && ((properties.linearTilingFeatures & features) == features)) || ((tiling == vk::ImageTiling::eOptimal) && ((properties.optimalTilingFeatures & features) == features))) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    vk::Format find_depth_format() {
        return find_supported_format({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    void load_model() {
        tinyobj::attrib_t attribute;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string error;


        if (!tinyobj::LoadObj(&attribute, &shapes, &materials, &error, "textures/oiia.obj")) {
            throw std::runtime_error(error);
        }

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                vertex v;

                v.position = {
                    attribute.vertices[3 * index.vertex_index + 0],
                    attribute.vertices[3 * index.vertex_index + 1],
                    attribute.vertices[3 * index.vertex_index + 2]
                };

                v.texture_coord = {
                    attribute.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attribute.texcoords[2 * index.texcoord_index + 1]
                };

                v.color = { 1.0f, 1.0f, 1.0f };

                vertices.push_back(v);
                indices.push_back(indices.size());
            }
        }
    }

    void generate_mipmaps(vk::raii::CommandBuffer& buffer, vk::raii::Image& image, vk::Format format, std::int32_t width, std::int32_t height, std::uint32_t mip_levels) {
        vk::FormatProperties properties = physical_device.getFormatProperties(format);
        

        if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        vk::ImageMemoryBarrier barrier;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        std::int32_t mip_width = width;
        std::int32_t mip_height = height;

        for (std::uint32_t i = 1; i < mip_levels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

            vk::ImageBlit blit;
            blit.srcSubresource = { vk::ImageAspectFlagBits::eColor, i - 1, 0, 1 };
            blit.srcOffsets = std::array<vk::Offset3D, 2>({ {}, {mip_width, mip_height, 1} });
            blit.dstSubresource = { vk::ImageAspectFlagBits::eColor, i, 0, 1 };
            blit.dstOffsets = std::array<vk::Offset3D, 2>({{}, {1 < mip_width ? mip_width / 2 : 1, 1 < mip_height ? mip_height / 2 : 1, 1}});

            buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

            if (1 < mip_width) mip_width /= 2;
            if (1 < mip_height) mip_height /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
    }

    vk::SampleCountFlagBits get_max_usable_samples() {
        vk::PhysicalDeviceProperties properties = physical_device.getProperties();
        vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

        return vk::SampleCountFlagBits::e1;
    }

    void create_color_resources() {
        vk::Format format = swap_chain_surface_format.format;

        std::tie(color_image, color_memory) = create_image(swap_chain_extent.width, swap_chain_extent.height, 1, msaa_samples, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
        color_view = create_image_view(color_image, format, vk::ImageAspectFlagBits::eColor, 1);

    }

    static std::vector<char> read_file(const std::string name) {
        std::ifstream file(name, std::ios::ate | std::ios::binary);

        if (!file) throw std::runtime_error("failed to open file!");

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        file.close();

        return buffer;
    }
};

std::vector<const char*> required_instance_extensions()
{
    std::uint32_t glfw3_extension_count = 0;
    auto glfw3_extensions = glfwGetRequiredInstanceExtensions(&glfw3_extension_count);

    std::vector<const char*> extensions(glfw3_extensions, glfw3_extensions + glfw3_extension_count);

    if (enable_validation_layers)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

int main()
{
    try {
        vulkan_instance instance;
        instance.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}