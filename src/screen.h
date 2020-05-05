//
//  screen.h
//  GPURaymarching
//
//  Created by Antoni Wójcik on 17/09/2019.
//

#ifndef screen_h
#define screen_h

#include <string>
#include <fstream>

#include "object.h"
#include "camera.h"

class Screen {
private:
    short width, height;
    Shader screen_shader;
    unsigned int FBO_scene, FBO_screen;
    
    unsigned int scene_texture;
    GLuint scene_texture_loc;
    unsigned int screen_texture;
    GLuint screen_texture_loc;
    
    float vertices[12];
    unsigned int indices[6];
    unsigned int VBO, VAO, EBO;
    
    void setupSceneFramebuffer(Shader& cloud_shader) {
        glGenFramebuffers(1, &FBO_scene);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_scene);
        
        glGenTextures(1, &scene_texture);
        glBindTexture(GL_TEXTURE_2D, scene_texture);
        scene_texture_loc = glGetUniformLocation(cloud_shader.ID, "sceneTexture");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene_texture, 0);
        
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "ERROR: OpenGL: Failed to create framebuffer" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void setupScreenFramebuffer() {
        glGenFramebuffers(1, &FBO_screen);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_screen);
        
        glGenTextures(1, &screen_texture);
        glBindTexture(GL_TEXTURE_2D, screen_texture);
        screen_texture_loc = glGetUniformLocation(screen_shader.ID, "screenTexture");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_texture, 0);
        
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "ERROR: OpenGL: Failed to create framebuffer" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    inline void bindScene() {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_scene);
        glViewport(0, 0, width, height);
    }
    
    inline void bindScreen() {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO_screen);
        glViewport(0, 0, width, height);
    }
    
    inline void unbind(int scr_width, int scr_height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, scr_width, scr_height);
    }
    
public:
    Screen(const char* screen_vertex_path, const char* screen_fragment_path, Shader& cloud_shader, int buff_width, int buff_height) : vertices {
        1.0f,  1.0f, 0.0f,  // top right
        1.0f, -1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f,  // bottom left
        -1.0f,  1.0f, 0.0f   // top left
    }, indices {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    }, screen_shader(screen_vertex_path, screen_fragment_path), width(buff_width), height(buff_height) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        screen_shader.use();
        
        setupSceneFramebuffer(cloud_shader);
        setupScreenFramebuffer();
    }
    
    ~Screen() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        
        glDeleteFramebuffers(1, &FBO_scene);
        glDeleteFramebuffers(1, &FBO_screen);
    }
    
    inline void clearScene() {
        bindScene();
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    inline void drawObject(Object& obj, Camera& camera) {
        
        glEnable(GL_CULL_FACE);
        bindScene();
        
        obj.render(camera);
        
        glDisable(GL_CULL_FACE);
    }
    
    inline void drawClouds(Shader& shader) {
        bindScreen();
        
        shader.use();
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, scene_texture); // !!!!!!!!!!!!!
        glUniform1i(scene_texture_loc, 1);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    inline void drawScreen(Shader& shader, int scr_width, int scr_height) {
        unbind(scr_width, scr_height);
        screen_shader.use();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screen_texture);
        glUniform1i(screen_texture_loc, 0);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    inline void takeScreenshot(int scr_width, int scr_height, const std::string& name = "screenshot", bool show_image = false) {
        std::cout << "Taking screenshot: " << name << ".tga" << std::endl;
        short TGA_header[] = {0, 2, 0, 0, 0, 0, width, height, 24};
        char* pixel_data = new char[3*width*height]; //there are 3 colors (RGB) for each pixel
        std::ofstream file("screenshots/" + name + ".tga", std::ios::out | std::ios::binary);
        if(!pixel_data || !file) {
            std::cerr << "ERROR: COULD NOT TAKE THE SCREENSHOT" << std::endl;
            exit(-1);
        }
        
        bindScreen();
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixel_data);
        unbind(scr_width, scr_height);
        
        file.write((char*)TGA_header, 9*sizeof(short));
        file.write(pixel_data, 3*width*height);
        file.close();
        delete [] pixel_data;
        
        if(show_image) {
            std::cout << "Opening the screenshot" << std::endl;
            std::system(("open " + name + ".tga").c_str());
        }
    }
    
    /*void resize(int buff_width, int buff_height) {
        width = buff_width;
        height = buff_height;
    }*/
};

#endif /* screen_h */
