#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>

struct dcomp_server;

struct dcomp_renderer {
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family;
    VkCommandPool cmd_pool;
    VkDescriptorSetLayout desc_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkSampler sampler;
    VkDescriptorPool desc_pool;

    // output (monitor) info
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage *swapchain_images;
    VkImageView *swapchain_views;
    uint32_t swapchain_len;
    VkFramebuffer *framebuffers;
    VkCommandBuffer *cmd_bufs;
    VkFence *fences;
    VkRenderPass render_pass;

    uint32_t output_w, output_h;

    // shader modules
    VkShaderModule vert_mod, frag_mod;
};

struct dcomp_renderer *renderer_create(struct dcomp_server *server, int fd);
void renderer_destroy(struct dcomp_renderer *r);
void renderer_commit(struct dcomp_renderer *r);

// texture management
struct dcomp_texture {
    VkImage image;
    VkImageView view;
    VkDeviceMemory mem;
    VkSampler sampler;
    uint32_t w, h;
};

struct dcomp_texture *renderer_create_texture(struct dcomp_renderer *r,
                                               uint32_t w, uint32_t h,
                                               void *data);
void renderer_destroy_texture(struct dcomp_renderer *r, struct dcomp_texture *tex);

// draw a view at position
void renderer_draw_view(struct dcomp_renderer *r, struct dcomp_texture *tex,
                        int32_t x, int32_t y, uint32_t w, uint32_t h);

#endif
