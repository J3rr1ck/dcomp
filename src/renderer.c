#include "renderer.h"
#include "compositor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>


#include "vert.h"
#include "frag.h"

static VkInstance create_instance(void) {
    const char *exts[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    };
    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "dcomp",
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = exts,
    };
    VkInstance inst;
    VkResult res = vkCreateInstance(&info, NULL, &inst);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", res);
        exit(1);
    }
    return inst;
}

static VkPhysicalDevice pick_phys_dev(VkInstance inst) {
    uint32_t count;
    vkEnumeratePhysicalDevices(inst, &count, NULL);
    VkPhysicalDevice *devs = malloc(count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(inst, &count, devs);
    VkPhysicalDevice chosen = devs[0];
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devs[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            chosen = devs[i];
            break;
        }
    }
    free(devs);
    return chosen;
}

static uint32_t find_queue(VkPhysicalDevice dev) {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, NULL);
    VkQueueFamilyProperties *props = malloc(count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, props);
    uint32_t chosen = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            chosen = i;
            break;
        }
    }
    free(props);
    return chosen;
}

static VkDevice create_device(VkPhysicalDevice phys, uint32_t qf) {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = qf,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    const char *exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qinfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = exts,
    };
    VkDevice dev;
    vkCreateDevice(phys, &info, NULL, &dev);
    return dev;
}

static VkShaderModule create_shader(VkDevice dev, const void *code, size_t sz) {
    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sz,
        .pCode = (const uint32_t *)code,
    };
    VkShaderModule mod;
    vkCreateShaderModule(dev, &info, NULL, &mod);
    return mod;
}

struct dcomp_renderer *renderer_create(struct dcomp_server *server, int fd) {
    (void)fd;
    struct dcomp_renderer *r = calloc(1, sizeof(struct dcomp_renderer));

    r->instance = create_instance();
    r->phys_dev = pick_phys_dev(r->instance);
    r->queue_family = find_queue(r->phys_dev);
    r->device = create_device(r->phys_dev, r->queue_family);
    vkGetDeviceQueue(r->device, r->queue_family, 0, &r->queue);

    // Create Wayland surface
    VkWaylandSurfaceCreateInfoKHR winfo = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = (struct wl_display *)server->display,
    };
    VkResult res = vkCreateWaylandSurfaceKHR(r->instance, &winfo, NULL, &r->surface);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateWaylandSurfaceKHR failed: %d\n", res);
        exit(1);
    }

    // Get surface capabilities and choose format
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->phys_dev, r->surface, &caps);
    uint32_t fmt_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(r->phys_dev, r->surface, &fmt_count, NULL);
    VkSurfaceFormatKHR *fmts = malloc(fmt_count * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(r->phys_dev, r->surface, &fmt_count, fmts);
    VkSurfaceFormatKHR fmt = fmts[0];
    if (fmt_count == 0) fmt.format = VK_FORMAT_B8G8R8A8_UNORM;
    free(fmts);

    r->output_w = caps.currentExtent.width;
    r->output_h = caps.currentExtent.height;
    if (r->output_w == 0xFFFFFFFF || r->output_h == 0xFFFFFFFF) {
        r->output_w = 1920;
        r->output_h = 1080;
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR sc_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = r->surface,
        .minImageCount = 2,
        .imageFormat = fmt.format,
        .imageColorSpace = fmt.colorSpace,
        .imageExtent = { r->output_w, r->output_h },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    vkCreateSwapchainKHR(r->device, &sc_info, NULL, &r->swapchain);

    // Get swapchain images
    uint32_t sc_len;
    vkGetSwapchainImagesKHR(r->device, r->swapchain, &sc_len, NULL);
    r->swapchain_len = sc_len;
    r->swapchain_images = malloc(sc_len * sizeof(VkImage));
    vkGetSwapchainImagesKHR(r->device, r->swapchain, &sc_len, r->swapchain_images);

    // Create image views
    r->swapchain_views = malloc(sc_len * sizeof(VkImageView));
    for (uint32_t i = 0; i < sc_len; i++) {
        VkImageViewCreateInfo vinfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = r->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = fmt.format,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        };
        vkCreateImageView(r->device, &vinfo, NULL, &r->swapchain_views[i]);
    }

    // Render pass
    VkAttachmentDescription att = {
        .format = fmt.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference ref = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ref,
    };
    VkRenderPassCreateInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &att,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };
    vkCreateRenderPass(r->device, &rp_info, NULL, &r->render_pass);

    // Framebuffers
    r->framebuffers = malloc(sc_len * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < sc_len; i++) {
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = r->render_pass,
            .attachmentCount = 1,
            .pAttachments = &r->swapchain_views[i],
            .width = r->output_w,
            .height = r->output_h,
            .layers = 1,
        };
        vkCreateFramebuffer(r->device, &fb_info, NULL, &r->framebuffers[i]);
    }

    // Command pool (must be before command buffer allocation)
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = r->queue_family,
    };
    vkCreateCommandPool(r->device, &pool_info, NULL, &r->cmd_pool);

    // Command buffers
    VkCommandBufferAllocateInfo cmd_alloc = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = r->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = sc_len,
    };
    r->cmd_bufs = malloc(sc_len * sizeof(VkCommandBuffer));
    vkAllocateCommandBuffers(r->device, &cmd_alloc, r->cmd_bufs);

    // Shader modules
    r->vert_mod = create_shader(r->device, vert_data, vert_size);
    r->frag_mod = create_shader(r->device, frag_data, frag_size);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding bind = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo dsl_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &bind,
    };
    vkCreateDescriptorSetLayout(r->device, &dsl_info, NULL, &r->desc_set_layout);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pl_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &r->desc_set_layout,
    };
    vkCreatePipelineLayout(r->device, &pl_info, NULL, &r->pipeline_layout);

    // Sampler
    VkSamplerCreateInfo samp_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    vkCreateSampler(r->device, &samp_info, NULL, &r->sampler);

    // Graphics pipeline
    VkPipelineShaderStageCreateInfo stages[2] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = r->vert_mod, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = r->frag_mod, .pName = "main" },
    };
    VkPipelineVertexInputStateCreateInfo vertex_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo input_asm = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    };
    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    VkPipelineRasterizationStateCreateInfo raster = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_NONE,
    };
    VkPipelineMultisampleStateCreateInfo ms = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
    VkPipelineColorBlendAttachmentState blend_att = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_att,
    };
    VkDynamicState dyns[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dyns,
    };
    VkGraphicsPipelineCreateInfo pipe_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_info,
        .pInputAssemblyState = &input_asm,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster,
        .pMultisampleState = &ms,
        .pColorBlendState = &blend,
        .pDynamicState = &dyn,
        .layout = r->pipeline_layout,
        .renderPass = r->render_pass,
    };
    vkCreateGraphicsPipelines(r->device, NULL, 1, &pipe_info, NULL, &r->pipeline);

    // Descriptor pool
    VkDescriptorPoolSize pool_sz = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 };
    VkDescriptorPoolCreateInfo dp_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 64,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_sz,
    };
    vkCreateDescriptorPool(r->device, &dp_info, NULL, &r->desc_pool);

    return r;
}

void renderer_destroy(struct dcomp_renderer *r) {
    if (!r) return;
    // TODO: proper cleanup
    free(r);
}

void renderer_commit(struct dcomp_renderer *r) {
    if (!r || !r->swapchain) return;

    VkSemaphoreCreateInfo sem_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore sem;
    vkCreateSemaphore(r->device, &sem_info, NULL, &sem);

    uint32_t idx;
    vkAcquireNextImageKHR(r->device, r->swapchain, UINT64_MAX, sem, VK_NULL_HANDLE, &idx);

    VkCommandBuffer cmd = r->cmd_bufs[idx];
    VkCommandBufferBeginInfo begin = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &begin);

    VkClearValue clear = { .color = { .float32 = {0.2f, 0.3f, 0.4f, 1.0f} } };
    VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = r->render_pass,
        .framebuffer = r->framebuffers[idx],
        .renderArea = { .offset = {0,0}, .extent = { r->output_w, r->output_h } },
        .clearValueCount = 1,
        .pClearValues = &clear,
    };
    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r->pipeline);

    VkViewport vp = { 0, 0, (float)r->output_w, (float)r->output_h, 0, 1 };
    VkRect2D sc = { {0,0}, { r->output_w, r->output_h } };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    // Draw a full-screen quad (4 vertices as triangle strip)
    vkCmdDraw(cmd, 4, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &sem,
    };
    VkFence fence = r->fences[idx];
    vkResetFences(r->device, 1, &fence);
    vkQueueSubmit(r->queue, 1, &submit, fence);

    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &r->swapchain,
        .pImageIndices = &idx,
        .waitSemaphoreCount = 0,
    };
    vkQueuePresentKHR(r->queue, &present);

    vkDestroySemaphore(r->device, sem, NULL);
}

struct dcomp_texture *renderer_create_texture(struct dcomp_renderer *r,
                                               uint32_t w, uint32_t h,
                                               void *data) {
    // Simplified texture creation (no proper staging, just for demo)
    VkImageCreateInfo img_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .extent = { w, h, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };
    VkImage image;
    vkCreateImage(r->device, &img_info, NULL, &image);

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(r->device, image, &mem_req);
    VkMemoryAllocateInfo alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = 0,
    };
    VkDeviceMemory mem;
    vkAllocateMemory(r->device, &alloc, NULL, &mem);
    vkBindImageMemory(r->device, image, mem, 0);

    // Upload data (simplified)
    void *map;
    vkMapMemory(r->device, mem, 0, VK_WHOLE_SIZE, 0, &map);
    memcpy(map, data, w * h * 4);
    vkUnmapMemory(r->device, mem);

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    VkImageView view;
    vkCreateImageView(r->device, &view_info, NULL, &view);

    struct dcomp_texture *tex = calloc(1, sizeof(struct dcomp_texture));
    tex->image = image;
    tex->view = view;
    tex->mem = mem;
    tex->w = w;
    tex->h = h;
    return tex;
}

void renderer_destroy_texture(struct dcomp_renderer *r, struct dcomp_texture *tex) {
    if (!tex) return;
    vkDestroyImageView(r->device, tex->view, NULL);
    vkDestroyImage(r->device, tex->image, NULL);
    vkFreeMemory(r->device, tex->mem, NULL);
    free(tex);
}

void renderer_draw_view(struct dcomp_renderer *r, struct dcomp_texture *tex,
                        int32_t x, int32_t y, uint32_t w, uint32_t h) {
    (void)r; (void)tex; (void)x; (void)y; (void)w; (void)h;
}
