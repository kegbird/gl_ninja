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
#include <string>

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
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
#include <utils/shader_v1.h>
#include <utils/model_v2.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include<btBulletDynamicsCommon.h>

// number of lights in the scene
#define NR_LIGHTS 3

// dimensions of application's window
GLuint screenWidth = 1280, screenHeight = 720;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// setup of Shader Programs for the shaders used in the application
void SetupShaders();
// delete Shader Programs whan application ends
void DeleteShaders();
// print on console the name of current shader
void PrintCurrentShader(int shader);

// load image from disk and create an OpenGL texture
GLint LoadTexture(const char* path);

//Setup Bullet Physics
void SetUpPhysics();
void DeletePhysics();

// we initialize an array of booleans for each keybord key
bool keys[1024];

// we set the initial position of mouse cursor in the application window
GLfloat lastX = 800, lastY = 600;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// enum data structure to manage indices for shaders swapping
enum available_ShaderPrograms{ BLINN_PHONG_TEX_ML, GGX_TEX_ML};
// strings with shaders names to print the name of the current one on console
const char * print_available_ShaderPrograms[] = { "BLINN-PHONG-TEX-ML", "GGX-TEX-ML"};

// index of the current shader (= 0 in the beginning)
GLuint current_program = BLINN_PHONG_TEX_ML;
// a vector for all the Shader Programs used and swapped in the application
vector<Shader> shaders;

// Uniforms to be passed to shaders
// pointlights positions
glm::vec3 lightPositions[] = {
    glm::vec3(5.0f, 10.0f, 10.0f),
    glm::vec3(-5.0f, 10.0f, 10.0f),
    glm::vec3(5.0f, 10.0f, -10.0f),
};

// specular and ambient components
GLfloat specularColor[] = {1.0,1.0,1.0};
GLfloat ambientColor[] = {0.1,0.1,0.1};
// weights for the diffusive, specular and ambient components
GLfloat Kd = 0.8f;
GLfloat Ks = 0.5f;
GLfloat Ka = 0.1f;
// shininess coefficient for Blinn-Phong shader
GLfloat shininess = 25.0f;

// attenuation parameters for Blinn-Phong shader
GLfloat constant = 1.0f;
GLfloat linear = 0.02f;
GLfloat quadratic = 0.001f;

// roughness index for Cook-Torrance shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// vector for the textures IDs
vector<GLint> textureID;

// UV repetitions
GLfloat repeat = 1.0;

//Bullet variables
btDefaultCollisionConfiguration* collisionConfiguration;

btCollisionDispatcher* dispatcher;

btBroadphaseInterface * overlappingPairCache;

btSequentialImpulseConstraintSolver* solver;

btDiscreteDynamicsWorld* dynamicsWorld;

btAlignedObjectArray<btCollisionShape*> collisionShapes;

btCollisionShape* groundShape;

btCollisionShape* cubeShape;

/////////////////// MAIN function ///////////////////////
int main()
{
	SetUpPhysics();
  // Initialization of OpenGL context using GLFW
  glfwInit();
  // We set OpenGL specifications required for this application
  // In this case: 3.3 Core
  // It is possible to raise the values, in order to use functionalities of OpenGL 4.x
  // If not supported by your graphics HW, the context will not be created and the application will close
  // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
  // in relation also to the values indicated in these GLFW commands
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // we set if the window is resizable
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  // we create the application's window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "GL_Ninja", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // we create the Shader Programs used in the application
    SetupShaders();

    // we create the Shader Program used for the plane (fixed)
    Shader plane_shader("18_phong_tex_multiplelights.vert", "19a_blinnphong_tex_multiplelights.frag");

    // we load the model(s) (code of Model class is in include/utils/model_v2.h)
    Model cubeModel("../../models/cube.obj");
    Model planeModel("../../models/plane.obj");

    // we load the images and store them in a vector
    textureID.push_back(LoadTexture("../../textures/UV_Grid_Sm.png"));
    textureID.push_back(LoadTexture("../../textures/SoilCracked.png"));

    // we print on console the name of the first shader used
    PrintCurrentShader(current_program);

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // Check is an I/O event is happening
        glfwPollEvents();
        // View matrix (=camera): position, view direction, camera "up" vector
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 7.0f), glm::vec3(0.0f, 1.0f, 7.0f) + glm::vec3(0,0,-1), glm::vec3(0,1,0));

        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        /////////////////// PLANE ////////////////////////////////////////////////
        // We render a plane under the objects. We apply the fullcolor shader to the plane, and we do not apply the rotation applied to the other objects.
        plane_shader.Use();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureID[1]);

        // we pass projection and view matrices to the Shader Program of the plane
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint kdLocation = glGetUniformLocation(plane_shader.Program, "Kd");
        GLint textureLocation = glGetUniformLocation(plane_shader.Program, "tex");
        GLint repeatLocation = glGetUniformLocation(plane_shader.Program, "repeat");
        GLint matAmbientLocation = glGetUniformLocation(plane_shader.Program, "ambientColor");
        GLint matSpecularLocation = glGetUniformLocation(plane_shader.Program, "specularColor");
        GLint kaLocation = glGetUniformLocation(plane_shader.Program, "Ka");
        GLint ksLocation = glGetUniformLocation(plane_shader.Program, "Ks");
        GLint shineLocation = glGetUniformLocation(plane_shader.Program, "shininess");
        GLint constantLocation = glGetUniformLocation(plane_shader.Program, "constant");
        GLint linearLocation = glGetUniformLocation(plane_shader.Program, "linear");
        GLint quadraticLocation = glGetUniformLocation(plane_shader.Program, "quadratic");

        // we pass each light position to the shader
        for (GLuint i = 0; i < NR_LIGHTS; i++)
        {
            string number = to_string(i);
            glUniform3fv(glGetUniformLocation(plane_shader.Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
        }

        // we assign the value to the uniform variables
        glUniform1f(kdLocation, Kd);
        glUniform1i(textureLocation, 1);
        glUniform1f(repeatLocation, 80.0);
        glUniform3fv(matAmbientLocation, 1, ambientColor);
        glUniform3fv(matSpecularLocation, 1, specularColor);
        glUniform1f(kaLocation, Ka);
        glUniform1f(ksLocation, 0.0f);
        glUniform1f(shineLocation, 1.0f);
        glUniform1f(constantLocation, constant);
        glUniform1f(linearLocation, linear);
        glUniform1f(quadraticLocation, quadratic);

        // we create the transformation matrix by defining the Euler's matrices, and the matrix for normals transformation
        glm::mat4 planeModelMatrix;
        glm::mat3 planeNormalMatrix;
				
        planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -2.0f, 0.0f));
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 1.0f, 10.0f));
        planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(plane_shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(plane_shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));

        // we render the plane
        planeModel.Draw(plane_shader);

        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the selected Shader Program as part of the current rendering process
        shaders[current_program].Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID[0]);

        // we determine the position in the Shader Program of the uniform variable
        kdLocation = glGetUniformLocation(shaders[current_program].Program, "Kd");
        textureLocation = glGetUniformLocation(shaders[current_program].Program, "tex");
        repeatLocation = glGetUniformLocation(shaders[current_program].Program, "repeat");

        glUniform1f(kdLocation, Kd);
        glUniform1i(textureLocation, 0);
        glUniform1f(repeatLocation, repeat);

        // we pass each light position to the shader
        for (GLuint i = 0; i < NR_LIGHTS; i++)
        {
            string number = to_string(i);
            glUniform3fv(glGetUniformLocation(shaders[current_program].Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
        }

        // the other uniforms are passed only to the corresponding shader
        if (current_program == BLINN_PHONG_TEX_ML)
        {
            // we determine the position in the Shader Program of the uniform variable
            GLint matAmbientLocation = glGetUniformLocation(shaders[current_program].Program, "ambientColor");
            GLint matSpecularLocation = glGetUniformLocation(shaders[current_program].Program, "specularColor");
            GLint kaLocation = glGetUniformLocation(shaders[current_program].Program, "Ka");
            GLint ksLocation = glGetUniformLocation(shaders[current_program].Program, "Ks");
            GLint shineLocation = glGetUniformLocation(shaders[current_program].Program, "shininess");
            GLint constantLocation = glGetUniformLocation(shaders[current_program].Program, "constant");
            GLint linearLocation = glGetUniformLocation(shaders[current_program].Program, "linear");
            GLint quadraticLocation = glGetUniformLocation(shaders[current_program].Program, "quadratic");

            // we assign the value to the uniform variable
            glUniform3fv(matAmbientLocation, 1, ambientColor);
            glUniform3fv(matSpecularLocation, 1, specularColor);
            glUniform1f(kaLocation, Ka);
            glUniform1f(ksLocation, Ks);
            glUniform1f(shineLocation, shininess);
            glUniform1f(constantLocation, constant);
            glUniform1f(linearLocation, linear);
            glUniform1f(quadraticLocation, quadratic);

        }
        if (current_program == GGX_TEX_ML)
        {
            // we determine the position in the Shader Program of the uniform variable
            GLint alphaLocation = glGetUniformLocation(shaders[current_program].Program, "alpha");
            GLint f0Location = glGetUniformLocation(shaders[current_program].Program, "F0");
            // we assign the value to the uniform variable
            glUniform1f(alphaLocation, alpha);
            glUniform1f(f0Location, F0);
        }

        // we pass projection and view matrices to the Shader Program
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));


				dynamicsWorld->stepSimulation(1.f/60.f,10);
				
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[0];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform trans;
				if (body && body->getMotionState())
				{
					body->getMotionState()->getWorldTransform(trans);
				}
				btScalar* array;
				trans.getOpenGLMatrix(array);
        //CUBE
        // we create the transformation matrix by defining the Euler's matrices, and the normals transformation matrix
        glm::mat4 cubeModelMatrix;
        glm::mat3 cubeNormalMatrix;
				cubeModelMatrix=glm::mat4(array[0],array[1],array[2],array[3],array[4],array[5],array[6],array[7],array[8],array[9],array[10],array[11],array[12],array[13],array[14],array[15]);
        cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaders[current_program].Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shaders[current_program].Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));

        // we render the cube
        cubeModel.Draw(shaders[current_program]);

        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    DeleteShaders();
		DeletePhysics();
    // we close and delete the created context
    glfwTerminate();
    return 0;
}


//////////////////////////////////////////
// we create and compile shaders (code of Shader class is in include/utils/shader_v1.h), and we add them to the list of available shaders
void SetupShaders()
{
  // we create the Shader Programs (code in shader_v1.h)
    Shader shader1("18_phong_tex_multiplelights.vert", "19a_blinnphong_tex_multiplelights.frag");
    shaders.push_back(shader1);
    Shader shader2("18_phong_tex_multiplelights.vert", "19b_ggx_tex_multiplelights.frag");
    shaders.push_back(shader2);
}

//////////////////////////////////////////
// we load the image from disk and we create an OpenGL texture
GLint LoadTexture(const char* path)
{
    GLuint textureImage;
    int w, h, channels;
    unsigned char* image;
    image = stbi_load(path, &w, &h, &channels, STBI_rgb);

    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;

    glGenTextures(1, &textureImage);
    glBindTexture(GL_TEXTURE_2D, textureImage);
    // 3 channels = RGB ; 4 channel = RGBA
    if (channels==3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels==4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    // we set how to consider UVs outside [0,1] range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
    return textureImage;
}

/////////////////////////////////////////
// we delete all the Shaders Programs
void DeleteShaders()
{
    for(GLuint i = 0; i < shaders.size(); i++)
        shaders[i].Delete();
}

//////////////////////////////////////////
// we print on console the name of the currently used shader
void PrintCurrentShader(int shader)
{
    std::cout << "Current shader:" << print_available_ShaderPrograms[shader]  << std::endl;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
		if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
				printf("Applying impulse to the cube!\n");
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[0];
				btRigidBody* body = btRigidBody::upcast(obj);
				body->applyForce(btVector3(-5000.f,+10000.f,0.f),btVector3(-1,0,0));
		}
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;

    // pressing a key between 1 and 2, we change the shader applied to the models
    if((key >= GLFW_KEY_1 && key <= GLFW_KEY_2) && action == GLFW_PRESS)
    {
        // "1" to "2" -> ASCII codes from 49 to 50
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 2
        // we subtract 1 to have indices from 0 to 1 in the shaders list
        current_program = (key-'0'-1);
        PrintCurrentShader(current_program);
    }
    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	GLint viewport[4]; //var to hold the viewport info
        GLdouble modelview[16]; //var to hold the modelview info
        GLdouble projection[16]; //var to hold the projection matrix info
        GLfloat winX, winY, winZ; //variables to hold screen x,y,z coordinates
        GLdouble worldX, worldY, worldZ; //variables to hold world x,y,z coordinates

        glGetDoublev( , modelview ); //get the modelview info
        glGetDoublev( GL_PROJECTION_MATRIX, projection ); //get the projection matrix info
        glGetIntegerv( GL_VIEWPORT, viewport ); //get the viewport info

	winX = (float)x;
        winY = (float)viewport[3] - (float)y;
	winZ = 0;
	
	//get the world coordinates from the screen coordinates
        gluUnProject( winX, winY, winZ, modelview, projection, viewport, &worldX, &worldY, &worldZ);

}

void SetUpPhysics()
{
	collisionConfiguration = new btDefaultCollisionConfiguration();
	
	dispatcher = new btCollisionDispatcher (collisionConfiguration);
	
	overlappingPairCache = new btDbvtBroadphase ();
	
	solver = new btSequentialImpulseConstraintSolver;
	
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration );
	
	dynamicsWorld -> setGravity (btVector3 (0 , -10 ,0));
	
	cubeShape = new btBoxShape(btVector3(btScalar(1.0),btScalar(1.0),btScalar(1.0)));
	
	collisionShapes.push_back(cubeShape);
	
	btTransform cubeTransform;
	
	cubeTransform.setIdentity();
	
	cubeTransform.setOrigin(btVector3(5.0,1.0,0.0));
	
	btScalar mass(10.0);
	
	btVector3 localInertia(0,0,0);
	
	cubeShape->calculateLocalInertia(mass, localInertia);
	
	btDefaultMotionState* myMotionState = new btDefaultMotionState(cubeTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, cubeShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);

	//add the body to the dynamics world
	dynamicsWorld->addRigidBody(body);
}

void DeletePhysics()
{
	delete dynamicsWorld;
	
	delete solver;
	
	delete overlappingPairCache;
	
	delete dispatcher;
	
	delete collisionConfiguration;
	
	collisionShapes.clear();
}

