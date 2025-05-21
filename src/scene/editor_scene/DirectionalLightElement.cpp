#include "DirectionalLightElement.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, EditorScene::ElementRef parent) {
    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        "New Directional Light",
        glm::vec3{0.0f, 2.0f, 0.0f},     // Default position above scene
        glm::vec3{0.0f, -1.0f, 0.0f},    // Default direction pointing down
        DirectionalLight::create(
            glm::vec3{0.0f, 2.0f, 0.0f},
            glm::vec3{0.0f, -1.0f, 0.0f},
            glm::vec3{1.0f}               // Default white color
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("cone.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{},              // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        )
    );

    light_element->update_instance_data();
    return light_element;
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::from_json(const SceneContext& scene_context, EditorScene::ElementRef parent, const json& j) {
    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        j["name"],
        j["position"],
        j["direction"],
        DirectionalLight::create(
            j["position"],
            j["direction"],
            j["colour"]
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("cone.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{},              // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4(
                        j["colour"][0].get<float>(),
                        j["colour"][1].get<float>(),
                        j["colour"][2].get<float>(),
                        1.0f
                    )
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        )
    );
    
    light_element->visible = j["visible"];
    light_element->visual_scale = j["visual_scale"];
    light_element->update_instance_data();
    
    return light_element;
}

json EditorScene::DirectionalLightElement::into_json() const {
    return {
        {"name",         name},
        {"type",         ELEMENT_TYPE_NAME},
        {"position",     position},
        {"direction",    direction},
        {"colour",       light->colour},
        {"visible",      visible},
        {"visual_scale", visual_scale},
    };
}

void EditorScene::DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    bool transformUpdated = false;

    // Sun Position Controls Section
    if (ImGui::CollapsingHeader("Sun Position", ImGuiTreeNodeFlags_DefaultOpen)) {
        // CHANGE: Subtract 90° to make overhead = 0°
        static float elevation = glm::degrees(asin(-direction.y)) - 90.0f;
        static float azimuth = glm::degrees(atan2(direction.x, direction.z));
        
        // More intuitive angle-based controls
        bool angles_changed = false;
        // CHANGE: Adjust slider range to center around 0°
        angles_changed |= ImGui::SliderFloat("Elevation", &elevation, -90.0f, 90.0f, "%.1f°");
        ImGui::DragDisableCursor(scene_context.window);
        angles_changed |= ImGui::SliderFloat("Azimuth", &azimuth, -180.0f, 180.0f, "%.1f°");
        ImGui::DragDisableCursor(scene_context.window);
        
        // Convert angles back to direction vector if changed
        if (angles_changed) {
            // CHANGE: Add 90° to elevation before conversion
            float adjusted_elevation_rad = glm::radians(elevation - 90.0f);
            float azimuth_rad = glm::radians(azimuth);
            
            // Convert spherical to cartesian coordinates
            direction.y = -sin(adjusted_elevation_rad);  // Negative because up is -Y
            float cos_elev = cos(adjusted_elevation_rad);
            direction.x = cos_elev * sin(azimuth_rad);
            direction.z = cos_elev * cos(azimuth_rad);
            
            direction = glm::normalize(direction);
            transformUpdated = true;
        }
    
        // Allow direct vector input for advanced users
        if (ImGui::CollapsingHeader("Advanced Direction Vector")) {
            glm::vec3 temp_direction = direction;
            if (ImGui::DragFloat3("Raw Direction", &temp_direction[0], 0.01f)) {
                if (glm::length(temp_direction) > 0.001f) {
                    direction = glm::normalize(temp_direction);
                    
                    // ADJUSTED: Update the angles to match
                    elevation = glm::degrees(asin(-direction.y)) - 90.0f;
                    azimuth = glm::degrees(atan2(direction.x, direction.z));
                    
                    transformUpdated = true;
                }
            }
            ImGui::DragDisableCursor(scene_context.window);
        }
    }

    ImGui::Spacing();
    ImGui::Text("Light Properties");
    transformUpdated |= ImGui::ColorEdit3("Colour", &light->colour[0]);
    ImGui::Spacing();

    ImGui::Text("Visuals");
    transformUpdated |= ImGui::Checkbox("Show Visuals", &visible);
    transformUpdated |= ImGui::DragFloat("Visual Scale", &visual_scale, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    if (transformUpdated) {
        update_instance_data();
    }
}

void EditorScene::DirectionalLightElement::update_instance_data() {
    // CHANGE: Calculate a position for the visual that's always OPPOSITE to the light direction
    // This will place the cone "above" the scene when the light points downward
    float visual_distance = 10.0f; // Distance from origin to place the visual
    glm::vec3 visual_position = position - direction * visual_distance;
    
    transform = glm::translate(visual_position);
    
    if (!EditorScene::is_null(parent)) {
        transform = (*parent)->transform * transform;
    }
    
    // Update light data - keep the original light direction and properties
    light->position = glm::vec3(position); // Original position for the light data
    light->direction = glm::normalize(direction);
    
    // Rest of your code for rotation remains unchanged
    if (visible) {
        // Start with identity matrix
        glm::mat4 model = glm::mat4(1.0f);
        
        // Add the scaling
        model = glm::scale(model, glm::vec3(visual_scale));
        model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Create rotation that aligns cone with direction
        glm::vec3 default_dir = glm::vec3(0.0f, -1.0f, 0.0f);
        
        if (glm::abs(glm::dot(default_dir, direction)) > 0.999f) {
            if (direction.y > 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }
        } else {
            glm::vec3 axis = glm::normalize(glm::cross(default_dir, direction));
            float angle = glm::acos(glm::clamp(glm::dot(default_dir, direction), -1.0f, 1.0f));
            model = glm::rotate(model, angle, axis);
        }
        
        // Add translation last
        model = transform * model;
        
        light_cone->instance_data.model_matrix = model;
        light_cone->instance_data.material.emission_tint = glm::vec4(light->colour, 1.0f);
    } else {
        light_cone->instance_data.model_matrix = glm::scale(glm::mat4(1.0f), 
                                                 glm::vec3(std::numeric_limits<float>::infinity()));
    }
}

const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}