#include <iostream>
#include <fstream>
#include <string>

#include <math.h>

using namespace std;

#include "geometry.h"

// NOTE: The WaveFront OBJ format spec, states that meshes are allowed to be defined by faces
//       consisting of 3 or more vertices. For the purposes of this loader (and since this is the
//       most common case) we only support triangle faces (defined by exactly 3 vertices) and as a
//       result, the loader will ignore any data after the third vertex for each face
//
//       Similarly, the spec allows for vertex positions and texture coordinates to both have a
//       w-coordinate. The loader will ignore these and assumes that all vertex specifications contain
//       exactly 3 values, and that all texture coordinate specifications contain exactly 2 values


// NOTE: There is currently no support for mtl material references or anything like that,
//       just load whatever texture you want to use manually

enum OBJDataType
{
	NONE,
	VERTEX,
	TEXTURECOORD,
	NORMAL,
	FACE,
	COMMENT
};

void GeometryData::loadFromOBJFile(string filename)
{
	GeometryData tempGeom;

	ifstream inStream;
	inStream.open(filename, ifstream::in);
	if (inStream.fail())
	{
		cout << "Unable to open obj file: " << filename << endl;
		return;
	}
	bool firstLoop = true;
	OBJDataType currentDataType = NONE;
	while (!inStream.eof())
	{
		switch (currentDataType)
		{
		case NONE:
		{
			inStream >> ws;
			char typeChar1 = inStream.get();
			if (inStream.eof())
			{
				break;
			}
			char typeChar2 = inStream.get();
			if (typeChar1 == '#')
			{
				currentDataType = COMMENT;
			}
			else if (typeChar1 == 'f')
			{
				currentDataType = FACE;
			}
			else if (typeChar1 != 'v')
			{
				cout << "OBJ parse error: Expected 'v', 'f' or '#' at the start of the line" << endl;
				cout << "Found: " << typeChar1 << typeChar2 << endl;
				currentDataType = COMMENT;
			}
			else
			{
				if ((typeChar2 == ' ') || (typeChar2 == '\t'))
				{
					currentDataType = VERTEX;
				}
				else if (typeChar2 == 't')
				{
					currentDataType = TEXTURECOORD;
				}
				else if (typeChar2 == 'n')
				{
					currentDataType = NORMAL;
				}
				else if (typeChar2 == 'p')
				{
					cout << "OBJ parse error: Free-form geometry is not supported, ignoring" << endl;
					currentDataType = COMMENT;
				}
				else
				{
					cout << "Unsupported data entry v" << (char)typeChar2 << ", ignoring" << endl;
					currentDataType = COMMENT;
				}
			}

		} break;

		case VERTEX:
		{
			float x;
			float y;
			float z;
			inStream >> x >> y >> z;

			if (firstLoop) {
				firstLoop = false;
				minX = x;
				maxX = x;
				minY = y;
				maxY = y;
				minZ = z;
				maxZ = z;
			}
			else {
				//check x, y, z for bounding box
				if (x < minX) minX = x;
				else if (x > maxX) maxX = x;
				if (y < minY) minY = y;
				else if (y > maxY) maxY = y;
				if (z < minZ) minZ = z;
				else if (z > maxZ) maxZ = z;
			}

			tempGeom.vertices.push_back(x);
			tempGeom.vertices.push_back(y);
			tempGeom.vertices.push_back(z);
			currentDataType = COMMENT;


		} break;

		case TEXTURECOORD:
		{
			float u;
			float v;
			inStream >> u >> v;
			tempGeom.textureCoords.push_back(u);
			tempGeom.textureCoords.push_back(v);
			currentDataType = COMMENT;
		} break;

		case NORMAL:
		{
			float x;
			float y;
			float z;
			inStream >> x >> y >> z;
			tempGeom.normals.push_back(x);
			tempGeom.normals.push_back(y);
			tempGeom.normals.push_back(z);
			currentDataType = COMMENT;
		} break;

		case FACE:
		{
			// NOTE: Here is where we assume that exactly 3 vertices are used to specify a face
			int vertIndex = 0;
			int texCoordIndex = 0;
			int normalIndex = 0;

			FaceData face = {};
			for (int index = 0; index<3; index++)
			{
				inStream >> vertIndex;
				char postVertexCheckChar = inStream.get();
				if (postVertexCheckChar == '/')
				{
					char texCoordExistCheckChar = inStream.get();
					inStream.unget();
					if (texCoordExistCheckChar != '/')
					{
						inStream >> texCoordIndex;
					}

					char postTexCoordCheckChar = inStream.get();
					if (postTexCoordCheckChar == '/')
					{
						inStream >> normalIndex;
					}
					else
					{
						inStream.unget(); // This is just to prevent us from consuming a newline
					}
				}

				// NOTE: We subtract 1 here because the OBJ format uses 1-based indices
				face.vertexIndex[index] = vertIndex - 1;
				face.texCoordIndex[index] = texCoordIndex - 1;
				face.normalIndex[index] = normalIndex - 1;
			}
			tempGeom.faces.push_back(face);
			currentDataType = COMMENT;
		} break;

		case COMMENT:
		{
			int nextChar = inStream.get();
			if (nextChar == '\n')
			{
				currentDataType = NONE;
			}
		} break;

		default:
		{}
		}
	}


	// NOTE: Since our rendering pipeline supports only 1 set of indices for our data, we need to
	//       do some post-processing here in order to lay out all the unique v/vt/vn triples
	// TODO: We're currently just assuming all the triples are distinct, but its probably worth doing
	//       at least a little bit of checking on that front
	// TODO: We're deciding whether or not to add texture coords and normals on a per-face basis,
	//       which doesn't really make sense because if there are any then there should be for all
	//       vertices, but this way that might not be the case

	for (int faceIndex = 0; faceIndex<tempGeom.faces.size(); faceIndex++)
	{
		FaceData face = tempGeom.faces[faceIndex];
		bool hasTextureCoords = (face.texCoordIndex[0] >= 0);
		bool hasNormals = (face.normalIndex[0] >= 0);
		for (int vertIndex = 0; vertIndex<3; vertIndex++)
		{
			for (int i = 0; i<3; i++)
			{
				vertices.push_back(tempGeom.vertices[(3 * face.vertexIndex[vertIndex]) + i]);
			}

			if (hasTextureCoords)
			{
				for (int i = 0; i<2; i++)
				{
					textureCoords.push_back(
						tempGeom.textureCoords[(2 * face.texCoordIndex[vertIndex]) + i]);
				}
			}
			if (hasNormals)
			{
				for (int i = 0; i<3; i++)
				{
					normals.push_back(tempGeom.normals[(3 * face.normalIndex[vertIndex]) + i]);
				}
			}
		}

		// Compute the (bi)tangent for the face, and add it for each vertex
		if (hasTextureCoords && hasNormals)
		{
			int vertexStartIndex = vertices.size() - 9;
			int uvStartIndex = textureCoords.size() - 6;
			float* vertices = &this->vertices[vertexStartIndex];
			float* texCoords = &textureCoords[uvStartIndex];

			float deltaX1 = vertices[3] - vertices[0];
			float deltaY1 = vertices[4] - vertices[1];
			float deltaZ1 = vertices[5] - vertices[2];
			float deltaX2 = vertices[6] - vertices[0];
			float deltaY2 = vertices[7] - vertices[1];
			float deltaZ2 = vertices[8] - vertices[2];

			float deltaU1 = texCoords[2] - texCoords[0];
			float deltaV1 = texCoords[3] - texCoords[1];
			float deltaU2 = texCoords[4] - texCoords[0];
			float deltaV2 = texCoords[5] - texCoords[1];

			float inverseDet = 1.0f / (deltaU1*deltaV2 - deltaU2 * deltaV1);

			float tangentX = inverseDet * (deltaV2*deltaX1 - deltaV1 * deltaX2);
			float tangentY = inverseDet * (deltaV2*deltaY1 - deltaV1 * deltaY2);
			float tangentZ = inverseDet * (deltaV2*deltaZ1 - deltaV1 * deltaZ2);

			float bitangentX = inverseDet * (deltaU1*deltaX2 - deltaU2 * deltaX1);
			float bitangentY = inverseDet * (deltaU1*deltaY2 - deltaU2 * deltaY1);
			float bitangentZ = inverseDet * (deltaU1*deltaZ2 - deltaU2 * deltaZ1);

			float tangentLength = sqrt(tangentX*tangentX +
				tangentY * tangentY +
				tangentZ * tangentZ);
			float bitangentLength = sqrt(bitangentX*bitangentX +
				bitangentY * bitangentY +
				bitangentZ * bitangentZ);

			tangentX /= tangentLength;
			tangentY /= tangentLength;
			tangentZ /= tangentLength;
			bitangentX /= bitangentLength;
			bitangentY /= bitangentLength;
			bitangentZ /= bitangentLength;

			// NOTE: Each vertex in the face gets the same (bi)tangent pair
			for (int vertIndex = 0; vertIndex<3; vertIndex++)
			{
				tangents.push_back(tangentX);
				tangents.push_back(tangentY);
				tangents.push_back(tangentZ);

				bitangents.push_back(bitangentX);
				bitangents.push_back(bitangentY);
				bitangents.push_back(bitangentZ);
			}
		}
	}

	cout << "Successfully loaded an OBJ with " << vertices.size() / 3 << " vertices " << endl;

}

int GeometryData::vertexCount()
{
	return vertices.size() / 3;
}

void* GeometryData::vertexData()
{
	return (void*)&vertices[0];
}

void* GeometryData::textureCoordData()
{
	return (void*)&textureCoords[0];
}

void* GeometryData::normalData()
{
	return (void*)&normals[0];
}

void* GeometryData::tangentData()
{
	return (void*)&tangents[0];
}

void* GeometryData::bitangentData()
{
	return (void*)&bitangents[0];
}

//for bounding boxes
float GeometryData::getMinX() const{
	return minX;
}

float GeometryData::getMinY() const {
	return minY;
}

float GeometryData::getMinZ() const {
	return minZ;
}

float GeometryData::getMaxX() const {
	return maxX;
}

float GeometryData::getMaxY() const {
	return maxY;
}

float GeometryData::getMaxZ() const {
	return maxZ;
}

float GeometryData::getCentreX() const {
	return -(maxX + minX) / 2;
}

float GeometryData::getCentreY() const {
	return -(maxY+minY)/2;
}

float GeometryData::getCentreZ() const {
	return -(maxZ + minZ) / 2;
}
