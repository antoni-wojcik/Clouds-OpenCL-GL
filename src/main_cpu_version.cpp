//
//  main.cpp
//  Clouds
//
//  Created by Antoni Wójcik on 21/03/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <random>
#include <cmath>

#define SIZE 200

struct pos{
    int x, y;
    pos() {}
    pos(int xx, int yy) {
        x = xx;
        y = yy;
    }
};

float distance_2(const pos& p1, const pos& p2) {
    return (p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y);
}

void add_layer(float brightness[SIZE][SIZE], std::mt19937& rng, int nodes, float blending, float persistence){
    int square_size = SIZE/nodes;
    
    std::uniform_int_distribution<std::mt19937::result_type> distr(0,square_size-1);
    
    pos v[nodes+2][nodes+2];
    for(int j = 1; j <= nodes; j++) for(int i = 1; i <= nodes; i++) {
        v[i][j].x = distr(rng);
        v[i][j].y = distr(rng);
    }
    for(int i = 0; i < nodes+2; i++) {
        int j = 1+(i-1+nodes)%nodes;
        v[i][0] = v[j][nodes];
        v[i][nodes+1] = v[j][1];
        v[0][i] = v[nodes][j];
        v[nodes+1][i] = v[1][j];
    }
    
    std::cout << "\n";
    
    float sqr_size_inv = 1.0f/(square_size*square_size*2);
    
    for(int j = 0; j < nodes; j++) for(int i = 0; i < nodes; i++) {
        for(int y = 0; y < square_size; y++) for(int x = 0; x < square_size; x++) {
            pos p((i+1)*square_size+x, (j+1)*square_size+y);
            
            float min_d = square_size*square_size*2;
            for(int a = 0; a < 3; a++) for(int b = 0; b < 3; b++) {
                pos v_global = v[i+a][j+b];
                v_global.x += square_size*(i+a);
                v_global.y += square_size*(j+b);
                float dist = distance_2(p, v_global);
                if(dist < min_d) min_d = dist;
            }
            
            if(blending == 1.0f) brightness[p.x-square_size][p.y-square_size] = 1.0f-std::tanh(min_d*sqr_size_inv*persistence);
            else brightness[p.x-square_size][p.y-square_size] = blending*(1.0f-std::tanh(min_d*sqr_size_inv*persistence)) + (1.0f-blending)*brightness[p.x-square_size][p.y-square_size];
        }
    }
}

int main(int argc, const char * argv[]) {
    std::random_device dev;
    std::mt19937 rng(dev()); //random number generator
    
    float brightness[SIZE][SIZE];
    add_layer(brightness, rng, 5, 1.0f, 10.0f);
    add_layer(brightness, rng, 10, 0.4f, 10.0f);
    add_layer(brightness, rng, 20, 0.1f, 10.0f);
    add_layer(brightness, rng, 50, 0.05f, 10.0f);
    add_layer(brightness, rng, 100, 0.04f, 10.0f);
    
    std::ofstream img("sim.ppm");
    img << "P3\n" << SIZE << " " << SIZE << "\n255\n";
    
    for(int j = SIZE-1; j >= 0; j--) for(int i = 0; i < SIZE; i++) {
        int ir = int(255.99f*brightness[i][j]);
        int ig = int(255.99f*brightness[i][j]);
        int ib = int(255.99f*brightness[i][j]);
        
        img << ir << " " << ig << " " << ib << "\n";
    }

    img.close();
    
    system("open sim.ppm");
    return 0;
}
