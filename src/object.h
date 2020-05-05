//
//  object.h
//  Clouds
//
//  Created by Antoni Wójcik on 05/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef object_h
#define object_h

#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "model.h"
#include "shader.h"
#include "camera.h"

class Object {
private:
    Shader shader;
    
    Model model;
    
    glm::mat4 modelMatrix;
    
    glm::vec3 pos;
    glm::vec3 scale;
    
    void updateMMatrix(Camera& camera) {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, pos - camera.transferPos()); // translate it down so it's at the center of the scene
        modelMatrix = glm::scale(modelMatrix, scale);
    }
    
public:
    Object(const char* model_path, const char* obj_vertex_path, const char* obj_fragment_path) : model(model_path), shader(obj_vertex_path, obj_fragment_path) {
        pos = glm::vec3(0.5f, 0.5f, 0.5f);
        scale = glm::vec3(0.2f);
    }
    
    void render(Camera& camera) {
        updateMMatrix(camera);
        glm::mat4 PVMMatrix = camera.transferPVMatrix() * modelMatrix;
        
        shader.use();
        shader.setMat4("M", modelMatrix);
        shader.setMat4("PVM", PVMMatrix);
        shader.setVec3("camera_pos", camera.transferPos());
        
        model.draw(shader);
    }
};

#endif /* object_h */
