#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <string>

struct FaceData
{
	int vertexIndex[3];
	int texCoordIndex[3];
	int normalIndex[3];
};

class GeometryData
{
public:
	void loadFromOBJFile(std::string filename);

	int vertexCount();

	void* vertexData();
	void* textureCoordData();
	void* normalData();
	void* tangentData();
	void* bitangentData();

	//for bounding box
	float getMinX() const;
	float getMinY() const;
	float getMinZ() const;
	float getMaxX() const;
	float getMaxY() const;
	float getMaxZ() const;

	float getCentreX() const;
	float getCentreY() const;
	float getCentreZ() const;

private:
	std::vector<float> vertices;
	std::vector<float> textureCoords;
	std::vector<float> normals;
	std::vector<float> tangents;
	std::vector<float> bitangents;

	std::vector<FaceData> faces;

	//bounding box
	float minX, minY, minZ, maxX, maxY, maxZ;

};

#endif
