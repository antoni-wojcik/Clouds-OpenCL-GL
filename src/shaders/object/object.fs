#version 410 core
out vec4 frag_color;

in vec3 normal;
in vec2 tex_coords;
in vec3 cam_rel_pos;

uniform sampler2D obj_texture;
uniform vec3 camera_pos;

const vec3 light_col = vec3(144.0f/255.0f, 154.0f/255.0f, 171.0f/255.0f);
const vec3 light_dir = normalize(vec3(0.5f, 1.0f, 0.3f));

const float ambient_strength = 0.2f;
const float specular_strength = 1.0f;

void main() {
    vec3 frag_pos = cam_rel_pos + camera_pos;
    vec3 n_normal = normalize(normal);
    
    float diffuse_strength = max(dot(n_normal, light_dir), 0.0f);
    
    vec3 view_dir = normalize(cam_rel_pos);
    vec3 reflect_dir = reflect(light_dir, n_normal);
    
    float specular_factor = pow(max(dot(view_dir, reflect_dir), 0.0f), 32);
    
    frag_color = vec4(texture(obj_texture, tex_coords).rgb, length(cam_rel_pos));
    
    frag_color.xyz *= ((ambient_strength + diffuse_strength + specular_factor * specular_strength) * light_col);
}
