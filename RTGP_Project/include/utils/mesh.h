/*
Mesh class - v2
- allocation and initialization of VBO, VAO, and EBO buffers, and sets as OpenGL must consider the data in the buffers
VBO : Vertex Buffer Object - memory allocated on GPU memory to store the mesh data (vertices and their attributes, like e.g. normals, etc)
EBO : Element Buffer Object - a buffer maintaining the indices of vertices composing the mesh faces
VAO : Vertex Array Object - a buffer that helps to "manage" VBO and its inner structure. It stores pointers to the different vertex attributes stored in the VBO. When we need to render an object, we can just bind the corresponding VAO, and all the needed calls to set up the binding between vertex attributes and memory positions in the VBO are automatically configured.
See https://learnopengl.com/#!Getting-started/Hello-Triangle for details.
N.B. 1) in this version of the class, textures are loaded and applied
N.B. 2) adaptation of https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/mesh.h
author: Davide Gadia
Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

#pragma once

using namespace std;

// Std. Includes
#include <stdlib.h> 
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

// GL Includes
#include <glad/glad.h> // Contains all the necessery OpenGL includes
// we use GLM data structures to write data in the VBO, VAO and EBO buffers
#include <glm/glm.hpp>
#include <btConvexHullShape.h>

// data structure for vertices
struct Vertex {
    // vertex coordinates
    glm::vec3 Position;
    // Normal
    glm::vec3 Normal;
    // Texture coordinates
    glm::vec2 TexCoords;
    // Tangent
    glm::vec3 Tangent;
    // Bitangent
    glm::vec3 Bitangent;
	
	glm::vec4 GetPosition()
	{
		return glm::vec4(Position.x, Position.y, Position.z, 1.0f);
	}
	
	bool Equals(Vertex other)
	{
		float epsilon=0.001f;
		glm::vec3 otherPosition=other.GetPosition();
		float deltaX=fabs(Position.x-otherPosition.x);
		float deltaY=fabs(Position.y-otherPosition.y);
		float deltaZ=fabs(Position.z-otherPosition.z);
		float deltaXNormal=fabs(Normal.x-other.Normal.x);
		float deltaYNormal=fabs(Normal.y-other.Normal.y);
		float deltaZNormal=fabs(Normal.z-other.Normal.z);
		float deltaU=fabs(TexCoords.x-other.TexCoords.x);
		float deltaV=fabs(TexCoords.y-other.TexCoords.y);
		float deltaXTangent=fabs(Tangent.x-other.Tangent.x);
		float deltaYTangent=fabs(Tangent.y-other.Tangent.y);
		float deltaZTangent=fabs(Tangent.z-other.Tangent.z);
		float deltaXBitangent=fabs(Bitangent.x-other.Bitangent.x);
		float deltaYBitangent=fabs(Bitangent.y-other.Bitangent.y);
		float deltaZBitangent=fabs(Bitangent.z-other.Bitangent.z);
		
		return (deltaX<=epsilon) && (deltaY<=epsilon) && (deltaZ<=epsilon) && 
				(deltaXNormal<=epsilon) && (deltaYNormal<=epsilon) && (deltaZNormal<=epsilon) &&
				(deltaU<=epsilon) && (deltaV<=epsilon) &&
				(deltaXTangent<=epsilon) && (deltaYTangent<=epsilon) && (deltaZTangent<=epsilon) &&
				(deltaXBitangent<=epsilon) && (deltaYBitangent<=epsilon) && (deltaZBitangent<=epsilon);
	}
};

// data structure for textures
struct Texture {
    GLuint id;
    string type;
    aiString path;
};

/////////////////// MESH class ///////////////////////
class Mesh {
public:
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<Texture> textures;
    GLuint VAO;

	Mesh(){}
    //////////////////////////////////////////
    // Constructor
	Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures)
	{
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
		this->setupMesh();
	}
    //////////////////////////////////////////
	
	glm::vec4 CalculateCentroid(float area, vector<float> triangleAreas, vector<glm::vec3> triangleCentroids, vector<GLuint> ind, vector<Vertex> ver)
	{
		glm::vec3 centroid;
		for(unsigned int i=0;i<triangleCentroids.size();i++)
		{
			centroid+=(triangleAreas[i]*triangleCentroids[i]);
		}
		centroid/=area;
		return glm::vec4(centroid.x, centroid.y, centroid.z, 1.0f);
	}
	
	float CalculateTriangleArea(Vertex a, Vertex b, Vertex c)
	{
		glm::vec3 ba=b.GetPosition()-a.GetPosition();
		glm::vec3 ca=c.GetPosition()-a.GetPosition();
		glm::vec3 cross=glm::cross(ba, ca);
		return (glm::sqrt(cross.x*cross.x+cross.y*cross.y+cross.z*cross.z))*0.5f;
	}
	
	glm::vec3 CalculateTriangleCentroid(Vertex a, Vertex b, Vertex c)
	{
		glm::vec4 triangleCentroid=(a.GetPosition()+b.GetPosition()+c.GetPosition())/3.0f;
		return glm::vec3(triangleCentroid.x, triangleCentroid.y, triangleCentroid.z);
	}
	
	int ExistVertex(Vertex v, vector<Vertex> ver, vector<GLuint> ind)
	{
		for(int j=0;j<ver.size();j++)
		{
			if(ver[j].Equals(v))
			{
				return j;
			}
		}
		return -1;
	}
	
	int AddVertex(Vertex v, vector<Vertex> & ver, vector<GLuint> & ind)
	{
		int j=ExistVertex(v, ver, ind);
			
		if(0<=j)
		{
			ind.push_back(j);
			return j;
		}
		else
		{
			ver.push_back(v);
			ind.push_back(ver.size()-1);
			return ver.size()-1;
		}
	}
	
	void AddNewTriangle(glm::vec4 planePoint, 
						glm::vec4 planeNormal, 
						vector<Vertex> & negativeMeshVertices, 
						vector<GLuint> & negativeMeshIndices, 
						float & negativeMeshArea,
						vector<glm::vec3> & negativeTriangleCentroids,
						vector<float> & negativeTriangleAreas,
						vector<Vertex> & positiveMeshVertices, 
						vector<GLuint> & positiveMeshIndices, 
						float & positiveMeshArea,
						vector<glm::vec3> & positiveTriangleCentroids,
						vector<float> & positiveTriangleAreas,
						vector<Vertex> newVertices, 
						int indA, 
						int indB, 
						int indC)
	{
		
		//true means positive, false means negative
		bool a=(PositiveOrNegativeSide(indA, planeNormal, planePoint)>0.f);
		bool b=(PositiveOrNegativeSide(indB, planeNormal, planePoint)>0.f);
		bool c=(PositiveOrNegativeSide(indC, planeNormal, planePoint)>0.f);
		
		vector<int> pV;
		vector<int> nV;
		float triangleArea;
		
		//6 cases
		if(a & !b & !c)
		{
			pV.push_back(indA);
			nV.push_back(indB);
			nV.push_back(indC);
		}
		else if(!a & b & !c)
		{
			pV.push_back(indB);
			nV.push_back(indA);
			nV.push_back(indC);
		}
		else if(!a & !b & c)
		{
			pV.push_back(indC);
			nV.push_back(indB);
			nV.push_back(indA);
		}
		else if(a & b & !c)
		{
			pV.push_back(indA);
			pV.push_back(indB);
			nV.push_back(indC);
			
		}
		else if(a & !b & c)
		{
			pV.push_back(indC);
			pV.push_back(indA);
			nV.push_back(indB);
			//A C positive
		}
		else if(!a & b & c)
		{
			pV.push_back(indC);
			pV.push_back(indB);
			nV.push_back(indA);		
			//B C positive
		}
		
		//2 triangle above or below the cut?
		if(pV.size()==2)
		{
			//first triangle
			AddVertex(vertices[nV[0]], negativeMeshVertices, negativeMeshIndices);
			AddVertex(newVertices[0], negativeMeshVertices, negativeMeshIndices);
			AddVertex(newVertices[1], negativeMeshVertices, negativeMeshIndices);
			
			triangleArea=CalculateTriangleArea(vertices[nV[0]], newVertices[0], newVertices[1]);
			negativeTriangleAreas.push_back(triangleArea);
			negativeMeshArea+=triangleArea;
			negativeTriangleCentroids.push_back(CalculateTriangleCentroid(vertices[nV[0]], newVertices[0], newVertices[1]));

			int k=AddVertex(vertices[pV[0]], positiveMeshVertices, positiveMeshIndices);
			AddVertex(vertices[pV[1]], positiveMeshVertices, positiveMeshIndices);
			int j=AddVertex(newVertices[0], positiveMeshVertices, positiveMeshIndices);
			
			triangleArea=CalculateTriangleArea(vertices[pV[0]], vertices[pV[1]], newVertices[0]);
			positiveTriangleAreas.push_back(triangleArea);
			positiveMeshArea+=triangleArea;
			positiveTriangleCentroids.push_back(CalculateTriangleCentroid(vertices[pV[0]], vertices[pV[1]], newVertices[0]));
			
			positiveMeshIndices.push_back(j);
			AddVertex(newVertices[1], positiveMeshVertices, positiveMeshIndices);
			positiveMeshIndices.push_back(k);
			
			triangleArea=CalculateTriangleArea(vertices[pV[0]], newVertices[1], newVertices[0]);
			positiveTriangleAreas.push_back(triangleArea);
			positiveMeshArea+=triangleArea;
			positiveTriangleCentroids.push_back(CalculateTriangleCentroid(vertices[pV[0]], newVertices[1], newVertices[0]));
		}
		else
		{
			//first triangle
			AddVertex(vertices[pV[0]], positiveMeshVertices, positiveMeshIndices);
			AddVertex(newVertices[0], positiveMeshVertices, positiveMeshIndices);
			AddVertex(newVertices[1], positiveMeshVertices, positiveMeshIndices);
			
			triangleArea=CalculateTriangleArea(vertices[pV[0]], newVertices[0], newVertices[1]);
			positiveTriangleAreas.push_back(triangleArea);
			positiveMeshArea+=triangleArea;
			positiveTriangleCentroids.push_back(CalculateTriangleCentroid(vertices[pV[0]], newVertices[0], newVertices[1]));
			
			//second triangle
			int k=AddVertex(vertices[nV[0]], negativeMeshVertices, negativeMeshIndices);
			AddVertex(newVertices[0], negativeMeshVertices, negativeMeshIndices);
			int j=AddVertex(newVertices[1], negativeMeshVertices, negativeMeshIndices);
			
			triangleArea=CalculateTriangleArea(vertices[nV[0]], newVertices[0], newVertices[1]);
			negativeTriangleAreas.push_back(triangleArea);
			negativeMeshArea+=triangleArea;
			negativeTriangleCentroids.push_back(CalculateTriangleCentroid(vertices[nV[0]], newVertices[0], newVertices[1]));
			
			//third triangle
			AddVertex(vertices[nV[1]], negativeMeshVertices, negativeMeshIndices);
			negativeMeshIndices.push_back(k);
			negativeMeshIndices.push_back(j);
			
			triangleArea=CalculateTriangleArea(vertices[nV[1]], vertices[nV[0]], newVertices[1]);
			negativeTriangleAreas.push_back(triangleArea);
			negativeMeshArea+=triangleArea;
			negativeTriangleCentroids.push_back(CalculateTriangleCentroid(vertices[nV[1]], vertices[nV[0]], newVertices[1]));
		}
	}
	
	void AddExistingTriangle(vector<float> & triangleAreas, vector<glm::vec3> & triangleCentroids, float & totalArea, vector<GLuint> & ind, vector<Vertex> & ver, int indA, int indB, int indC)
	{
		bool tmp[3]={false, false, false};
		for(unsigned int i=0; i<ver.size();i++)
		{
			if(ver[i].Equals(vertices[indA]))
			{
				ind.push_back(i);
				tmp[0]=true;
				continue;
			}
			
			if(ver[i].Equals(vertices[indB]))
			{
				ind.push_back(i);
				tmp[1]=true;
				continue;
			}
			
			if(ver[i].Equals(vertices[indC]))
			{
				ind.push_back(i);
				tmp[2]=true;
				continue;
			}
		}
		
		if(!tmp[0])
		{
			ver.push_back(vertices[indA]);
			ind.push_back(ver.size()-1);
		}
		
		if(!tmp[1])
		{
			ver.push_back(vertices[indB]);
			ind.push_back(ver.size()-1);
		}
		
		if(!tmp[2])
		{
			ver.push_back(vertices[indC]);
			ind.push_back(ver.size()-1);
		}
		
		int i=ind[ind.size()-3];
		int j=ind[ind.size()-2];
		int k=ind[ind.size()-1];
		float triangleArea=CalculateTriangleArea(ver[i], ver[j], ver[k]);
		triangleAreas.push_back(triangleArea);
		totalArea+=triangleArea;
		glm::vec3 triangleCentroid=CalculateTriangleCentroid(ver[i], ver[j], ver[k]);
		triangleCentroids.push_back(triangleCentroid);
	}
	
	void Cut(Mesh & positiveMesh, Mesh & negativeMesh, glm::vec4 & positiveMeshPosition, glm::vec4 & negativeMeshPosition, glm::vec4 cutStartPoint, glm::vec4 cutEndPoint, glm::mat4 model ,glm::mat4 view, glm::mat4 projection, btConvexHullShape* & positiveConvexHullShape, btConvexHullShape* & negativeConvexHullShape, float & positiveWeightFactor, float & negativeWeightFactor)
	{
		//Convert world vertices to object space
		glm::mat4 invModel=glm::inverse(model);
		cutStartPoint=invModel*cutStartPoint;
		cutEndPoint=invModel*cutEndPoint;
		//The calculate the normal in object space
		glm::vec4 cutVector=glm::vec4(cutEndPoint.x-cutStartPoint.x, cutEndPoint.y-cutStartPoint.y, 0, cutEndPoint.w-cutStartPoint.w);
		glm::vec4 cutNormal=glm::vec4(-cutVector.y, cutVector.x, 0.0f, 0.0f);
		cutNormal=glm::normalize(cutNormal);
						
		float positiveMeshArea;
		float negativeMeshArea;
		vector<glm::vec3> positiveTriangleCentroids;
		vector<float> positiveTriangleAreas;
		vector<glm::vec3> negativeTriangleCentroids;
		vector<float> negativeTriangleAreas;
		glm::vec4 negativeMeshCentroid;
		glm::vec4 positiveMeshCentroid;
		vector<Vertex> negativeMeshVertices;
		vector<GLuint> negativeMeshIndices;
		vector<Vertex> positiveMeshVertices;
		vector<GLuint> positiveMeshIndices;
		glm::vec4 vertex;
		float d[3]={0.f, 0.f, 0.f};
		positiveConvexHullShape=new btConvexHullShape();
		negativeConvexHullShape=new btConvexHullShape();
		
		Vertex tmp;
		for(unsigned int i=0;i<indices.size();i+=3)
		{
			int indA=indices[i];
			int indB=indices[i+1];
			int indC=indices[i+2];
			
			if(CutTriangle(d, i, cutEndPoint, cutNormal))
			{	
				vector<Vertex> newVertices;
				newVertices=CalculateNewVertices(cutNormal, cutEndPoint, negativeMeshVertices, negativeMeshIndices, positiveMeshVertices, positiveMeshIndices, indA, indB, indC, d);
				AddNewTriangle(	cutEndPoint, 
								cutNormal, 
								negativeMeshVertices, 
								negativeMeshIndices, 
								negativeMeshArea, 
								negativeTriangleCentroids, 
								negativeTriangleAreas, 
								positiveMeshVertices, 
								positiveMeshIndices, 
								positiveMeshArea, 
								positiveTriangleCentroids, 
								positiveTriangleAreas, 
								newVertices, 
								indA, 
								indB, 
								indC);
			}
			else
			{
				//Separe negative vertices from positive ones since there is no cut
				if(PositiveOrNegativeSide(indA, cutNormal, cutEndPoint)>0.f)
					AddExistingTriangle(positiveTriangleAreas, positiveTriangleCentroids, positiveMeshArea, positiveMeshIndices, positiveMeshVertices, indA, indB, indC);
				else
					AddExistingTriangle(negativeTriangleAreas, negativeTriangleCentroids, negativeMeshArea, negativeMeshIndices, negativeMeshVertices, indA, indB, indC);
			}
		}
		
		negativeMeshCentroid=CalculateCentroid(negativeMeshArea, negativeTriangleAreas, negativeTriangleCentroids, negativeMeshIndices, negativeMeshVertices);
		positiveMeshCentroid=CalculateCentroid(positiveMeshArea, positiveTriangleAreas, positiveTriangleCentroids, positiveMeshIndices, positiveMeshVertices);
		
		for(unsigned int i=0;i<positiveMeshVertices.size();i++)
		{
			positiveMeshVertices[i].Position.x-=positiveMeshCentroid.x;
			positiveMeshVertices[i].Position.y-=positiveMeshCentroid.y;
			positiveMeshVertices[i].Position.z-=positiveMeshCentroid.z;
			positiveConvexHullShape->addPoint(btVector3(positiveMeshVertices[i].Position.x, positiveMeshVertices[i].Position.y, positiveMeshVertices[i].Position.z));
		}
		
		for(unsigned int i=0;i<negativeMeshVertices.size();i++)
		{
			negativeMeshVertices[i].Position.x-=negativeMeshCentroid.x;
			negativeMeshVertices[i].Position.y-=negativeMeshCentroid.y;
			negativeMeshVertices[i].Position.z-=negativeMeshCentroid.z;
			negativeConvexHullShape->addPoint(btVector3(negativeMeshVertices[i].Position.x, negativeMeshVertices[i].Position.y, negativeMeshVertices[i].Position.z));
		}
		
		positiveMesh=Mesh(positiveMeshVertices, positiveMeshIndices, textures);
		negativeMesh=Mesh(negativeMeshVertices, negativeMeshIndices, textures);
		
		positiveWeightFactor=positiveMeshArea/(positiveMeshArea+negativeMeshArea);
		negativeWeightFactor=1-positiveWeightFactor;
		
		positiveMeshPosition=model*positiveMeshCentroid;
		negativeMeshPosition=model*negativeMeshCentroid;
	}
	
	float PositiveOrNegativeSide(int i, glm::vec4 planeNormal, glm::vec4 planePoint)
	{
		glm::vec4 vertex=vertices[i].GetPosition();
		glm::vec4 vertexToCutPlane=glm::vec4(vertex.x-planePoint.x, vertex.y-planePoint.y, 0, 0);
		return glm::dot(planeNormal, vertexToCutPlane);
	}
	
	vector<Vertex> CalculateNewVertices(glm::vec4 planeNormal, glm::vec4 planePoint, vector<Vertex> & negativeMeshVertices, vector<GLuint> & negativeMeshIndices, vector<Vertex> & positiveMeshVertices, vector<GLuint> & positiveMeshIndices, GLuint indA, GLuint indB, GLuint indC, float* d)
	{
		vector<Vertex> newVertices;
		Vertex tmp;
		
		if(0.0f<d[0] && d[0]<1.0f)
		{
			tmp=Vertex();
			tmp.Position=vertices[indB].GetPosition()*d[0]+vertices[indA].GetPosition()*(1.0f-d[0]);
			tmp.Normal=vertices[indB].Normal*d[0]+vertices[indA].Normal*(1.0f-d[0]);
			tmp.Tangent=vertices[indB].Tangent*d[0]+vertices[indA].Tangent*(1.0f-d[0]);
			tmp.Bitangent=vertices[indB].Bitangent*d[0]+vertices[indA].Bitangent*(1.0f-d[0]);
			newVertices.push_back(tmp);
		}
				
		if(0.0f<d[1] && d[1]<1.0f)
		{
			tmp=Vertex();
			tmp.Position=vertices[indC].GetPosition()*d[1]+vertices[indB].GetPosition()*(1.0f-d[1]);
			tmp.Normal=vertices[indC].Normal*d[1]+vertices[indB].Normal*(1.0f-d[1]);
			tmp.Tangent=vertices[indC].Tangent*d[1]+vertices[indB].Tangent*(1.0f-d[1]);
			tmp.Bitangent=vertices[indC].Bitangent*d[1]+vertices[indB].Bitangent*(1.0f-d[1]);
			newVertices.push_back(tmp);
		}
			
		if(0.0f<d[2] && d[2]<1.0f)
		{
			tmp=Vertex();
			tmp.Position=vertices[indC].GetPosition()*d[2]+vertices[indA].GetPosition()*(1.0f-d[2]);
			tmp.Normal=vertices[indC].Normal*d[2]+vertices[indA].Normal*(1.0f-d[2]);
			tmp.Tangent=vertices[indC].Tangent*d[2]+vertices[indA].Tangent*(1.0f-d[2]);
			tmp.Bitangent=vertices[indC].Bitangent*d[2]+vertices[indA].Bitangent*(1.0f-d[2]);
			newVertices.push_back(tmp);
		}
		
		return newVertices;
	}
	
	bool CutTriangle(float* d, int i, glm::vec4 planePoint, glm::vec4 cutNormal)
	{
		glm::vec4 a=vertices[indices[i]].GetPosition();
		glm::vec4 b=vertices[indices[i+1]].GetPosition();
		float tmp=glm::dot(b-a, cutNormal);
		if(tmp==0.0)
		{
			d[0]=-1.f;
		}
		else
		{	
			tmp=glm::dot(planePoint-a, cutNormal)/tmp;
			d[0]=tmp;
		}
		
		a=vertices[indices[i+1]].GetPosition();
		b=vertices[indices[i+2]].GetPosition();
		tmp=glm::dot(b-a, cutNormal);
		if(tmp==0.0)
		{
			d[1]=-1.f;
		}
		else
		{
			tmp=glm::dot(planePoint-a, cutNormal)/tmp;
			d[1]=tmp;
		}
		
		a=vertices[indices[i]].GetPosition();
		b=vertices[indices[i+2]].GetPosition();
		tmp=glm::dot(b-a, cutNormal);
		
		if(tmp==0.0)
		{
			d[2]=-1.f;
		}
		else
		{
			tmp=glm::dot(planePoint-a, cutNormal)/tmp;
			d[2]=tmp;
		}
		
		if((d[0]<0.f || 1.0f<d[0]) && (d[1]<0.f || 1.0f<d[1]) && (d[2]<0.f || 1.0f<d[2]))
			return false;
		return true;
	}

    // rendering of mesh
    void Draw(Shader shader)
    {
        // Bind appropriate textures
        GLuint diffuseNr = 1;
        GLuint specularNr = 1;
        GLuint normalNr = 1;
        GLuint heightNr = 1;
        for(GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // Active proper texture unit before binding
            // Retrieve texture number (the N in diffuse_textureN)
            stringstream ss;
            string number;
            string name = this->textures[i].type;
            if(name == "texture_diffuse")
                ss << diffuseNr++; // Transfer GLuint to stream
            else if(name == "texture_specular")
                ss << specularNr++; // Transfer GLuint to stream
            else if(name == "texture_normal")
                ss << normalNr++; // Transfer GLuint to stream
             else if(name == "texture_height")
                ss << heightNr++; // Transfer GLuint to stream
            number = ss.str();
			
            // Now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader.Program, (name + number).c_str()), i);
            // And finally bind the texture
            glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
        }

        // VAO is made "active"
        glBindVertexArray(this->VAO);
        // rendering of data in the VAO
        glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
        // VAO is "detached"
        glBindVertexArray(0);

        // Always good practice to set everything back to defaults once configured.
        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    //////////////////////////////////////////

    // buffers are deallocated when application ends
    void Delete()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
  // VBO and EBO
  GLuint VBO, EBO;

  //////////////////////////////////////////
  // buffer objects\arrays are initialized
  // a brief description of their role and how they are binded can be found at:
  // https://learnopengl.com/#!Getting-started/Hello-Triangle
  // (in different parts of the page), or here:
  // http://www.informit.com/articles/article.aspx?p=1377833&seqNum=8
  void setupMesh()
  {
      // we create the buffers
      glGenVertexArrays(1, &this->VAO);
      glGenBuffers(1, &this->VBO);
      glGenBuffers(1, &this->EBO);

      // VAO is made "active"
      glBindVertexArray(this->VAO);
      // we copy data in the VBO - we must set the data dimension, and the pointer to the structure cointaining the data
      glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
      glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);
      // we copy data in the EBO - we must set the data dimension, and the pointer to the structure cointaining the data
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

      // we set in the VAO the pointers to the different vertex attributes (with the relative offsets inside the data structure)
      // vertex positions
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
      // Normals
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
      // Texture Coordinates
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));
      // Tangent
      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Tangent));
      // Bitangent
      glEnableVertexAttribArray(4);
      glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Bitangent));

      glBindVertexArray(0);
  }
};