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
 */
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec4 color;
layout (location = 1) in vec2 tex_coord;

layout (location = 0) out vec4 out_color;

void main() {
    vec4 texture_color = texture(tex, tex_coord.xy);
    if (texture_color.r > 0.02) {
        out_color = vec4((color.rgb * texture_color.rrr), texture_color.r);
    } else {
        discard;
    }
}