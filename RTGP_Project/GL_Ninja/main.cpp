/*
Es04a: as Es03c, but textures are used for the diffusive component of materials
- swapping (pressing keys from 1 to 2) between a shader with texturing applied to the Blinn-Phong and GGX illumination models
N.B. 1) The presented shader swapping method is used in OpenGL 3.3 .
        For OpenGL 4.x, shader subroutines can be used : http://www.geeks3d.com/20140701/opengl-4-shader-subroutines-introduction-3d-programming-tutorial/
N.B. 2) we have considered point lights only, the code must be modifies in case of lights of different nature
N.B. 3) there are other methods (more efficient) to pass multiple data to the shaders (search for Uniform Buffer Objects)
N.B. 4) with last versions of OpenGL, using structures like the one cited above, it is possible to pass a dynamic number of lights
N.B. 5) Updated versions, including texturing, of Model (model_v2.h) and Mesh (mesh_v2.h) classes are used.
author: Davide Gadia
Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen
                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

#ifdef _WIN32
    #define __USE_MINGW_ANSI_STDIO 0
#endif
// Std. Includes
#include <list>
#include <time.h>
#include <string>
#include <cstdlib>

#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, and for FPS camera
// in this example, the Model and Mesh classes support texturing
#include <utils/shader.h>
#include <utils/model.h>
#include <utils/physics.h>
#include <utils/scene.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#define N_MODELS 7

GLuint screenWidth = 1280, screenHeight = 720;

void drawIndicatorLine(Shader lineShader);
void calculateCutNDCCoordinates(int i);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void display_framerate(float framerate);
GLint loadTexture(const char* path);

bool keys[1024];

bool stop=false;
bool pressing = false;
bool cut=false;
const GLfloat maxSecPerFrame = 1.0f / 60.0f;

GLboolean wireframe = GL_FALSE;

glm::vec3 lightPositions[] = {
    glm::vec3(5.0f, 10.0f, 10.0f),
    glm::vec3(-5.0f, 10.0f, 10.0f),
    glm::vec3(5.0f, 10.0f, -10.0f),
};

unsigned int VAOCut, VBOCut;
glm::vec3 cutVerticesNDC[] = {glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 0.f, 1.f)};

GLFWwindow* window;

int main()
{
	srand(time(0));
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
    window = glfwCreateWindow(screenWidth, screenHeight, "GL_Ninja", nullptr, nullptr);
    if (!window)
    {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    glViewport(0, 0, screenWidth, screenHeight);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.f, 0.f, 0.f, 1.0f);
		
	Shader lineShader("lineShader.vert", "lineShader.frag");
	
	// Projection matrix: FOV angle, aspect ratio, near and far planes
	glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 15.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.f, 0.f, 7.f), glm::vec3(0.f, 0.f, 6.f), glm::vec3(0.f, 1.f, 0.f));
	array<string, N_MODELS> modelPaths={"../../models/monkey.obj","../../models/circle.obj","../../models/icosphere.obj","../../models/cylinder.obj","../../models/cone.obj","../../models/triangle.obj", "../../models/sphere.obj"};
	int modelIndex=0;
	
	Scene scene=Scene(projection, view);
	
	double startTime = glfwGetTime();
	GLfloat deltaTime;
	double currentTime;
	GLfloat lastFrame;
	glGenVertexArrays(1, &VAOCut);
	glGenBuffers(1, &VBOCut);
	glBindVertexArray(VAOCut);
	glBindBuffer(GL_ARRAY_BUFFER, VBOCut);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cutVerticesNDC), cutVerticesNDC, GL_DYNAMIC_DRAW);		
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);  
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
    string fpsStr="";
	int numFrames=0;
	
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			
		currentTime = glfwGetTime();
		deltaTime = currentTime - lastFrame;
		lastFrame = currentTime;
		
		numFrames++;
		if(currentTime - startTime>=1)
		{
			fpsStr="Fps: "+std::to_string(numFrames);
			cout<<fpsStr;
			cout << string(fpsStr.length(),'\b');
			numFrames=0;
			startTime=glfwGetTime();
		}
		
		
		if(!stop)
			scene.SimulationStep();
			
		if(scene.AllMeshRemoved())
		{
			scene.AddMesh(modelIndex, modelPaths[modelIndex]);
			modelIndex++;
			modelIndex = modelIndex%N_MODELS;
		}
		
		if(pressing)
		{		
			calculateCutNDCCoordinates(1);
			lineShader.Use();
			glBindVertexArray(VAOCut);
			glBindBuffer(GL_ARRAY_BUFFER, VBOCut);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cutVerticesNDC), cutVerticesNDC);	
			glDrawArrays(GL_LINES, 0, 2);
		}
		else if(cut)
		{
			cut=false;
			scene.Cut(cutVerticesNDC[0], cutVerticesNDC[1]);
		}
		
		scene.DrawScene();
		
        glfwSwapBuffers(window);
    }
	
	glDeleteVertexArrays(1, &VAOCut);
    glDeleteBuffers(1, &VBOCut);
	scene.Clear();
	lineShader.Delete();
    glfwTerminate();
    return 0;
}

void display_framerate(float framerate)
{
	
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;
	
	if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		stop=!stop;
		
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

void calculateCutNDCCoordinates(int i)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	float x=(float)xpos;
	float y=(float)ypos;
	cutVerticesNDC[i]=glm::vec3(2.0f*(x/screenWidth) - 1.0f, (-1)*2.0f*(y/screenHeight) + 1.0f, 0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		calculateCutNDCCoordinates(0);
		pressing=true;
	}
	else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE )
	{
		pressing=false;
		cut=true;
	}
}