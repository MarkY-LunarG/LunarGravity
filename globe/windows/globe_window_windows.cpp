/*
 * Copyright (c) 2018-2019 LunarG, Inc.
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

#if defined(VK_USE_PLATFORM_WIN32_KHR)

#include <cstring>

#include "globe_logger.hpp"
#include "globe_event.hpp"
#include "globe_window_windows.hpp"

#include "shellscalingapi.h"

GlobeWindowWindows::GlobeWindowWindows(GlobeApp *app, const std::string &name, bool start_fullscreen)
    : GlobeWindow(app, name, start_fullscreen) {}

GlobeWindowWindows::~GlobeWindowWindows() {}

bool GlobeWindowWindows::PrepareCreateInstanceItems(std::vector<std::string> &layers,
                                                    std::vector<std::string> &extensions, void **next) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    uint32_t extension_count = 0;

    // Determine the number of instance extensions supported
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    // Query the available instance extensions
    std::vector<VkExtensionProperties> extension_properties;

    extension_properties.resize(extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_properties.data());
    if (VK_SUCCESS != result || 0 >= extension_count) {
        return false;
    }

    bool found_surface_ext = false;
    bool found_platform_surface_ext = false;

    for (uint32_t i = 0; i < extension_count; i++) {
        if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_surface_ext = true;
            extensions.push_back(extension_properties[i].extensionName);
        }
        if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extension_properties[i].extensionName)) {
            found_platform_surface_ext = 1;
            extensions.push_back(extension_properties[i].extensionName);
        }
    }

    if (!found_platform_surface_ext) {
        logger.LogFatalError(
            "vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            " extension.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.");
    }

    return found_surface_ext && found_platform_surface_ext;
}

bool GlobeWindowWindows::CreateVkSurface(VkInstance instance, VkPhysicalDevice phys_device, VkSurfaceKHR &surface) {
    GlobeLogger &logger = GlobeLogger::getInstance();

    if (_vk_surface != VK_NULL_HANDLE) {
        logger.LogInfo("GlobeWindowWindows::CreateVkSurface but surface already created.  Using existing one.");
        surface = _vk_surface;
        return true;
    }

    // Create a WSI surface for the window:
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = _instance_handle;
    createInfo.hwnd = _window_handle;

    PFN_vkCreateWin32SurfaceKHR fpCreateSurface =
        reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
    if (nullptr == fpCreateSurface || VK_SUCCESS != fpCreateSurface(instance, &createInfo, nullptr, &surface)) {
        logger.LogError("Failed call to vkCreateWin32SurfaceKHR");
        return false;
    }

    return true;
}

// MS-Windows event handling function:
static LRESULT CALLBACK GlobeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GlobeEventType globe_event_type = GLOBE_EVENT_NONE;
    GlobeMouseButton mouse_button = GLOBE_MOUSEBUTTON_NONE;
    switch (uMsg) {
        case WM_CREATE: {
            CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
            GlobeWindowWindows *globe_window = reinterpret_cast<GlobeWindowWindows *>(cs->lpCreateParams);
            SetWindowLongPtr(hWnd, 0, (LONG_PTR)globe_window);
        } break;
        case WM_CLOSE:
            GlobeEventList::getInstance().InsertEvent(GlobeEvent(GLOBE_EVENT_QUIT));
            break;
        case WM_PAINT: {
            // The validation callback calls MessageBox which can generate paint
            // events - don't make more Vulkan calls if we got here from the
            // callback
            GlobeEventList::getInstance().InsertEvent(GlobeEvent(GLOBE_EVENT_WINDOW_DRAW));
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_GETMINMAXINFO:  // set window's minimum size
        {
            GlobeWindowWindows *globe_window = reinterpret_cast<GlobeWindowWindows *>(GetWindowLongPtr(hWnd, 0));
            if (nullptr != globe_window) {
                ((MINMAXINFO *)lParam)->ptMinTrackSize = globe_window->Minsize();
            }
            return 0;
        }
        case WM_KEYDOWN: {
            globe_event_type = GLOBE_EVENT_KEY_PRESS;
            break;
        }
        case WM_KEYUP: {
            globe_event_type = GLOBE_EVENT_KEY_RELEASE;
            break;
        }
        case WM_LBUTTONDOWN: {
            globe_event_type = GLOBE_EVENT_MOUSE_PRESS;
            mouse_button = GLOBE_MOUSEBUTTON_LEFT;
            break;
        }
        case WM_LBUTTONUP: {
            globe_event_type = GLOBE_EVENT_MOUSE_RELEASE;
            mouse_button = GLOBE_MOUSEBUTTON_LEFT;
            break;
        }
        case WM_MBUTTONDOWN: {
            globe_event_type = GLOBE_EVENT_MOUSE_PRESS;
            mouse_button = GLOBE_MOUSEBUTTON_MIDDLE;
            break;
        }
        case WM_MBUTTONUP: {
            globe_event_type = GLOBE_EVENT_MOUSE_RELEASE;
            mouse_button = GLOBE_MOUSEBUTTON_MIDDLE;
            break;
        }
        case WM_RBUTTONDOWN: {
            globe_event_type = GLOBE_EVENT_MOUSE_PRESS;
            mouse_button = GLOBE_MOUSEBUTTON_RIGHT;
            break;
        }
        case WM_RBUTTONUP: {
            globe_event_type = GLOBE_EVENT_MOUSE_RELEASE;
            mouse_button = GLOBE_MOUSEBUTTON_RIGHT;
            break;
        }
        case WM_SIZE: {
            // Resize the application to the new window size.
            GlobeEvent *event = new GlobeEvent(GLOBE_EVENT_WINDOW_RESIZE);
            uint32_t width = lParam & 0xffff;
            uint32_t height = (lParam & 0xffff0000) >> 16;
            event->_data.resize.width = width;
            event->_data.resize.height = height;
            GlobeEventList::getInstance().InsertEvent(*event);
            delete event;
            break;
        }
        default:
            break;
    }
    if (globe_event_type == GLOBE_EVENT_MOUSE_PRESS || globe_event_type == GLOBE_EVENT_MOUSE_RELEASE) {
        GlobeEvent *event = new GlobeEvent(globe_event_type);
        event->_data.mouse_button = mouse_button;
        GlobeEventList::getInstance().InsertEvent(*event);
    } else if (globe_event_type == GLOBE_EVENT_KEY_PRESS || globe_event_type == GLOBE_EVENT_KEY_RELEASE) {
        switch (wParam) {
            case VK_ESCAPE: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ESCAPE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_LEFT: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_LEFT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_RIGHT: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_RIGHT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_UP: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_UP;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_DOWN: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ARROW_DOWN;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_SPACE: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_SPACE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_INSERT: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_INSERT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_DELETE: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_DELETE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_BACK: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_BACKSPACE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_TAB: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_TAB;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_CLEAR: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_CLEAR;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_RETURN: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_ENTER;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_PRIOR: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_PAGE_UP;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_NEXT: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_PAGE_DOWN;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_HOME: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_HOME;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case VK_END: {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_END;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            } break;
            case 0x30: {  // 0
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_0;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x31: {  // 1
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_1;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x32: {  // 2
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_2;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x33: {  // 3
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_3;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x34: {  // 4
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_4;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x35: {  // 5
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_5;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x36: {  // 6
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_6;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x37: {  // 7
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_7;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x38: {  // 8
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_8;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x39: {  // 9
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_9;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x41: {  // A
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_A;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x42: {  // B
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_B;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x43: {  // C
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_C;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x44: {  // D
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_D;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x45: {  // E
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_E;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x46: {  // F
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x47: {  // G
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_G;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x48: {  // H
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_H;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x49: {  // I
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_I;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x4A: {  // J
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_J;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x4B: {  // K
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_K;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x4C: {  // L
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_L;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x4D: {  // M
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_M;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x4E: {  // N
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_N;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x4F: {  // O
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_O;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x50: {  // P
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_P;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x51: {  // Q
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_Q;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x52: {  // R
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_R;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x53: {  // S
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_S;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x54: {  // T
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_T;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x55: {  // U
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_U;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x56: {  // V
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_V;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x57: {  // W
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_W;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x58: {  // X
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_X;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x59: {  // Y
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_Y;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case 0x5A: {  // Z
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_Z;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_LWIN: {  // Left window
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_LEFT_OS;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_RWIN: {  // Right window
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_RIGHT_OS;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_ADD: {  // +
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_EQUAL;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_SUBTRACT:
            case VK_OEM_MINUS: {  // -
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_DASH;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_DECIMAL:
            case VK_OEM_PERIOD: {  // .
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_PERIOD;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_DIVIDE:
            case VK_OEM_2: {  // forward slash
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_FORWARD_SLASH;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F1: {  // F1
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F1;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F2: {  // F2
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F2;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F3: {  // F3
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F3;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F4: {  // F4
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F4;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F5: {  // F5
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F5;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F6: {  // F6
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F6;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F7: {  // F7
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F7;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F8: {  // F8
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F8;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F9: {  // F9
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F9;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F10: {  // F10
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F10;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F11: {  // F11
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F11;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_F12: {  // F12
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_F12;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_LSHIFT: {  // Left shift
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_LEFT_SHIFT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_RSHIFT: {  // Right shift
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_RIGHT_SHIFT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_LCONTROL: {  // Left control
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_LEFT_CTRL;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_RCONTROL: {  // Right control
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_RIGHT_CTRL;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_LMENU: {  // Left alt
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_LEFT_ALT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_RMENU: {  // Right alt
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_RIGHT_ALT;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_1: {  // Semicolon
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_SEMICOLON;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_COMMA: {  // ,
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_COMMA;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_3: {  // Backquote/tilda
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_BACK_QUOTE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_4: {  // {
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_LEFT_BRACKET;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_5: {  // Backslash
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_BACKSLASH;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_6: {  // }
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_RIGHT_BRACKET;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
            case VK_OEM_7: {  // '
                GlobeEvent *event = new GlobeEvent(globe_event_type);
                event->_data.key = GLOBE_KEYNAME_QUOTE;
                GlobeEventList::getInstance().InsertEvent(*event);
                delete event;
            }
        }

        return 0;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

bool GlobeWindowWindows::CreatePlatformWindow(VkInstance instance, VkPhysicalDevice phys_device, uint32_t width,
                                              uint32_t height) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    _width = width;
    _height = height;
    _vk_instance = instance;
    _vk_physical_device = phys_device;

    int windows_error = GetLastError();
    DWORD windows_extended_style = {};
    DWORD windows_style = {};
    WNDCLASSEX win_class_ex = {};
    if (NULL == _instance_handle) {
        _instance_handle = GetModuleHandle(NULL);
    }

    // Initialize the window class structure:
    win_class_ex.cbSize = sizeof(WNDCLASSEX);
    win_class_ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    win_class_ex.lpfnWndProc = GlobeWindowProc;
    win_class_ex.cbClsExtra = 0;
    win_class_ex.cbWndExtra = 0;
    win_class_ex.hInstance = _instance_handle;
    win_class_ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class_ex.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    win_class_ex.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class_ex.hbrBackground = NULL;
    win_class_ex.lpszMenuName = NULL;
    win_class_ex.lpszClassName = _name.c_str();
    // Register window class:
    if (!RegisterClassEx(&win_class_ex)) {
        logger.LogFatalError("Unexpected error trying to start the application!");
        exit(1);
    }
    windows_error = GetLastError();
    if (_is_fullscreen) {
        DEVMODE dev_mode = {};
        dev_mode.dmSize = sizeof(DEVMODE);
        dev_mode.dmPelsWidth = width;
        dev_mode.dmPelsHeight = height;
        dev_mode.dmBitsPerPel = 32;
        dev_mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&dev_mode, CDS_FULLSCREEN)) {
            _is_fullscreen = false;
        } else {
            windows_extended_style = WS_EX_APPWINDOW;
            windows_style = WS_POPUP | WS_VISIBLE;
            ShowCursor(FALSE);
        }
    }
    if (!_is_fullscreen) {
        windows_extended_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        windows_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    }

    // Create window with the registered class:
    RECT wr = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    if (!AdjustWindowRectEx(&wr, windows_style, FALSE, windows_extended_style)) {
        logger.LogFatalError("Cannot adjust window size!");
        exit(1);
    }
    windows_error = GetLastError();
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_SYSTEM_DPI_AWARE);
    _window_handle = CreateWindowEx(windows_extended_style, _name.c_str(), _name.c_str(), windows_style, 0, 0,
                                    wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, _instance_handle, this);
    windows_error = GetLastError();
    if (!_window_handle) {
        logger.LogFatalError("Cannot create a window in which to draw!");
        exit(1);
    }
    // Window client area size must be at least 1 pixel high, to prevent crash.
    _minsize.x = GetSystemMetrics(SM_CXMINTRACK);
    _minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;

    _window_created = true;

    if (!CreateVkSurface(instance, phys_device, _vk_surface)) {
        logger.LogError("Failed to create vulkan surface for window");
        return false;
    }

    logger.LogInfo("Created Platform Window");
    return true;
}

bool GlobeWindowWindows::DestroyPlatformWindow() {
    if (_is_fullscreen) {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(TRUE);
    }
    return GlobeWindow::DestroyPlatformWindow();
}

#endif
