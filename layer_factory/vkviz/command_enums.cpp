/* Copyright (C) 2018 Google Inc.
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
 * Author: William Henning <whenning@google.com>
 */

#include <vector>

#include "command_enums.h"

namespace {
std::vector<std::string> kCommandNames{
    "None",
    "Begin Command Buffer",
    "Begin Debug Utils Label EXT",
    "Begin Query",
    "Begin Renderpass",
    "Bind Descriptor Sets",
    "Bind Index Buffer",
    "Bind Pipeline",
    "Bind Vertex Buffers",
    "Blit Image",
    "Clear Attachments",
    "Clear Color Image",
    "Clear Depth Stencil Image",
    "Copy Buffer",
    "Copy Buffer to Image",
    "Copy Image",
    "Copy Image to Buffer",
    "Copy Query Pool Results",
    "Debug Marker Begin EXT",
    "Debug Marker End EXT",
    "Debug Marker Insert EXT",
    "Dispatch",
    "Dispatch Base",
    "Dispatch Base KHR",
    "Dispatch Indirect",
    "Draw",
    "Draw Indexed",
    "Draw Indexed Indirect",
    "Draw Indexed Indirect Count AMD",
    "Draw Indirect",
    "Draw Indirect Count AMD",
    "End Command Buffer",
    "End Debug Utils Label EXT",
    "End Query",
    "End Renderpass",
    "Execute Commands",
    "Fill Buffer",
    "Insert Debug Utils Label EXT",
    "Next Subpass",
    "Pipeline Barrier",
    "PROCESSCommand SNVX",
    "Push Constants",
    "Push Descriptor Set KHR",
    "Push Descriptor Set With Template KHR",
    "Reserve Space For Commands NVX",
    "Reset Command Buffer",
    "Reset Event",
    "Reset Query Pool",
    "Resolve Image",
    "Set Blend Constants",
    "Set Depth Bias",
    "Set Depth Bounds",
    "Set Device Mask",
    "Set Device Mask KHR",
    "Set Discard Rectangle EXT",
    "Set Event",
    "Set Line Width",
    "Set Sample Locations EXT",
    "Set Scissor",
    "Set Stencil Compare Mask",
    "Set Stencil Reference",
    "Set Stencil Write Mask",
    "Set Viewport",
    "Set Viewport W Scaling NV",
    "Update Buffer",
    "Wait Events",
    "Write Buffer Marker AMD",
    "Write Timestamp",
};
}

std::string cmdToString(CMD_TYPE cmd) { return kCommandNames[cmd]; }
