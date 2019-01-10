//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_texture.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

struct GlobeTextureData {
    bool setup_for_render_target;
    bool is_color;
    bool is_depth;
    bool is_stencil;
    uint32_t width;
    uint32_t height;
    uint32_t num_components;
    VkSampleCountFlagBits vk_sample_count;
    VkFormat vk_format;
    VkFormatProperties vk_format_props;
    VkSampler vk_sampler;
    VkImage vk_image;
    VkImageLayout vk_image_layout;
    VkDeviceMemory vk_device_memory;
    VkDeviceSize vk_allocated_size;
    VkImageView vk_image_view;
    std::vector<uint8_t> raw_data;
    GlobeTextureData* staging_texture_data;
};

class GlobeResourceManager;

class GlobeTexture {
   public:
    static GlobeTexture* LoadFromFile(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                      VkCommandBuffer vk_command_buffer, const std::string& texture_name,
                                      const std::string& directory);
    static GlobeTexture* CreateRenderTarget(const GlobeResourceManager* resource_manager, VkDevice vk_device,
                                            VkCommandBuffer vk_command_buffer, uint32_t width, uint32_t height,
                                            VkFormat vk_format);

    GlobeTexture(const GlobeResourceManager* resource_manager, VkDevice vk_device, const std::string& texture_name,
                 GlobeTextureData* texture_data);
    ~GlobeTexture();

    uint32_t Width() { return _width; }
    uint32_t Height() { return _height; }
    bool UsesStagingTexture() { return _uses_staging_texture; }
    GlobeTexture* StagingTexture() { return _staging_texture; }
    bool DeleteStagingTexture();
    VkFormat GetVkFormat() { return _vk_format; }
    VkSampleCountFlagBits GetVkSampleCount() { return _vk_sample_count; }
    VkSampler GetVkSampler() { return _vk_sampler; }
    VkImage GetVkImage() { return _vk_image; }
    VkImageView GetVkImageView() { return _vk_image_view; }
    VkImageLayout GetVkImageLayout() { return _vk_image_layout; }
    VkAttachmentDescription GenVkAttachmentDescription();
    VkAttachmentReference GenVkAttachmentReference(uint32_t attachment_index);

   private:
    bool _setup_for_render_target;
    bool _is_color;
    bool _is_depth;
    bool _is_stencil;
    const GlobeResourceManager* _globe_resource_mgr;
    VkPhysicalDevice _vk_physical_device;
    VkDevice _vk_device;
    std::string _texture_name;
    bool _uses_staging_texture;
    GlobeTexture* _staging_texture;
    uint32_t _width;
    uint32_t _height;
    VkSampleCountFlagBits _vk_sample_count;
    VkFormat _vk_format;
    VkSampler _vk_sampler;
    VkImage _vk_image;
    VkImageLayout _vk_image_layout;
    VkDeviceMemory _vk_device_memory;
    VkDeviceSize _vk_allocated_size;
    VkImageView _vk_image_view;
};
