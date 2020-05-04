#version 410 core

#define SAMPLE_SEP 0.003f
#define SAMPLE_SEP_BLANK 0.02f

#define SIZE 1.0f
#define MAIN_RAY_ABSORBTION 200.0f
#define BRIGHTNESS_AMPLIFY 190.0f

in vec2 fragPos;
out vec4 fragColor;

uniform vec3 origin;
uniform vec3 camera_llc; // camera's lower left corner position
uniform vec3 horizontal;
uniform vec3 vertical;

uniform sampler3D sam;
uniform float time;

const vec3 box_origin = vec3(0.0f, 0.0f, 0.0f);
const vec3 box_end = vec3(2*SIZE, SIZE, 2*SIZE);
const float SIZE_INV = 1.0f / SIZE;

const vec3 light_dir = normalize(vec3(0.5f, 1.0f, 0.01f));
const vec3 light_col = vec3(144.0f/255.0f, 154.0f/255.0f, 171.0f/255.0f);
const vec3 no_light_col = vec3(71.0f/255.0f, 73.0f/255.0f, 77.0f/255.0f); //vec3(0.514f, 0.392f, 0.494f);//vec3(0.933f, 0.663f, 0.604f);

const vec3 bottom_col = vec3(34.0f/255.0f, 41.0f/255.0f, 46.0f/255.0f);
const vec3 top_col = vec3(60.0f/255.0f, 69.0f/255.0f, 77.0f/255.0f);
const vec3 moon_col = vec3(203.0f/255.0f, 214.0f/255.0f, 234.0f/255.0f) * 1.5f;

const vec3 velocity = vec3(0.05f, 0.0f, 0.02f);


struct Ray {
    vec3 start; //starting location
    vec3 dir; //has to be normalized
    vec3 dir_inv;
    float param; //has to be set to 0 when initalized
};


vec3 currentRayPoint(in Ray r) {
    return r.start + r.dir*r.param;
}

Ray genInitialRay(in vec3 start, float s, float t) {
    Ray r;
    r.start = start;
    r.dir = normalize(camera_llc + s*horizontal + t*vertical);
    r.dir_inv = vec3(1.0f) / r.dir;
    r.param = 0.0f;
    return r;
}

float distInBox(inout Ray r) {
    vec3 t1 = (box_origin - r.start) * r.dir_inv;
    vec3 t2 = (box_end - r.start) * r.dir_inv;
    vec3 t_min = min(t1, t2);
    vec3 t_max = max(t1, t2);
    
    float param1 = max(max(t_min.x, t_min.y), t_min.z);
    float param2 = min(t_max.x, min(t_max.y, t_max.z));
    
    float dist_to_box = max(0.0f, param1);
    float dist_inside_box = max(0.0f, param2 - dist_to_box);
    
    r.param = dist_to_box;
    return dist_inside_box;
}

float sampleDensity(float density, float sub_dist) {
    return density * sub_dist * SIZE_INV;
}

vec3 calculateBackground(in vec3 dir) {
    float angle = 0.5f+0.5f*dot(dir, -light_dir);
    
    if(angle > 0.001f) {
        vec3 color = mix(top_col, bottom_col, angle*angle);
        
        float halo = exp(-angle*angle*300000.0f);
        
        color = mix(color, moon_col, halo);
        
        return color;
    } else return moon_col;
}

void main() {
    Ray r_main = genInitialRay(origin, fragPos.x, fragPos.y);
    
    float dist_in_box = distInBox(r_main);
    
    vec3 final_col = vec3(0.0f);

    float transmittance = 1.0f;
    
    vec3 background_color = calculateBackground(r_main.dir);
    
    if(dist_in_box > 0.0f) {
        vec3 normal = normalize(cross(vertical, horizontal));
        float inv_cos_angle = 1.0f / dot(r_main.dir, normal);
        float sub_dist = SAMPLE_SEP * inv_cos_angle;
        float sub_dist_blank = SAMPLE_SEP_BLANK * inv_cos_angle;
            
        float dist = 0.0f;
        float brightness = 0.0f;
        
        float r_param_max = r_main.param + dist_in_box;
        
        while(dist <= dist_in_box) {
            vec3 sample_point = currentRayPoint(r_main) * SIZE_INV + velocity * time;
            vec2 data = texture(sam, sample_point).rg;
            float data_point = data.x;
            
            float dens_step = sampleDensity(data_point, sub_dist);
            
            if(dens_step > 0.0f) {
                float light_transmittance = data.y;
                
                brightness += dens_step * light_transmittance * transmittance;
                transmittance *= exp(-dens_step * MAIN_RAY_ABSORBTION);
                
                if(transmittance <= 0.01f) break;
                
                r_main.param += sub_dist;
                dist += sub_dist;
            } else {
                r_main.param += sub_dist_blank;
                dist += sub_dist_blank;
            }
        }
        
        //sub_dist = (dist_in_box - dist);
        r_main.param = r_param_max;
        vec3 sample_point = currentRayPoint(r_main) * SIZE_INV + velocity * time;
        
        vec2 data = texture(sam, sample_point).rg;
        float data_point = data.x;
        
        float dens_step = sampleDensity(data_point, sub_dist);
        
        float light_transmittance = data.y;
        brightness += dens_step * light_transmittance * transmittance;
        transmittance *= exp(-dens_step * MAIN_RAY_ABSORBTION);
        
        
        final_col = no_light_col + light_col * brightness * BRIGHTNESS_AMPLIFY;
        
        final_col = mix(final_col, background_color, transmittance);
        
        fragColor = vec4(final_col, 1.0f);
    }
    
    final_col = mix(final_col, background_color, transmittance);
    
    fragColor = vec4(final_col, 1.0f);
    
    gl_FragDepth = 0.0f;
}
