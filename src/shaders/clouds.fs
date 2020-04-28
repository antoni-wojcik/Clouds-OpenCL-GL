#version 410 core

#define SAMPLES 30
#define LIGHT_SAMPLES 5
#define SIZE 20.0f

in vec2 fragPos;
out vec4 fragColor;

uniform vec3 origin;
uniform vec3 camera_llc; // camera's lower left corner position
uniform vec3 horizontal;
uniform vec3 vertical;

uniform sampler3D sam;
uniform float time;

vec3 sides[3];


//using Ray structs will allow for shadows and reflections
struct Ray {
    vec3 start; //starting location
    vec3 dir; //has to be normalized
    //float param; //has to be set to 0 when initalized
    //float previous_distance; //the distance the origin of the ray has travelled from the camera (includes reflections), used in the fog calculations
};

/*vec3 current_ray_point(in Ray r) {
    return r.start + r.dir*r.param;
}*/

Ray gen_initial_ray(in vec3 start, float s, float t) {
    Ray r;
    r.start = start;
    r.dir = normalize(camera_llc + s*horizontal + t*vertical);
    //r.param = 0;
    //r.previous_distance = 0;
    return r;
}

bool sideHit (in Ray r, in vec3 plane_o, in vec3 plane_side_a, in vec3 plane_side_b, out vec3 result) {
    vec3 normal = cross(plane_side_a, plane_side_b);
    float d_n = dot(r.dir, normal);
    if(d_n == 0) return false;
    float param = dot(plane_o - r.start, normal)/d_n;
    if(param < 0) return false;
    vec3 intersection = r.start + r.dir*param;
    
    //coordinates on the surface * lenght of the sides: a = a_coord * a_side_length
    
    float a = dot(intersection - plane_o, plane_side_a);
    float b = dot(intersection - plane_o, plane_side_b);
    
    //normalize them to [0, 1] (if inside the cube) and check if inside [0, 1]
    
    if(0 <= a && a <= dot(plane_side_a, plane_side_a) && 0 <= b && b <= dot(plane_side_b, plane_side_b)) {
        result = intersection;
        return true;
    } else return false;
}

int testSides(inout Ray r, out vec3 hit1, out vec3 hit2) {
    vec3 plane_o = vec3(0);
    
    vec3 hit_point;
    bool hit = false;
    int hit_count = 0;
    
    for(int j = 0; j < 3; j++) {
        hit = sideHit(r, plane_o, sides[j], sides[(j+1)%3], hit_point);
        if(hit) {
            hit_count += 1;
            if(hit_count == 1) hit1 = hit_point;
            else {
                hit2 = hit_point;
                return hit_count;
            }
            hit = false;
        }
    }
    
    plane_o += sides[0] + sides[1] + sides[2];
    
    for(int j = 0; j < 3; j++) {
        hit = sideHit(r, plane_o, -sides[j], -sides[(j+1)%3], hit_point);
        if(hit) {
            hit_count += 1;
            if(hit_count == 1) hit1 = hit_point;
            else {
                hit2 = hit_point;
                return hit_count;
            }
            hit = false;
        }
    }
    
    return hit_count;
}

vec3 getPoint(in vec3 start, in vec3 dir, float param) {
    return start + dir*param;
}

void main() {
    sides[0] = vec3(SIZE, 0, 0);
    sides[1] = vec3(0, SIZE, 0);
    sides[2] = vec3(0, 0, SIZE);
    
    vec3 hit1;
    vec3 hit2;
    float hit_distance;
    float sub_distance;
    Ray r = gen_initial_ray(origin, fragPos.x, fragPos.y);
    
    int hit_count = testSides(r, hit1, hit2);
    if(hit_count == 2) {
        vec3 h1 = hit1 - r.start;
        vec3 h2 = hit2 - r.start;
        hit1 /= SIZE;
        hit2 /= SIZE;
        hit_distance = distance(hit1, hit2);
        if(dot(h1, h1) > dot(h2, h2)) hit1 = hit2;
    } else if(hit_count == 1) {
        hit2 = origin/SIZE;
        hit_distance = distance(hit1/SIZE, hit2);
        hit1 = hit2;
    } else return;
    //fragColor = vec4(texture(sam, vec3(fragPos, time)).rrr, 1);
    
    sub_distance = hit_distance/SAMPLES;
    
    vec3 screen_normal = normalize(cross(horizontal, vertical));
    
    vec4 dens = vec4(0); // integrated density in all the channels
    float strength = 5;
    
    for(int i = 0; i < SAMPLES; i++) {
        vec3 sampling_point = getPoint(hit1, r.dir, float(i) * sub_distance);
        vec4 density_step = texture(sam, sampling_point) * sub_distance;
        dens += density_step;
    }
    
    float brightness = 0.0f;
    //if(dens.r > 0.1f && dens.g > 0.1f && dens.b > 0.1f && dens.a > 0.1f) {
        //brightness += dens.r*dens.g*dens.b*dens.a;
    //}
    
    brightness = dot(dens, dens);
    
    
    float transmittance = 1 - exp(-brightness * strength);
    
    fragColor = vec4(1, 1, 1, transmittance);
    
    //fragColor = vec4(1, 1, 1, hit_distance);
    
    //fragColor = vec4(texture(sam, getPoint(hit1, r.dir, 0)).rgb,1);
    
    //fragColor = vec4(r.dir, 1.0f);
}
