///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/******************************************************************/
/*  DefineObjectMaterials()										***/
/*																***/
/*  This method is used for configuring the various material	***/
/*  settings for all of the objects within the 3D scene.		***/
/*****************************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Metal material for the metal casing of the pencil
	// mild ambient color with high reflectiveness
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.ambientStrength = 0.3f;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.shininess = 10.0;
	metalMaterial.tag = "metalMaterial";

	m_objectMaterials.push_back(metalMaterial);

	// Material for the wood in the pencil tip
	// low reclectivity and shininess
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "woodMaterial";

	m_objectMaterials.push_back(woodMaterial);

	// Material for plastic used in the blue sleeved cards
	// Matte material with a higher shininess
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.3f);
	plasticMaterial.ambientStrength = 0.4f;
	plasticMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.8f);
	plasticMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	plasticMaterial.shininess = 6.0f;
	plasticMaterial.tag = "plasticMaterial";

	m_objectMaterials.push_back(plasticMaterial);


	// Material for the face up trading card
	// Matte material look with a slight shininess for the cards finish
	OBJECT_MATERIAL cardMaterial;
	cardMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cardMaterial.ambientStrength = 0.4f;
	cardMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	cardMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cardMaterial.shininess = 0.1f;
	cardMaterial.tag = "cardMaterial";

	m_objectMaterials.push_back(cardMaterial);

	// Material for fabric used in the scene for the playmat
	// Low ambient collor with no shininess
	OBJECT_MATERIAL fabricMaterial;
	fabricMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); 
	fabricMaterial.ambientStrength = 0.3f;
	fabricMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	fabricMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	fabricMaterial.shininess = 0.0f;
	fabricMaterial.tag = "fabricMaterial";

	m_objectMaterials.push_back(fabricMaterial);

	// Material for the glossy lacquered outside of a pencil
	// Bright yellow with a strong shininess for the glossy look of a pencil
	OBJECT_MATERIAL glossyPencilMaterial;
	glossyPencilMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.0f);  
	glossyPencilMaterial.ambientStrength = 0.5f; 
	glossyPencilMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.1f); 
	glossyPencilMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); 
	glossyPencilMaterial.shininess = 2.0f; 
	glossyPencilMaterial.tag = "glossyPencilMaterial";

	m_objectMaterials.push_back(glossyPencilMaterial);

	// Material for pencil lead
	// Dark gray with no shininess
	OBJECT_MATERIAL pencilLeadMaterial;
	pencilLeadMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	pencilLeadMaterial.ambientStrength = 0.2f; 
	pencilLeadMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); 
	pencilLeadMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); 
	pencilLeadMaterial.shininess = 0.0f;
	pencilLeadMaterial.tag = "pencilLeadMaterial";

	m_objectMaterials.push_back(pencilLeadMaterial);

	// Material for pink rubber eraser
	// Pink with a low shininess
	OBJECT_MATERIAL pinkEraserMaterial;
	pinkEraserMaterial.ambientColor = glm::vec3(0.5f, 0.2f, 0.3f); 
	pinkEraserMaterial.ambientStrength = 0.3f; 
	pinkEraserMaterial.diffuseColor = glm::vec3(0.3f, 0.15f, 0.1f); 
	pinkEraserMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	pinkEraserMaterial.shininess = 2.0f;
	pinkEraserMaterial.tag = "pinkEraserMaterial";

	m_objectMaterials.push_back(pinkEraserMaterial);

	// Material for marble used for the dice
	// Matte material with a higher shininess
	OBJECT_MATERIAL marbleMaterial;
	marbleMaterial.ambientColor = glm::vec3(0.1f, 0.3f, 0.1f);
	marbleMaterial.ambientStrength = 0.4f;
	marbleMaterial.diffuseColor = glm::vec3(0.1f, 0.3f, 0.1f);
	marbleMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	marbleMaterial.shininess = 6.0f;
	marbleMaterial.tag = "marbleMaterial";

	m_objectMaterials.push_back(marbleMaterial);

}


/******************************************************************/
/*  SetupSceneLights()											***/
/*																***/
/*  This method is called to add and configure the light		***/
/*  sources for the 3D scene.  There are up to 4 light sources.	***/
/******************************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Lightbulb to the west
	m_pShaderManager->setVec3Value("lightSources[0].position", -30.0f, 14.0f, -2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.3f, 0.3f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.6f, 0.5f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.4f);

	// Sunlight to the north
	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 20.0f, -26.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.6f, 0.55f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f); 
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.6f);


}


/******************************************************************/
/*  LoadSceneTextures()											***/
/*																***/
/*  This method is used for preparing the 3D scene by loading	***/
/*  the shapes, textures in memory to support the 3D scene		***/
/*  rendering													***/
/******************************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/


	bool bReturn = false;

	bReturn = CreateGLTexture(
		"Textures/Eevee_playmat_texture.png", "playmat");
	bReturn = CreateGLTexture(
		"Textures/Wood_texture.jpg", "wood");
	bReturn = CreateGLTexture(
		"Textures/Pencil_cylinder_texture.png", "pencilCylinder");
	bReturn = CreateGLTexture(
		"Textures/Metal_grate_texture.jpg", "metal");
	bReturn = CreateGLTexture(
		"Textures/Plains_texture.png", "plains");
	bReturn = CreateGLTexture(
		"Textures/Plastic_texture.jpg", "plastic");
	bReturn = CreateGLTexture(
		"Textures/Lead_texture.jpg", "lead");
	bReturn = CreateGLTexture(
		"Textures/Layered_cards_texture.png", "deck");
	bReturn = CreateGLTexture(
		"Textures/Rubber_texture.jpg", "rubber");
	bReturn = CreateGLTexture(
		"Textures/Marble_texture.jpg", "marble");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();

	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh(); 
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPyramid3Mesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderPencil();
	RenderCards();
	RenderDice();
}


/******************************************************************/
/*  RenderPencil()												***/
/*																***/
/*  This method is called to render the pencil in the scene		***/
/******************************************************************/
void SceneManager::RenderPencil()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/

	/******************************************************************/
	// Code for playmat below										***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(15.0f, 1.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, .0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set texture to playmat
	SetShaderTexture("playmat");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("fabricMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	//***Following are sections for drawing the pencil				
	// split into a tapered cone and cone for the tip				
	//***Cylinder for the main yellow wood, and two cylinders for the eraser
	/******************************************************************/
	// Code for main pencil cylinder								***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 7.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 0.15f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the cylinder
	SetShaderTexture("pencilCylinder");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glossyPencilMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	/******************************************************************/
	// Code for pencil metal cylinder								***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.155f, 0.35f, 0.155f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;

	// set the XYZ position for the mesh

	positionXYZ = glm::vec3(5.6f, 0.15f, 6.33f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture for the metal cylinder
	SetShaderTexture("metal");
	SetTextureUVScale(0.7f, 0.7f);
	SetShaderMaterial("metalMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	/******************************************************************/
	// Code for pencil eraser cylinder								***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.50f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 70.0f;

	// set the XYZ position for the mesh

	positionXYZ = glm::vec3(5.6f, 0.15f, 6.33f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("rubber");
	SetShaderMaterial("pinkEraserMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	/******************************************************************/
	// Code for pencil tip tapered cylinder							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.42f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -110.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 0.15f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to wood
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("woodMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();



	/******************************************************************/
	// Code for pencil led											***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.145f, 0.75f, 0.145f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -110.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.0f, 0.15f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("lead");
	SetShaderMaterial("pencilLeadMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
}



/******************************************************************/
/* RenderCards()												***/
/*																***/
/* This method is called to render the cards in the scene		***/
/******************************************************************/
void SceneManager::RenderCards()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/



	/******************************************************************/
	/*** Deck of cards Code											***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 2.0f, 4.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 5.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 1.0f, 2.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to the deck of cards
	SetShaderTexture("deck");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plasticMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();



	/******************************************************************/
	/*** Top of deck of cards code									***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 0.02f, 4.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 5.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 2.01f, 2.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to plastic
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plasticMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	
	/******************************************************************/
	/*** Plains card Code											***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.75f, 0.0f, 2.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.5f, 0.05f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to Plains
	SetShaderTexture("plains");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cardMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	/******************************************************************/
	/*** Blue card Code												***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.75f, 0.0f, 2.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.25f, 0.02f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to plastic
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plasticMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	/******************************************************************/
	/*** Blue card Code (top of stack)								***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.75f, 0.0f, 2.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.3f, 0.01f, 3.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to plastic
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plasticMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	/******************************************************************/
	/*** Blue card Code (Middle of stack)							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.75f, 0.0f, 2.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -22.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.3f, 0.005f, 3.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to plastic
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plasticMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	/******************************************************************/
	/*** Blue card Code (bottom of stack)							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.75f, 0.0f, 2.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -22.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.3f, 0.005f, 3.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to plastic
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plasticMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}



/******************************************************************/
/*  RenderDice()												***/
/*																***/
/*  This method is called to render the dice in the scene		***/
/******************************************************************/
void SceneManager::RenderDice()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/



	/******************************************************************/
	/*** Pyramid dice code											***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.1f, 0.4f, -0.64f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to the deck of cards
	SetShaderTexture("marble");
	SetTextureUVScale(1.1f, 1.1f);
	SetShaderMaterial("marbleMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid3Mesh();


	/******************************************************************/
	/*** Cube dice code												***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.4f, 0.4f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Set the texture to the deck of cards
	SetShaderTexture("marble");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("marbleMaterial");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
