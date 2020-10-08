/*
Physics class - v1:
- initialization of the physics simulation using the Bullet librarty

The class sets up the collision manager and the resolver of the constraints, using basic general-purposes methods provided by the library. Advanced and multithread methods are available, please consult Bullet documentation and examples

createRigidBody method sets up a  Box or Sphere Collision Shape. For other Shapes, you must extend the method.

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

#pragma once
#include <time.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#define COLOR_LIMIT 256

class Scene
{
private:
	Physics engine;
	vector<Mesh> displayedMesh;
	GLfloat deltaTime;
	const GLfloat maxSecPerFrame=1.0f / 60.0f;
	GLfloat Kd = 0.8f;	
	glm::mat4 projection;
	glm::mat4 view;
	GLfloat currentFrame;
	GLfloat lastFrame;
	Shader meshShader;
	GLfloat meshDiffuseColor[3]={0.0f, 0.0f, 0.0f};
	glm::vec3 lightPositions[3] = {glm::vec3(5.0f, 10.0f, 10.0f), glm::vec3(-5.0f, 10.0f, 10.0f), glm::vec3(5.0f, 10.0f, -10.0f)};

public:
	Scene(glm::mat4 projection, glm::mat4 view)
	{
		engine=Physics();
		deltaTime=0.0f;
		currentFrame=0.0f;
		lastFrame=0.0f;
		this->projection=projection;
		this->view=view;
		meshShader=Shader("lambert.vert", "lambert.frag");
	}
	
	void AddMesh(int meshIndex, string meshPath)
	{
		GLfloat red=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		GLfloat green=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		GLfloat blue=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		meshDiffuseColor[0]=red;
		meshDiffuseColor[1]=green;
		meshDiffuseColor[2]=blue;
		Model* object = new Model(meshPath);
		displayedMesh.insert(displayedMesh.end(), object->meshes.begin(), object->meshes.end());
		engine.addRigidBodyWithImpulse(meshIndex);
	}
	
	bool AllMeshRemoved()
	{
		return engine.allObjectRemoved();
	}
	
	void DrawMeshes()
	{
		int i=0;
		std::vector<Mesh>::iterator displayedMeshIt;
		for (displayedMeshIt = displayedMesh.begin(); displayedMeshIt != displayedMesh.end(); ++displayedMeshIt)
		{
			meshShader.Use();
			glUniformMatrix4fv(glGetUniformLocation(meshShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(meshShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
			// we determine the position in the Shader Program of the uniform variables
			GLint pointLightLocation = glGetUniformLocation(meshShader.Program, "pointLightPosition");
			GLint objectDiffuseLocation = glGetUniformLocation(meshShader.Program, "diffuseColor");
			GLint kdObjectLocation = glGetUniformLocation(meshShader.Program, "Kd");
			// we assign the value to the uniform variables
			glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPositions[0]));
			glUniform3fv(objectDiffuseLocation, 1, meshDiffuseColor);
			glUniform1f(kdObjectLocation, Kd);
			glm::mat4 objectModelMatrix=engine.getObjectModelMatrix(i);
			glm::mat3 objectNormalMatrix;
			objectNormalMatrix = glm::inverseTranspose(glm::mat3(view*objectModelMatrix));
			glUniformMatrix4fv(glGetUniformLocation(meshShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objectModelMatrix));
			glUniformMatrix3fv(glGetUniformLocation(meshShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
			displayedMeshIt->Draw(meshShader);
			i++;
		}
	}
	
	void SimulationStep()
	{		
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		engine.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);
		engine.removeObjectUnderThreshold();
		if(AllMeshRemoved())
			displayedMesh.clear();
	}
	
	void Clear()
	{
		engine.Clear();
		meshShader.Delete();
		displayedMesh.clear();
	}    
};
