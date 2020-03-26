#version 410 core

#define SAMPLES 50
#define SAMPLE_SEP 0.1f
#define LIGHT_SAMPLES 10
#define SIZE 1.0f
#define LIGHT_ABSORBTION 10.0f
#define MAIN_RAY_ABSORBTION 200.0f
#define BRIGHTNESS_AMPLIFY 200.0f

in vec2 fragPos;
out vec4 FragColor;

uniform vec3 origin;
uniform vec3 camera_llc; // camera's lower left corner position
uniform vec3 horizontal;
uniform vec3 vertical;

uniform sampler3D sam;
uniform float time;

const vec3 box_origin = vec3(0, 0, 0);
const vec3 box_end = vec3(SIZE*10, SIZE, SIZE*10);
const float SIZE_INV = 1.0f/SIZE;

const vec3 light_dir = normalize(vec3(0.01, 1, 0.01));
const vec3 light_col = vec3(1.0f, 1.0f, 1.0f);

//using Ray structs will allow for shadows and reflections
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
    r.dir_inv = vec3(1)/r.dir;
    r.param = 0;
    return r;
}

Ray genLightRay(in vec3 start) {
    Ray r;
    r.start = start;
    r.dir = light_dir;
    r.dir_inv = vec3(1)/r.dir;
    r.param = 0;
    return r;
}

float distInBox(inout Ray r) {
    vec3 t1 = (box_origin - r.start) * r.dir_inv;
    vec3 t2 = (box_end - r.start) * r.dir_inv;
    vec3 t_min = min(t1, t2);
    vec3 t_max = max(t1, t2);
    
    float param1 = max(max(t_min.x, t_min.y), t_min.z);
    float param2 = min(t_max.x, min(t_max.y, t_max.z));
    
    float dist_to_box = max(0, param1);
    float dist_inside_box = max(0, param2 - dist_to_box);
    
    r.param = dist_to_box;
    return dist_inside_box;
}

float sampleDensity(in vec4 data_point, float sub_dist) {
    //return sub_dist;
    float dens = 0;
    if(data_point.x > 0.35f) {
        dens = data_point.x;
    }
    return dens * sub_dist * SIZE_INV;
}

float calcLight(in vec3 start) {
    //return 1;
    Ray r_light = genLightRay(start);
    
    float dist_edge = distInBox(r_light);
    
    float dens_tot = 0;
    
    float sub_dist = dist_edge/LIGHT_SAMPLES;
    
    for(int i = 0; i < LIGHT_SAMPLES; i++) {
        vec4 data_point = texture(sam, currentRayPoint(r_light) * SIZE_INV);
        dens_tot += sampleDensity(data_point, sub_dist);
        
        r_light.param += sub_dist;
    }
    
    return exp(-dens_tot * LIGHT_ABSORBTION);
}

void main() {
    Ray r_main = genInitialRay(origin, fragPos.x, fragPos.y);
    
    float dist_in_box = distInBox(r_main);
    
    if(dist_in_box > 0) {
        float sub_dist = dist_in_box/SAMPLES;
        
        float transmittance = 1;
        float brightness = 0;
        
        for(int i = 0; i < SAMPLES; i++) {
            vec3 sample_point = currentRayPoint(r_main) * SIZE_INV;
            vec4 data_point = texture(sam, sample_point);
            
            float dens_step = sampleDensity(data_point, sub_dist);
            
            if(dens_step > 0) {
                float light_transmittance = calcLight(sample_point);
                
                brightness += dens_step * light_transmittance * transmittance;
                
                transmittance *= exp(-dens_step * MAIN_RAY_ABSORBTION);
                
                if(transmittance <= 0.01f) break;

            //    r_main.param += sub_dist;
            //} else {
            //    r_main.param += sub_dist * SAMPLES * 0.1;
            }
            r_main.param += sub_dist;
        }
        
        vec3 final_col = light_col * brightness * BRIGHTNESS_AMPLIFY;
        
        FragColor = vec4(final_col, 1.0f - transmittance);
    }
}