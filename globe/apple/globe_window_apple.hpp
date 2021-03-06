/*
 * Copyright (c) 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Young, LunarG <marky@lunarg.com>
 */

#if defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)

#error("Unsupported/Unfinished target")

#pragma once

#include "globe_window.hpp"

class GlobeWindowApple {
   public:
    GlobeWindowApple(GlobeApp *associated_app, const std::string &name);
    virtual ~GlobeWindowApple();

    bool CreatePlatformWindow(VkInstance, VkPhysicalDevice phys_device, uint32_t width, uint32_t height);

    bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                    void **next);

    bool CheckAndRetrieveDeviceExtensions(const VkPhysicalDevice &physical_device,
                                          std::vector<std::string> &extensions);
    bool CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface);

    void SetMoltenVkView(void *view) { _molten_view = view; }

   private:
       void * _molten_view;
};

#endif
