/*
The main function of GL_Ninja in addition to creating the window and managing user's input, it creates and use
the scene object methods(inside the draw loop).
*/

#ifdef _WIN32
    #define __USE_MINGW_ANSI_STDIO 0
#endif
#include <list>
#include <time.h>
#include <string>
#include <cstdlib>
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif
#include <glad/glad.h>
#include <glfw/glfw3.h>
#ifdef _WINDOWS_
    #error windows.h was included!
#endif
#include <utils/shader.h>
#include <utils/model.h>
#include <utils/physics.h>
#include <utils/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#define N_MODELS 14

GLuint screenWidth = 1280, screenHeight = 720;
void drawIndicatorLine(Shader lineShader);
void calculateCutNDCCoordinates(int i);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
bool stop=false;
bool pressing = false;
bool cut=false;
GLboolean wireframe = GL_FALSE;
unsigned int VAOCut, VBOCut;
bool keys[1024];
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

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    glViewport(0, 0, screenWidth, screenHeight);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.f, 0.f, 0.f, 1.0f);
		
	Shader lineShader("lineShader.vert", "lineShader.frag");
	
	glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 15.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.f, 0.f, 7.f), glm::vec3(0.f, 0.f, 6.f), glm::vec3(0.f, 1.f, 0.f));
	//These are the models cyclically loaded in the application.
	array<string, N_MODELS> modelPaths={"../../models/cube.obj",
										"../../models/rook.obj",
										"../../models/pedestal.obj",
										"../../models/horse.obj",
										"../../models/icosphere.obj",
										"../../models/bishop.obj",
										"../../models/cylinder.obj",
										"../../models/pawn.obj",
										"../../models/cone.obj",
										"../../models/barrel.obj",
										"../../models/king.obj",
										"../../models/sphere.obj",
										"../../models/queen.obj",
										"../../models/monkey.obj"};
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
			scene.AddMesh(modelPaths[modelIndex]);
			modelIndex++;
			modelIndex = modelIndex%N_MODELS;
		}
		
		//By pressing the right mouse button, the line shader draws a line segment between two points in ndc space.
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
			//By releasing the right mouse button, the application tries to perform a cut, passing to the cut method
			//the cut points that define the cut segment.
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;
	
	if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		stop=!stop;
		
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

//The beginning and ending positions of the cut segment are store inside the cutVertices array;
//the first array slot is the starting point, while the second the ending point.
void calculateCutNDCCoordinates(int i)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	float x=(float)xpos;
	float y=(float)ypos;
	cutVerticesNDC[i]=glm::vec3(2.0f*(x/screenWidth) - 1.0f, (-1)*2.0f*(y/screenHeight) + 1.0f, 0);
}

//This callback function handles mouse inputs (right mouse button pressing and release).
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		calculateCutNDCCoordinates(0);
		pressing=true;
	}
	else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		pressing=false;
		cut=true;
	}
}