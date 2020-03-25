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

class Screen {
private:
    float vertices[12];
    unsigned int indices[6];
    unsigned int VBO, VAO, EBO;
public:
    Screen() : vertices {
        1.0f,  1.0f, 0.0f,  // top right
        1.0f, -1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f,  // bottom left
        -1.0f,  1.0f, 0.0f   // top left
    }, indices {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    } {
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
    }
    ~Screen() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    inline void draw(Shader& shader) const {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        shader.use();
        glBindVertexArray(VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
};

#endif /* screen_h */
