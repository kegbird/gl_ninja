#pragma once
#include<glm/glm.hpp>
#include<unordered_map>

class HalfEdge;
class Vertex {
public:
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
	
	Vertex(){}

    Vertex(glm::vec3 Position, glm::vec3 Normal, glm::vec2 TexCoords, glm::vec3 Tangent, glm::vec3 Bitangent)
    {
        this->Position=Position;
        this->Normal=Normal;
        this->TexCoords=TexCoords;
        this->Tangent=Tangent;
        this->Bitangent=Bitangent;
    }

	float PositiveOrNegativeSide(glm::vec3 planeNormal, glm::vec3 planePoint)
	{
		glm::vec3 vertexToCutPlane=glm::vec3(Position.x-planePoint.x, Position.y-planePoint.y, 0);
		return glm::dot(planeNormal, vertexToCutPlane);
	}
	
  	bool Equals(Vertex other)
  	{
		float epsilon=0.001f;
		glm::vec3 otherPosition=other.Position;
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
	
	
	bool operator==(const Vertex& other) const
    {
		float epsilon=0.001f;
		float deltaX=fabs(Position.x-other.Position.x);
		float deltaY=fabs(Position.y-other.Position.y);
		float deltaZ=fabs(Position.z-other.Position.z);
		return (deltaX<=epsilon) && (deltaY<=epsilon) && (deltaZ<=epsilon);
    }
};
