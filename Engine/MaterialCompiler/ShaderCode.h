#pragma once
namespace gear {
const char COMMON_DATA_FS_DATA[] = "layout(std140, set = 0, binding = 1) uniform FrameUniforms {\n	mat4 view_matrix;\n    mat4 proj_matrix;\n} frame_uniforms;\n\nlayout(std140, set = 0, binding = 2) uniform ObjectUniforms {\n	mat4 model_matrix;\n    mat4 normal_matrix;\n} object_uniforms;";
const char INPUTS_FS_DATA[] = "layout(location = 4) in highp vec3 vertex_world_position;\n\n#if defined(HAS_ATTRIBUTE_UV0) && !defined(HAS_ATTRIBUTE_UV1)\nlayout(location = 5) in highp vec2 vertex_uv01;\n#elif defined(HAS_ATTRIBUTE_UV1)\nlayout(location = 5) in highp vec4 vertex_uv01;\n#endif";
const char INPUTS_VS_DATA[] = "layout(location = LOCATION_POSITION) in vec3 mesh_position;\n\n#if defined(HAS_ATTRIBUTE_NORMAL)\nlayout(location = LOCATION_NORMAL) in vec3 mesh_normal;\nlayout(location = LOCATION_TANGENT) in vec3 mesh_tangent;\nlayout(location = LOCATION_BITANGENT) in vec3 mesh_bitangent;\n#endif\n\n#if defined(HAS_ATTRIBUTE_COLOR)\nlayout(location = LOCATION_COLOR) in vec4 mesh_color;\n#endif\n\n#if defined(HAS_ATTRIBUTE_UV0)\nlayout(location = LOCATION_UV0) in vec2 mesh_uv0;\n#endif\n\n#if defined(HAS_ATTRIBUTE_UV1)\nlayout(location = LOCATION_UV1) in vec2 mesh_uv1;\n#endif\n\n#if defined(HAS_ATTRIBUTE_BONE_INDICES)\nlayout(location = LOCATION_BONE_INDICES) in uvec4 mesh_bone_indices;\nlayout(location = LOCATION_BONE_WEIGHTS) in vec4 mesh_bone_weights;\n#endif\n\n\n#if defined(HAS_ATTRIBUTE_CUSTOM0)\nlayout(location = LOCATION_CUSTOM0) in vec4 mesh_custom0;\n#endif\n\n#if defined(HAS_ATTRIBUTE_CUSTOM1)\nlayout(location = LOCATION_CUSTOM1) in vec4 mesh_custom1;\n#endif\n\n#if defined(HAS_ATTRIBUTE_CUSTOM2)\nlayout(location = LOCATION_CUSTOM2) in vec4 mesh_custom2;\n#endif\n\n#if defined(HAS_ATTRIBUTE_CUSTOM3)\nlayout(location = LOCATION_CUSTOM3) in vec4 mesh_custom3;\n#endif\n\n#if defined(HAS_ATTRIBUTE_CUSTOM4)\nlayout(location = LOCATION_CUSTOM4) in vec4 mesh_custom4;\n#endif\n\n#if defined(HAS_ATTRIBUTE_CUSTOM5)\nlayout(location = LOCATION_CUSTOM5) in vec4 mesh_custom5;\n#endif\n\nlayout(location = 4) out highp vec3 vertex_world_position;\n\nlayout(location = 6) out highp vec4 vertex_position;\n\n#if defined(HAS_ATTRIBUTE_UV0) && !defined(HAS_ATTRIBUTE_UV1)\nlayout(location = 5) out highp vec2 vertex_uv01;\n#elif defined(HAS_ATTRIBUTE_UV1)\nlayout(location = 5) out highp vec4 vertex_uv01;\n#endif\n";
const char MAIN_FS_DATA[] = "layout(location = 0) out vec4 fragColor;\n\nvoid main() {\n    MaterialFragmentInputs inputs;\n    initMaterialFragment(inputs);\n\n    materialFragment(inputs);\n    // todo\n    fragColor = inputs.base_color;\n}\n";
const char MAIN_VS_DATA[] = "void main() {\n    MaterialVertexInputs material;\n    initMaterialVertex(material);\n\n    materialVertex(material);\n\n#if defined(HAS_ATTRIBUTE_UV0)\n    vertex_uv01.xy = material.uv0;\n#endif\n#if defined(HAS_ATTRIBUTE_UV1)\n    vertex_uv01.zw = material.uv1;\n#endif\n\n    gl_Position = material.world_position;\n}\n";
const char MATERIAL_INPUTS_FS_DATA[] = "struct MaterialFragmentInputs {\n    vec4  base_color;\n};\n\nvoid initMaterialFragment(out MaterialFragmentInputs material) {\n    material.base_color = vec4(1.0);\n}";
const char MATERIAL_INPUTS_VS_DATA[] = "struct MaterialVertexInputs {\n    vec4 world_position;\n\n#ifdef HAS_ATTRIBUTE_NORMAL\n    vec3 world_normal;\n#endif\n\n#ifdef HAS_ATTRIBUTE_COLOR\n    vec4 color;\n#endif\n\n#ifdef HAS_ATTRIBUTE_UV0\n    vec2 uv0;\n#endif\n\n#ifdef HAS_ATTRIBUTE_UV1\n    vec2 uv1;\n#endif\n};\n\nvoid initMaterialVertex(out MaterialVertexInputs material) {\n    material.world_position = frame_uniforms.proj_matrix * frame_uniforms.view_matrix * object_uniforms.model_matrix * vec4(mesh_position, 1.0);\n\n#ifdef HAS_ATTRIBUTE_UV0\n    material.uv0 = mesh_uv0;\n#endif\n\n#ifdef HAS_ATTRIBUTE_UV1\n    material.uv1 = mesh_uv1;\n#endif\n}\n";
const char TEXTURE_FRAG_DATA[] = "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n\nlayout(binding = 1) uniform sampler2D texSampler;\nlayout(location = 0) in vec2 fragTexCoord;\nlayout(location = 0) out vec4 outColor;\n\nvoid main()\n{\n    outColor = texture(texSampler, fragTexCoord);\n}";
const char TEXTURE_VERT_DATA[] = "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n\nlayout(binding = 0) uniform UniformBufferObject {\n    mat4 model;\n    mat4 view;\n    mat4 proj;\n} ubo;\n\nlayout(location = 0) in vec3 inPosition;\nlayout(location = 1) in vec2 inTexCoord;\n\nlayout(location = 0) out vec2 fragTexCoord;\n\nvoid main() \n{\n    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);\n    fragTexCoord = inTexCoord;\n}";
const char TRIANGLE_FRAG_DATA[] = "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n\nlayout(location = 0) out vec4 outColor;\n\nvoid main()\n{\n    outColor = vec4(1.0, 0.0, 0.0, 0.0);\n}";
const char TRIANGLE_VERT_DATA[] = "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n\nlayout(location = 0) in vec3 inPosition;\nlayout(location = 1) in vec3 inNormal;\nlayout(location = 2) in vec2 inTexCoord;\n\nvoid main() \n{\n    gl_Position = vec4(inPosition, 1.0);\n}";
}
