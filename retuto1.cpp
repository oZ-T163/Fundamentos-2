// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <time.h>
#include <chrono>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// Include GLEW. Always include it before gl.h and glfw3.h, since it's a bit magic.
#include <GL/glew.h>
#include <GL/gl.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
using namespace glm;

#include "common/shader.hpp"

const unsigned long FOURCC_DXT1 = 0x31545844; //(MAKEFOURCC('D','X','T','1'))
const unsigned long FOURCC_DXT3 = 0x33545844; //(MAKEFOURCC('D','X','T','3'))
const unsigned long FOURCC_DXT5 = 0x35545844; //(MAKEFOURCC('D','X','T','5'))

double timeToShader = 3;
// This will identify our vertex buffer
GLuint vertexbuffer[2];
GLuint programID;
GLuint programID2;
GLuint uvbuffer;
GLuint Texture;

class Triangle
{
    // An array of 3 vectors which represents 3 vertices
public:
    glm::vec3 vertices[3] = {
        vec3(-1, -1, 0),
        vec3(1, -1, 0),
        vec3(0, 1, 0)};
    quat rot = quat(vec3(0,0,0));
    glm::vec3 scale = vec3(1,1,1);
    glm::vec3 pos = vec3(0,0,0);
    void Translate(vec3 movePos){
        pos += movePos*rot;
    }
    void Rotate(vec3 eulerAngles){        
        rot = rot*quat(eulerAngles);
    } 
    void PassToBuffer(GLfloat *vertexB){
        int j = 0;
        for (int i = 0; i < 9; i += 3)
        {
            vec3 vector = vertices[j]*scale*rot+pos;            
            vertexB[i] = vector.x;
            vertexB[i + 1] = vector.y;
            vertexB[i + 2] = vector.z;
            j++;
        }
    }
};


Triangle triangle1, triangle2;

static GLfloat g_vertex_buffer_data1[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f, 1.0f, 0.0f
    };
static GLfloat g_vertex_buffer_data2[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f, 1.0f, 0.0f
    };

static const GLfloat g_color_buffer_data[] = {
    0.583f,  0.771f,  0.014f,
    0.609f,  0.115f,  0.436f,
    0.327f,  0.483f,  0.844f
};

static const GLfloat g_uv_buffer_data[] = {
    0.0f, 1.0f-0.0f,
    1.0f, 1.0f-0.0f,
    1.0f, 1.0f-1.0f
};


/*GLuint loadTGA_glfw(const char * imagepath){

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Read the file, call glTexImage2D with the right parameters
    glfwLoadTexture2D(imagepath, 0);

    // Nice trilinear filtering.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Return the ID of the texture we just created
    return textureID;
}*/
GLuint loadDDS(const char * imagepath){

    unsigned char header[124];

    FILE *fp;

    /* try to open the file */
    fp = fopen(imagepath, "rb");
    if (fp == NULL){
        printf("error 1\n");
        return 0;
    }

    /* verify the type of file */
    char filecode[4];
    fread(filecode, 1, 4, fp);
    if (strncmp(filecode, "DDS ", 4) != 0) {
        fclose(fp);
        printf("error 2\n");
        return 0;
    }

    /* get the surface desc */
    fread(&header, 124, 1, fp); 

    unsigned int height      = *(unsigned int*)&(header[8 ]);
    unsigned int width       = *(unsigned int*)&(header[12]);
    unsigned int linearSize  = *(unsigned int*)&(header[16]);
    unsigned int mipMapCount = *(unsigned int*)&(header[24]);
    unsigned int fourCC      = *(unsigned int*)&(header[80]);
    unsigned char * buffer;
    unsigned int bufsize;
    /* how big is it going to be including all mipmaps? */
    bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
    fread(buffer, 1, bufsize, fp);
    /* close the file pointer */
    fclose(fp);
        unsigned int components  = (fourCC == FOURCC_DXT1) ? 3 : 4;
    unsigned int format;
    switch(fourCC)
    {
    case FOURCC_DXT1:
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;
    case FOURCC_DXT3:
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;
    case FOURCC_DXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    default:
        free(buffer);
        printf("error 3\n");
        return 0;
    }
        // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    glEnable( GL_TEXTURE_2D );
    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    /* load the mipmaps */
    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
    {
        unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 
            0, size, buffer + offset);

        offset += size;
        width  /= 2;
        height /= 2;
    }
    free(buffer); 

    return textureID;
}

void Scene1(double deltaTime)
{
    static float yDirection = 1;
    // Draw triangle...

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_DRAW_BUFFER);
    //glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    //glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glVertexAttribPointer(
        0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset
    );    
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);

    if (g_vertex_buffer_data1[1] > 1 && yDirection > 0)
    {
        yDirection = -1;
        g_vertex_buffer_data1[1] -= (g_vertex_buffer_data1[1] - 1);
    }
    else if (g_vertex_buffer_data1[1] < -1 && yDirection < 0)
    {
        yDirection = 1;
        g_vertex_buffer_data1[1] -= (g_vertex_buffer_data1[1] + 1);
    }
    g_vertex_buffer_data1[1] += deltaTime * yDirection * 2;
}

void Scene2(double deltaTime, GLFWwindow *window)
{
    // Draw triangle...
	bool rotate = true;
    glUseProgram(programID);
    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_DRAW_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);     
    glVertexAttribPointer(
        0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset
    );
    
    // Bind our texture in Texture Unit 0
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, Texture);    
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glVertexAttribPointer(
        1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        2,                                // size : U+V => 2
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );
    // Draw the triangle !
      
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);

    glUseProgram(programID2);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]); 
    glVertexAttribPointer(
        0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset
    );

    // Draw the triangle !
       
    glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle

    glDisableVertexAttribArray(0);



    // Don't forget to #include <glm/gtc/quaternion.hpp> and <glm/gtx/quaternion.hpp>

    //triangle1.Rotate(vec3(0,0,deltaTime));
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        triangle1.Translate(vec3(-deltaTime,0,0));
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        triangle1.Translate(vec3(deltaTime,0,0));
    }

    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS){
        
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
            triangle1.scale += vec3(-deltaTime,0,0);
        }   
        else{
            triangle1.scale += vec3(+deltaTime,0,0);
        }
    }
    if(glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS){
        
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
            triangle1.scale += vec3(0,-deltaTime,0);
        }   
        else{
            triangle1.scale += vec3(0,+deltaTime,0);
        }
    }
    if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS){
        
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
            triangle1.scale += vec3(0,0,-deltaTime);
        }   
        else{
            triangle1.scale += vec3(0,0,+deltaTime);
        }
    }
	if(glfwGetKey (window,GLFW_KEY_R) == GLFW_PRESS)
	{
		
		if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
            triangle1.Rotate(vec3(0,0,deltaTime));
        }   
        else{
            triangle1.Rotate(vec3(0,0,-deltaTime));
        }
	}
	


    triangle1.PassToBuffer(g_vertex_buffer_data1);
    //triangle2.PassToBuffer(g_vertex_buffer_data2);
    
}

int main()
{

    float y = 0;

    // Initialise GLFW
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);               // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Open a window and create its OpenGL context
    GLFWwindow *window; // (In the accompanying source code, this variable is global for simplicity)
    window = glfwCreateWindow(768, 768, "Tutorial 01", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    glewExperimental = true;        // Needed in core profile
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders("MyVertex.shader", "MyFragment.shader");
    programID2 = LoadShaders("MyVertex2.shader", "MyFragment.shader");
    // Use our shader
    glUseProgram(programID);

    GLuint VertexArrayID[2];
    glGenVertexArrays(2, VertexArrayID);
    glBindVertexArray(VertexArrayID[0]);
    glBindVertexArray(VertexArrayID[1]);

    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(2, vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    //glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);    
    // Give our vertices to OpenGL.
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glNamedBufferData(VertexArrayID[0], sizeof(g_vertex_buffer_data1), g_vertex_buffer_data1, GL_STATIC_DRAW);


    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);    
    glNamedBufferData(VertexArrayID[1], sizeof(g_vertex_buffer_data2), g_vertex_buffer_data2, GL_STATIC_DRAW);

    GLuint unit = 0;
    glActiveTexture(GL_TEXTURE0 + unit);
    
    glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), unit);

    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);


    auto t_start = std::chrono::high_resolution_clock::now();
    // the work...
    auto t_end = std::chrono::high_resolution_clock::now();

    double deltaTime = 0;
    triangle2.pos = vec3(0,0,-1.5);

    int k = 0;
    do
    {
        
        if(glfwGetKey(window, GLFW_KEY_0))
        {
            k = 0;
        }
        if(glfwGetKey(window, GLFW_KEY_1))
        {
            k = 1;
        }
        if(glfwGetKey(window, GLFW_KEY_2))
        {
            k = 2;
        }

        switch(k)
        {
            case 0 : Texture = loadDDS("test_textura_PNG_DXT1_1.DDS");
                     break;

            case 1 : Texture = loadDDS("Sven_256.DDS");
                     break;

            case 2 : Texture = loadDDS("Sven_128.DDS");
                     break;
        }
        
        
        //getsTime Dif
        t_start = t_end;
        t_end = std::chrono::high_resolution_clock::now();

        deltaTime = std::chrono::duration<double>(t_end - t_start).count();

        /*timeToShader -= deltaTime;
        if (timeToShader < 0)
        {
            timeToShader = 3;
            GLuint newProgramID = LoadShaders("MyVertex.shader", "MyFragment.shader");
            // Use our shader
            glUseProgram(newProgramID);

            glDeleteProgram(programID);
            programID = newProgramID;
        }*/

 

        // Clear the screen. It's not mentioned before Tutorial 02, but it can cause flickering, so it's there nonetheless.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Draw through function
        Scene2(deltaTime, window);

        
        glNamedBufferData(VertexArrayID[0], sizeof(g_vertex_buffer_data1), g_vertex_buffer_data1, GL_STATIC_DRAW);        
        //glUseProgram(programID2);
        glNamedBufferData(VertexArrayID[1], sizeof(g_vertex_buffer_data2), g_vertex_buffer_data2, GL_STATIC_DRAW);
        
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

            

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);
}