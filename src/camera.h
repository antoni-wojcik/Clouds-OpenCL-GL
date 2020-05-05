//
//  camera.h
//  GPURaymarching
//
//  Created by Antoni Wójcik on 17/09/2019. 
//

#ifndef camera_h
#define camera_h

#include "glm.hpp"
#include "shader.h"

const float CAMERA_SPEED_SLOW = 0.3f;
const float CAMERA_SPEED_NORMAL = 1.0f;
const float CAMERA_SPEED_FAST = 5.0f;
const float MOUSE_SENSITIVITY = 0.2f;
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float ZOOM_MIN = 90.0f;
const float ZOOM_MAX = 10.0f;
const float ZOOM_SPEED = 0.5f;

enum CameraMovementDirection {
    FORWARD,
    BACK,
    LEFT,
    RIGHT
};

class Camera {
private:
    float fov, aspect;
    float speed;
    float half_height;
    float half_width;
    
    float yaw, pitch;
    
    glm::vec3 u, v, w;
    glm::vec3 up, position;
    glm::vec3 horizontal, vertical, lower_left_corner;
    
    glm::mat4 pvMatrix;
    
    inline void updatePVMatrix() {
        pvMatrix = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f) * glm::lookAt(glm::vec3(0.0f), w, v);
    }
    
    inline void updateVectors() {
        float rad_pitch = glm::radians(pitch), rad_yaw = glm::radians(yaw);
        w.x = glm::cos(rad_pitch) * glm::sin(rad_yaw);
        w.y = glm::sin(rad_pitch);
        w.z = glm::cos(rad_pitch) * glm::cos(rad_yaw);
        w = normalize(w);
        u = normalize(cross(w, up));
        v = cross(u, w);
        
        lower_left_corner = w-(half_width*u + half_height*v);
        horizontal = 2.0f*half_width*u;
        vertical = 2.0f*half_height*v;
        
        updatePVMatrix();
    }
    
    inline void setFov() {
        float angle = fov*M_PI/180.0f;
        half_height = tan(angle*0.5f);
        half_width = aspect * half_height;
        updateVectors();
    }
public:
    Camera(int camera_fov = 60.0f, const glm::vec3& pos = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& up_dir = glm::vec3(0.0f, -1.0f, 0.0f), float camera_aspect = 1.0f) : fov(camera_fov), aspect(camera_aspect), position(pos), up(up_dir), yaw(YAW), pitch(PITCH), speed(CAMERA_SPEED_SLOW) {
        float angle = fov*M_PI/180.0f;
        half_height = tan(angle*0.5f);
        half_width = aspect * half_height;
        updateVectors();
    }
    
    inline void move(CameraMovementDirection dir, float dt) {
        float ds = speed * dt;
        if(dir == FORWARD) position += w * ds;
        else if(dir == BACK) position -= w * ds;
        else if(dir == LEFT) position -= u * ds;
        else if(dir == RIGHT) position += u * ds;
    }
    
    inline void rotate(float x, float y) {
        yaw += x * MOUSE_SENSITIVITY * fov/ZOOM_MAX;
        pitch += y * MOUSE_SENSITIVITY * fov/ZOOM_MAX;
        //constrain the yaw
        if(pitch > 89.0f) pitch = 89.0f;
        else if(pitch < -89.0f) pitch = -89.0f;
        //constranin the pitch
        yaw = fmod(yaw, 360.0f);
        updateVectors();
    }
    
    inline void setFasterSpeed(bool speed_up) {
        if(speed_up) speed = CAMERA_SPEED_FAST;
        else speed = CAMERA_SPEED_NORMAL;
    }
    
    inline void setSlowerSpeed(bool speed_down) {
        if(speed_down) speed = CAMERA_SPEED_SLOW;
        else speed = CAMERA_SPEED_NORMAL;
    }
    
    inline void zoom(float scroll) {
        fov += scroll * ZOOM_SPEED;
        if(fov < ZOOM_MAX) fov = ZOOM_MAX;
        else if(fov > ZOOM_MIN) fov = ZOOM_MIN;
        setFov();
    }
    
    inline void setSize(float new_aspect) {
        aspect = new_aspect;
        float angle = fov*M_PI/180.0f;
        half_height = tan(angle*0.5f);
        half_width = aspect * half_height;
        updateVectors();
    }
    
    inline void transferData(Shader& shader) const {
        shader.setVec3("origin", position);
        shader.setVec3("camera_llc", lower_left_corner);
        shader.setVec3("horizontal", horizontal);
        shader.setVec3("vertical", vertical);
    }
    
    inline glm::mat4 transferPVMatrix() const {
        return pvMatrix;
    }
    
    inline glm::vec3 transferPos() const {
        return position;
    }
};

#endif /* camera_h */
