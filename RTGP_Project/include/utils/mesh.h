/*
Mesh class:
This class store and manage all data structures that define a triangular mesh on the gpu.
Here there is the implementation of the cut method, which is the core of this project.
Each cut produces two new meshes: the positive and negative one.
All points of the positive mesh lies in the half space(defined by the cut segment), where the half
plane test returns positive values (in this case, the plane is defined by the cutting segment).
*/

#pragma once

using namespace std;

#include <stdlib.h> 
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <btConvexHullShape.h>
#include <utils/vertex.h>
#include <utils/texture.h>

namespace std
{
	template<>
	struct hash<glm::vec3>
	{
		size_t operator()(const glm::vec3& v) const
		{
			return std::hash<float>{}(v.x) || std::hash<float>{}(v.y) << 2 || std::hash<float>{}(v.z) >> 2;
		}
	};
	
	template<>
	struct hash<Vertex>
	{
		size_t operator()(const Vertex& v) const
		{
			return std::hash<float>{}(v.Position.x) || std::hash<float>{}(v.Position.y) << 2 || std::hash<float>{}(v.Position.z) >> 2;
		}
	};
}

class Mesh {
public:
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<Texture> textures;
    GLuint VAO;

	Mesh(){}
    //CONSTRUCTOR
	Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures)
	{
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
		this->setupMesh();
	}
	//This method calculates the area of the triangle defined by a, b, c vertices; it is used, together with the method below,
	//to calculate the new pivots, needed after each cut.
	float CalculateTriangleArea(glm::vec3 a, glm::vec3 b, glm::vec3 c)
	{
		glm::vec3 ba=b-a;
		glm::vec3 ca=c-a;
		glm::vec3 cross=glm::cross(ba, ca);
		return (glm::sqrt(cross.x*cross.x+cross.y*cross.y+cross.z*cross.z))*0.5f;
	}
	glm::vec3 CalculateTriangleCenter(glm::vec3 a, glm::vec3 b, glm::vec3 c)
	{
		glm::vec3 triangleCenter=(a+b+c)/3.0f;
		return triangleCenter;
	}
	//Whether a cut occurs, some new points must be generated for all triangle which intersect the cutting plane.
	//This method returns a vector containing the new vertices, obtained from the intersection of the cutting plane and the triangle a, b, c.
	//Intfactors is the vector that holds the interpolation factors; each interpolation factor specifies if there is or not, intersection on a particular
	//segment of the triangle a, b, c.
	//For instance, if the condition 0.0f<=intFactors[0]<=1.0f is true, the cutting plane intersects the segment a, b of the triangle; so it's possible
	//to obtain the new vertex, by linear interpolating between a and b.
	//Intfactors[0] -> interpolation between a, b
	//Intfactors[1] -> interpolation between b, c
	//Intfactors[2] -> interpolation between c, a
	vector<Vertex> CalculateNewVertices(Vertex a, Vertex b, Vertex c, glm::vec4 planeNormal, glm::vec4 planePoint, float* intFactors)
	{
		vector<Vertex> newVertices;
		Vertex tmp;
		//a, b
		if(0.0f<=intFactors[0] && intFactors[0]<=1.0f)
		{
			tmp=Vertex();
			tmp.Position=b.Position*intFactors[0]+a.Position*(1.0f-intFactors[0]);
			tmp.Normal=b.Normal*intFactors[0]+a.Normal*(1.0f-intFactors[0]);
			tmp.Tangent=b.Tangent*intFactors[0]+a.Tangent*(1.0f-intFactors[0]);
			tmp.Bitangent=b.Bitangent*intFactors[0]+a.Bitangent*(1.0f-intFactors[0]);
			newVertices.push_back(tmp);
		}
		//b, c		
		if(0.0f<=intFactors[1] && intFactors[1]<=1.0f)
		{
			tmp=Vertex();
			tmp.Position=c.Position*intFactors[1]+b.Position*(1.0f-intFactors[1]);
			tmp.Normal=c.Normal*intFactors[1]+b.Normal*(1.0f-intFactors[1]);
			tmp.Tangent=c.Tangent*intFactors[1]+b.Tangent*(1.0f-intFactors[1]);
			tmp.Bitangent=c.Bitangent*intFactors[1]+b.Bitangent*(1.0f-intFactors[1]);
			newVertices.push_back(tmp);
		}
		//c, a	
		if(0.0f<=intFactors[2] && intFactors[2]<=1.0f)
		{
			tmp=Vertex();
			tmp.Position=c.Position*intFactors[2]+a.Position*(1.0f-intFactors[2]);
			tmp.Normal=c.Normal*intFactors[2]+a.Normal*(1.0f-intFactors[2]);
			tmp.Tangent=c.Tangent*intFactors[2]+a.Tangent*(1.0f-intFactors[2]);
			tmp.Bitangent=c.Bitangent*intFactors[2]+a.Bitangent*(1.0f-intFactors[2]);
			newVertices.push_back(tmp);
		}
		
		return newVertices;
	}
	//This function is called, only if a triangle of the mesh is divided by the cutting plane.
	//all triangle cut by the plane must generate 3 new triangle, which are assigned to the positive or
	//negative mesh, according how the cut has been performed.
	//Furthermore, this method generate a new face to fill the empty section there would be after the cut.
	//Finally these new triangles are then assigned to the positive and negative data structures.
	void AddNewTriangle(glm::vec4 planePoint, 
						glm::vec4 planeNormal,
						Vertex & sectionVertexCentroid,
						unordered_map<Vertex, int> & negativeSectionVertexIndexMap,
						unordered_map<Vertex, vector<int>> & negativeVertexIndexMap,
						vector<Vertex> & negativeMeshVertices, 
						vector<GLuint> & negativeMeshIndices, 
						glm::vec3 negativeCentroid,
						float & negativeMeshTotalArea,
						unordered_map<Vertex, int> & positiveSectionVertexIndexMap,
						unordered_map<Vertex, vector<int>> & positiveVertexIndexMap,
						vector<Vertex> & positiveMeshVertices, 
						vector<GLuint> & positiveMeshIndices, 
						glm::vec3 & positiveCentroid,
						float & positiveMeshTotalArea,
						vector<Vertex> newVertices,
						vector<Vertex> triangleVertices)
	{
		//True means positive, false negative.
		bool aCheck=triangleVertices[0].PositiveOrNegativeSide(planeNormal, planePoint)>0.f;
		bool bCheck=triangleVertices[1].PositiveOrNegativeSide(planeNormal, planePoint)>0.f;
		bool cCheck=triangleVertices[2].PositiveOrNegativeSide(planeNormal, planePoint)>0.f;
		
		vector<Vertex> positiveVertices;
		vector<Vertex> negativeVertices;
		float triangleArea;
		glm::vec3 triangleCenter;
		
		//6 cases
		if(aCheck & !bCheck & !cCheck)
		{
			positiveVertices.push_back(triangleVertices[0]);
			positiveVertices.push_back(newVertices[1]);
			positiveVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, newVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, newVertices[1].Position, newVertices[0].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[1]);
			negativeVertices.push_back(newVertices[0]);
			negativeVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[1].Position, newVertices[0].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[1].Position, newVertices[0].Position, newVertices[1].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[2]);
			negativeVertices.push_back(triangleVertices[1]);
			negativeVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[2].Position, triangleVertices[1].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[2].Position, triangleVertices[1].Position, newVertices[1].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
		}
		else if(!aCheck & bCheck & !cCheck)
		{
			positiveVertices.push_back(triangleVertices[1]);
			positiveVertices.push_back(newVertices[1]);
			positiveVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[1].Position, newVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[1].Position, newVertices[1].Position, newVertices[0].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[2]);
			negativeVertices.push_back(newVertices[1]);
			negativeVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[2].Position, newVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[2].Position, newVertices[1].Position, newVertices[0].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[0]);
			negativeVertices.push_back(triangleVertices[2]);
			negativeVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, triangleVertices[2].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, triangleVertices[2].Position, newVertices[0].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
		}
		else if(!aCheck & !bCheck & cCheck)
		{
			positiveVertices.push_back(triangleVertices[2]);
			positiveVertices.push_back(newVertices[1]);
			positiveVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[2].Position, newVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[2].Position, newVertices[1].Position, newVertices[0].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[0]);
			negativeVertices.push_back(newVertices[1]);
			negativeVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, newVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, newVertices[1].Position, newVertices[0].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[0]);
			negativeVertices.push_back(triangleVertices[1]);
			negativeVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, triangleVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, triangleVertices[1].Position, newVertices[0].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
		}
		else if(aCheck & bCheck & !cCheck)
		{
			positiveVertices.push_back(triangleVertices[0]);
			positiveVertices.push_back(triangleVertices[1]);
			positiveVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, triangleVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, triangleVertices[1].Position, newVertices[0].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			positiveVertices.push_back(triangleVertices[0]);
			positiveVertices.push_back(newVertices[1]);
			positiveVertices.push_back(newVertices[0]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, newVertices[1].Position, newVertices[0].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, newVertices[1].Position, newVertices[0].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[2]);
			negativeVertices.push_back(newVertices[0]);
			negativeVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[2].Position, newVertices[0].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[2].Position, newVertices[0].Position, newVertices[1].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
		}
		else if(aCheck & !bCheck & cCheck)
		{
			positiveVertices.push_back(triangleVertices[0]);
			positiveVertices.push_back(triangleVertices[2]);
			positiveVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position, triangleVertices[2].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, triangleVertices[2].Position, newVertices[1].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			positiveVertices.push_back(triangleVertices[0]);
			positiveVertices.push_back(newVertices[0]);
			positiveVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position,  newVertices[0].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position,  newVertices[0].Position, newVertices[1].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[1]);
			negativeVertices.push_back(newVertices[0]);
			negativeVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[1].Position,  newVertices[0].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[1].Position,  newVertices[0].Position, newVertices[1].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
		}
		else if(!aCheck & bCheck & cCheck)
		{
			positiveVertices.push_back(triangleVertices[1]);
			positiveVertices.push_back(triangleVertices[2]);
			positiveVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[1].Position,  triangleVertices[2].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[1].Position,  triangleVertices[2].Position, newVertices[1].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			positiveVertices.push_back(triangleVertices[1]);
			positiveVertices.push_back(newVertices[0]);
			positiveVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[1].Position,  newVertices[0].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[1].Position,  newVertices[0].Position, newVertices[1].Position);
			positiveCentroid+=(triangleArea*triangleCenter);
			positiveMeshTotalArea+=triangleArea;
			
			negativeVertices.push_back(triangleVertices[0]);
			negativeVertices.push_back(newVertices[0]);
			negativeVertices.push_back(newVertices[1]);
			triangleArea=CalculateTriangleArea(triangleVertices[0].Position,  newVertices[0].Position, newVertices[1].Position);
			triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position,  newVertices[0].Position, newVertices[1].Position);
			negativeCentroid+=(triangleArea*triangleCenter);
			negativeMeshTotalArea+=triangleArea;
		}
		
		//Section's face creation
		//First push the index of the section centroid
		positiveMeshIndices.push_back(0);
		negativeMeshIndices.push_back(0);
		//Second add the other two points
		for(int i=0;i<2;i++)
		{
			auto it=positiveSectionVertexIndexMap.find(newVertices[i]);
			if(it==positiveSectionVertexIndexMap.end())
			{
				Vertex vertex=Vertex();
				vertex.Position=newVertices[i].Position;
				vertex.Normal=(-planeNormal);
				vertex.TexCoords=glm::vec2(0, 0);
				vertex.Bitangent=glm::vec3(0, 0, 0);
				vertex.Tangent=glm::vec3(0, 0, 0);
				positiveMeshVertices.push_back(vertex);
				int index=positiveMeshVertices.size()-1;
				positiveMeshIndices.push_back(index);
				positiveSectionVertexIndexMap[vertex]=index;
				sectionVertexCentroid.Position.x+=vertex.Position.x;
				sectionVertexCentroid.Position.y+=vertex.Position.y;
				sectionVertexCentroid.Position.z+=vertex.Position.z;
			}
			else
			{
				positiveMeshIndices.push_back(positiveSectionVertexIndexMap[newVertices[i]]);
			}
		
			it=negativeSectionVertexIndexMap.find(newVertices[i]);
			if(it==negativeSectionVertexIndexMap.end())
			{
				Vertex vertex=Vertex();
				vertex.Position=newVertices[i].Position;
				vertex.Normal=planeNormal;
				vertex.TexCoords=glm::vec2(0, 0);
				vertex.Bitangent=glm::vec3(0, 0, 0);
				vertex.Tangent=glm::vec3(0, 0, 0);
				negativeMeshVertices.push_back(vertex);
				int index=negativeMeshVertices.size()-1;
				negativeMeshIndices.push_back(index);
				negativeSectionVertexIndexMap[vertex]=index;
			}
			else
			{
				negativeMeshIndices.push_back(negativeSectionVertexIndexMap[newVertices[i]]);
			}
		}
		
		//Update mesh structures
		for(unsigned int i=0;i<positiveVertices.size();i++)
		{
			auto itV=positiveVertexIndexMap.find(positiveVertices[i]);
			if(itV==positiveVertexIndexMap.end())
			{
				positiveMeshVertices.push_back(positiveVertices[i]);
				int index=positiveMeshVertices.size()-1;
				positiveMeshIndices.push_back(index);
				positiveVertexIndexMap[positiveVertices[i]].push_back(index);
			}
			else
			{
				bool found=false;
				vector<int> existingVertices=positiveVertexIndexMap[positiveVertices[i]];
				for(unsigned int j=0;j<existingVertices.size();j++)
				{
					int k=existingVertices[j];
					if(positiveMeshVertices[k].Equals(positiveVertices[i]))
					{
						found=true;
						positiveMeshIndices.push_back(k);
						break;
					}
				}
				
				if(!found)
				{
					positiveMeshVertices.push_back(positiveVertices[i]);
					int index=positiveMeshVertices.size()-1;
					positiveMeshIndices.push_back(index);
					positiveVertexIndexMap[positiveVertices[i]].push_back(index);
				}
			}
		}
		
		for(unsigned int i=0;i<negativeVertices.size();i++)
		{
			auto it=negativeVertexIndexMap.find(negativeVertices[i]);
			if(it==negativeVertexIndexMap.end())
			{
				negativeMeshVertices.push_back(negativeVertices[i]);
				int index=negativeMeshVertices.size()-1;
				negativeMeshIndices.push_back(index);
				negativeVertexIndexMap[negativeVertices[i]].push_back(index);
			}
			else
			{
				bool found=false;
				vector<int> existingVertices=negativeVertexIndexMap[negativeVertices[i]];
				for(unsigned int j=0;j<existingVertices.size();j++)
				{
					int k=existingVertices[j];
					if(negativeMeshVertices[k].Equals(negativeVertices[i]))
					{
						found=true;
						negativeMeshIndices.push_back(k);
						break;
					}
				}
				
				if(!found)
				{
					negativeMeshVertices.push_back(negativeVertices[i]);
					int index=negativeMeshVertices.size()-1;
					negativeMeshIndices.push_back(index);
					negativeVertexIndexMap[negativeVertices[i]].push_back(index);
				}
			}
		}
	}
	//This function calculates the interpolation factors, between a triangle and the cutting plane.
	//These factors are needed to calculate the new points that lie on the cutting section.
	//Each factor refers to a segment of the considered triangle: if at least two of these factors are between 0 and 1,
	//then there is intersection.
	bool CutTriangle(float* intFactors, vector<Vertex> triangleVertices, glm::vec3 planePoint, glm::vec3 cutNormal)
	{
		int tmp[]={0, 1, 1, 2, 0, 2};
		int j=0;
		for(int i=0; i<5; i+=2)
		{
			glm::vec3 a=triangleVertices[tmp[i]].Position;
			glm::vec3 b=triangleVertices[tmp[i+1]].Position;
			float tmp=glm::dot(b-a, cutNormal);
			if(tmp==0.0)
			{
				intFactors[j]=-1.f;
			}
			else
			{	
				tmp=glm::dot(planePoint-a, cutNormal)/tmp;
				intFactors[j]=tmp;
			}
			j++;
		}
		
		if((intFactors[0]<=0.f || 1.0f<=intFactors[0]) && (intFactors[1]<=0.f || 1.0f<=intFactors[1]) && (intFactors[2]<=0.f || 1.0f<=intFactors[2]))
			return false;
		return true;
	}

	//In case a triangle is not intersected by the cutting plane, then it must belong totally to the positive or negative part.
	//This procedure add the points of a triangle that doesn't intersect the cutting plane, to the given data structures of positive or negative mesh.
	void AddExistingTriangle(vector<Vertex> triangleVertices, unordered_map<Vertex, vector<int>> & vertexIndexMap, vector<GLuint> & ind, vector<Vertex> & ver, glm::vec3 & centroid, float & totalArea)
	{			
		int index=0;
		for(int i=0;i<3;i++)
		{
			auto it=vertexIndexMap.find(triangleVertices[i]);
			if(it==vertexIndexMap.end())
			{
				ver.push_back(triangleVertices[i]);
				index=ver.size()-1;
				ind.push_back(index);
				vertexIndexMap[triangleVertices[i]].push_back(index);
			}
			else
			{
				bool found=false;
				vector<int> temp=vertexIndexMap[triangleVertices[i]];
				for(unsigned int j=0;j<temp.size();j++)
				{
					if(ver[temp[j]].Equals(triangleVertices[i]))
					{
						found=true;
						ind.push_back(temp[j]);
						break;
					}
				}
				if(!found)
				{
					ver.push_back(triangleVertices[i]);
					index=ver.size()-1;
					ind.push_back(index);
					vertexIndexMap[triangleVertices[i]].push_back(index);
				}
			}
		}
		
		float triangleArea=CalculateTriangleArea(triangleVertices[0].Position, triangleVertices[1].Position, triangleVertices[2].Position);
		totalArea+=triangleArea;
		glm::vec3 triangleCenter=CalculateTriangleCenter(triangleVertices[0].Position, triangleVertices[1].Position, triangleVertices[2].Position);
		centroid+=(triangleArea*triangleCenter);
	}
	//This procedure cuts the mesh in two parts: positive and negative; these new meshes are saved in positiveMesh and negativeMesh.
	//Since physics simulation does not deal with directly with these meshes, we need also to generate some sort of bounding volume, to describe them to the physics engine.
	//that's why each produced mesh is also paired with a convex hull.
	//After the call of this method, the mesh involved in the cut must be removed from the scene, in order to maintain the scene consistent.
	void Cut(Mesh & positiveMesh, Mesh & negativeMesh, glm::vec4 & positiveMeshPosition, glm::vec4 & negativeMeshPosition, glm::vec4 cutStartPoint, glm::vec4 cutEndPoint, glm::mat4 model, btConvexHullShape* & positiveShape, btConvexHullShape* & negativeShape, float & positiveWeightFactor, float & negativeWeightFactor)
	{
		//Convert world vertices to object space
		glm::mat4 invModel=glm::inverse(model);
		cutStartPoint=invModel*cutStartPoint;
		cutEndPoint=invModel*cutEndPoint;
		//The calculate the normal in object space
		glm::vec4 cutVector=glm::vec4(cutEndPoint.x-cutStartPoint.x, cutEndPoint.y-cutStartPoint.y, 0, cutEndPoint.w-cutStartPoint.w);
		glm::vec4 cutNormal=glm::vec4(-cutVector.y, cutVector.x, 0.0f, 0.0f);
		cutNormal=glm::normalize(cutNormal);
		float negativeArea;
		vector<Vertex> negativeMeshVertices;
		vector<GLuint> negativeMeshIndices;
		float positiveArea;
		vector<Vertex> positiveMeshVertices;
		vector<GLuint> positiveMeshIndices;
		glm::vec3 positiveMeshCentroid;
		glm::vec3 negativeMeshCentroid;
		float intFactors[3]={0.f, 0.f, 0.f};
		unordered_map<Vertex, vector<int>> positiveVertexIndexMap;
		unordered_map<Vertex, vector<int>> negativeVertexIndexMap;
		unordered_map<Vertex, int> positiveSectionVertexIndexMap;
		unordered_map<Vertex, int> negativeSectionVertexIndexMap;
		Vertex sectionVertexCentroid=Vertex();
		positiveShape=new btConvexHullShape();
		negativeShape=new btConvexHullShape();
		
		positiveMeshVertices.push_back(sectionVertexCentroid);
		negativeMeshVertices.push_back(sectionVertexCentroid);

		Log log=Log();
		log.InitLog("Cut");
		for(unsigned int i=0;i<indices.size();i+=3)
		{
			int indA=indices[i];
			int indB=indices[i+1];
			int indC=indices[i+2];
			Vertex a=Vertex(glm::vec3(vertices[indA].Position.x, vertices[indA].Position.y, vertices[indA].Position.z), 
							glm::vec3(vertices[indA].Normal.x, vertices[indA].Normal.y, vertices[indA].Normal.z),
							glm::vec2(vertices[indA].TexCoords.x, vertices[indA].TexCoords.y),
							glm::vec3(vertices[indA].Tangent.x, vertices[indA].Tangent.y, vertices[indA].Tangent.z),
							glm::vec3(vertices[indA].Bitangent.x, vertices[indA].Bitangent.y, vertices[indA].Bitangent.z));
			Vertex b=Vertex(glm::vec3(vertices[indB].Position.x, vertices[indB].Position.y, vertices[indB].Position.z), 
							glm::vec3(vertices[indB].Normal.x, vertices[indB].Normal.y, vertices[indB].Normal.z),
							glm::vec2(vertices[indB].TexCoords.x, vertices[indB].TexCoords.y),
							glm::vec3(vertices[indB].Tangent.x, vertices[indB].Tangent.y, vertices[indB].Tangent.z),
							glm::vec3(vertices[indB].Bitangent.x, vertices[indB].Bitangent.y, vertices[indB].Bitangent.z));
			Vertex c=Vertex(glm::vec3(vertices[indC].Position.x, vertices[indC].Position.y, vertices[indC].Position.z), 
							glm::vec3(vertices[indC].Normal.x, vertices[indC].Normal.y, vertices[indC].Normal.z),
							glm::vec2(vertices[indC].TexCoords.x, vertices[indC].TexCoords.y),
							glm::vec3(vertices[indC].Tangent.x, vertices[indC].Tangent.y, vertices[indC].Tangent.z),
							glm::vec3(vertices[indC].Bitangent.x, vertices[indC].Bitangent.y, vertices[indC].Bitangent.z));
			vector<Vertex> triangleVertices={a, b, c};
			if(CutTriangle(intFactors, triangleVertices, cutEndPoint, cutNormal))
			{	
				vector<Vertex> newVertices;
				newVertices=CalculateNewVertices(a, b, c, cutNormal, cutEndPoint, intFactors);
				
				AddNewTriangle(	cutEndPoint, 
								cutNormal, 
								sectionVertexCentroid,
								negativeSectionVertexIndexMap,
								negativeVertexIndexMap,
								negativeMeshVertices, 
								negativeMeshIndices, 
								negativeMeshCentroid,
								negativeArea, 
								positiveSectionVertexIndexMap,
								positiveVertexIndexMap,
								positiveMeshVertices, 
								positiveMeshIndices, 
								positiveMeshCentroid,
								positiveArea, 
								newVertices,
								triangleVertices);
			}
			else
			{
				//Since there can be situation in which a point lies over the plane,
				//this code assign the triangle to the side, where the majority of points are.
				int counter=a.PositiveOrNegativeSide(cutNormal, cutEndPoint)>.0? 1 : -1;
				counter=b.PositiveOrNegativeSide(cutNormal, cutEndPoint)>.0? counter+1 : counter-1;
				counter=c.PositiveOrNegativeSide(cutNormal, cutEndPoint)>.0? counter+1 : counter-1;
				if(counter>0.f)
					AddExistingTriangle(triangleVertices, positiveVertexIndexMap, positiveMeshIndices, positiveMeshVertices, positiveMeshCentroid, positiveArea);
				else
					AddExistingTriangle(triangleVertices, negativeVertexIndexMap, negativeMeshIndices, negativeMeshVertices, negativeMeshCentroid, negativeArea);
			}
		}
		
		log.EndLog();
		
		sectionVertexCentroid.Position/=positiveSectionVertexIndexMap.size();
		
		positiveMeshVertices[0].Position.x=sectionVertexCentroid.Position.x;
		positiveMeshVertices[0].Position.y=sectionVertexCentroid.Position.y;
		positiveMeshVertices[0].Position.z=sectionVertexCentroid.Position.z;
		
		positiveMeshVertices[0].Normal.x=-cutNormal.x;
		positiveMeshVertices[0].Normal.y=-cutNormal.y;
		positiveMeshVertices[0].Normal.z=-cutNormal.z;
		
		negativeMeshVertices[0].Position.x=sectionVertexCentroid.Position.x;
		negativeMeshVertices[0].Position.y=sectionVertexCentroid.Position.y;
		negativeMeshVertices[0].Position.z=sectionVertexCentroid.Position.z;
		
		negativeMeshVertices[0].Normal.x=cutNormal.x;
		negativeMeshVertices[0].Normal.y=cutNormal.y;
		negativeMeshVertices[0].Normal.z=cutNormal.z;
		
		float epsilon=0.09f;
		if(positiveArea<=epsilon)
			positiveArea=1;
		else
			positiveMeshCentroid/=positiveArea;
			
		if(negativeArea<=epsilon)
			negativeArea=1;
		else
			negativeMeshCentroid/=negativeArea;
		
		positiveWeightFactor=positiveArea/(positiveArea+negativeArea);
		negativeWeightFactor=1-positiveWeightFactor;
		
		if(positiveWeightFactor<=0)
			positiveWeightFactor=1.f;
		if(negativeWeightFactor<=0)
			negativeWeightFactor=1.f;
			
		log.InitLog("Convex hull generation");
		positiveVertexIndexMap.clear();
		negativeVertexIndexMap.clear();
		
		for(unsigned int i=0;i<positiveMeshVertices.size();i++)
		{
			positiveMeshVertices[i].Position.x-=positiveMeshCentroid.x;
			positiveMeshVertices[i].Position.y-=positiveMeshCentroid.y;
			positiveMeshVertices[i].Position.z-=positiveMeshCentroid.z;
			
			auto it=positiveVertexIndexMap.find(positiveMeshVertices[i]);
			if(it==positiveVertexIndexMap.end())
			{
				positiveShape->addPoint(btVector3(positiveMeshVertices[i].Position.x, positiveMeshVertices[i].Position.y, positiveMeshVertices[i].Position.z));
				positiveVertexIndexMap[positiveMeshVertices[i]].push_back(0);
			}
		}
		
		for(unsigned int i=0;i<negativeMeshVertices.size();i++)
		{
			negativeMeshVertices[i].Position.x-=negativeMeshCentroid.x;
			negativeMeshVertices[i].Position.y-=negativeMeshCentroid.y;
			negativeMeshVertices[i].Position.z-=negativeMeshCentroid.z;
			
			auto it=negativeVertexIndexMap.find(negativeMeshVertices[i]);
			if(it==negativeVertexIndexMap.end())
			{
				negativeShape->addPoint(btVector3(negativeMeshVertices[i].Position.x, negativeMeshVertices[i].Position.y, negativeMeshVertices[i].Position.z));
				negativeVertexIndexMap[negativeMeshVertices[i]].push_back(0);
			}
		}
		
		log.EndLog();
		positiveMesh=Mesh(positiveMeshVertices, positiveMeshIndices, textures);
		negativeMesh=Mesh(negativeMeshVertices, negativeMeshIndices, textures);

		positiveMeshPosition=model*glm::vec4(positiveMeshCentroid.x, positiveMeshCentroid.y, positiveMeshCentroid.z, 1.0f);
		negativeMeshPosition=model*glm::vec4(negativeMeshCentroid.x, negativeMeshCentroid.y, negativeMeshCentroid.z, 1.0f);
	}

    void Draw(Shader shader)
    {
        // Bind appropriate textures
        GLuint diffuseNr = 1;
        GLuint specularNr = 1;
        GLuint normalNr = 1;
        GLuint heightNr = 1;
        for(GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            stringstream ss;
            string number;
            string name = this->textures[i].type;
            if(name == "texture_diffuse")
                ss << diffuseNr++;
            else if(name == "texture_specular")
                ss << specularNr++;
            else if(name == "texture_normal")
                ss << normalNr++;
             else if(name == "texture_height")
                ss << heightNr++;
            number = ss.str();
			
            glUniform1i(glGetUniformLocation(shader.Program, (name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
        }

        glBindVertexArray(this->VAO);
        glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
	
    void Delete()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
		vertices.clear();
		indices.clear();
		textures.clear();
    }

private:
  GLuint VBO, EBO;
  void setupMesh()
  {
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