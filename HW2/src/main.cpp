#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define pi 3.1415926
#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

int win_H = 800;
int win_W = 800;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
	Light_editing = 6,
	Shine_editing = 7
};

enum LightMode {
	directional = 0,
	positional = 1,
	spot = 2
};

GLint iLocMVP;
GLint iLocMV;

GLint iLocshader;
GLint iLocLMode;
GLint iLocShininess;
GLint iLocCamPos;
GLint shader = 0;
struct iLocLight {
	GLint position;
	GLint direction;
	GLint exponent;
	GLint cutoff;
	GLint iLocDiffuse;
	GLint iLocAmbient;
};
iLocLight iLocLights;
struct Light {
	Vector3 position;
	Vector3 direction;
	GLfloat exponent;
	GLfloat cutoff;
	Vector3 diffuse;
	Vector3 ambient;
};
Light Lights[3];
Light* main_light;

GLfloat shininess;

vector<string> filenames; // .obj filename list
struct iLocPhong {
	GLint Ka;
	GLint Kd;
	GLint Ks;
};
iLocPhong iLocPhongs;
struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;
LightMode cur_light_mode = directional;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1);


	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1);


	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	GLfloat Rx[3], Ry[3], Rz[3];
	GLfloat forward[3] = { (main_camera.center[0] - main_camera.position[0]),
						   (main_camera.center[1] - main_camera.position[1]),
						   (main_camera.center[2] - main_camera.position[2]) };

	GLfloat right[3] = { 0.0,0.0,0.0 };
	GLfloat up[3] = { main_camera.up_vector[0],
					  main_camera.up_vector[1],
					  main_camera.up_vector[2] };

	Rz[0] = -forward[0]; Rz[1] = -forward[1]; Rz[2] = -forward[2];

	Normalize(Rz);

	Cross(forward, up, right);
	Rx[0] = right[0]; Rx[1] = right[1]; Rx[2] = right[2];
	Normalize(Rx);

	Cross(Rz, Rx, Ry);

	Matrix4 R = Matrix4(Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1);
	Matrix4 T = Matrix4(1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1);

	view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	project_matrix = Matrix4(2 / (proj.right - proj.left), 0, 0, -((proj.right + proj.left) / (proj.right - proj.left)),
		0, 2 / (proj.top - proj.bottom), 0, -((proj.top + proj.bottom) / (proj.top - proj.bottom)),
		0, 0, 2 / -(proj.farClip - proj.nearClip), -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip)),
		0, 0, 0, 1);
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
	GLfloat f = abs(1 / tan(proj.fovy / 2));
	project_matrix = Matrix4(f / proj.aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, ((proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip)), 2 * proj.nearClip * proj.farClip / (proj.nearClip - proj.farClip),
		0, 0, -1, 0);
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{	
	win_H = height; win_W = width;
	glViewport(0, 0, width / 2, height);
	glViewport(width / 2, 0, width / 2, height);
	// [TODO] change your aspect ratio in both perspective and orthogonal view

	proj.aspect = (GLfloat)width / height/2;

	if (cur_proj_mode == Perspective) {
		setPerspective();
	}
	else if (cur_proj_mode == Orthogonal) {
		setOrthogonal();
	}
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];
	Matrix4 MV;
	// [TODO] multiply all the matrix
	// [TODO] row-major ---> column-major

	MVP = project_matrix * view_matrix * T * R * S;
	MV = T * R * S;

	mvp[0] = MVP[0]; mvp[4] = MVP[1]; mvp[8] = MVP[2]; mvp[12] = MVP[3];
	mvp[1] = MVP[4]; mvp[5] = MVP[5]; mvp[9] = MVP[6]; mvp[13] = MVP[7];
	mvp[2] = MVP[8]; mvp[6] = MVP[9]; mvp[10] = MVP[10]; mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13]; mvp[11] = MVP[14]; mvp[15] = MVP[15];

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_TRUE, &MVP[0]);
	glUniformMatrix4fv(iLocMV, 1, GL_TRUE, &MV[0]);
	
	glUniform3fv(iLocLights.position, 1, &(main_light->position[0]));
	//glUniform3fv(iLocLights.direction, 1, &(main_light->direction[0]));
	glUniform1f(iLocLights.exponent, main_light->exponent);
	glUniform1f(iLocLights.cutoff, (main_light->cutoff)*pi / 180);
	glUniform3fv(iLocLights.iLocDiffuse, 1, &(main_light->diffuse[0]));
	glUniform3fv(iLocLights.iLocAmbient, 1, &(main_light->ambient[0]));
	glUniform1f(iLocShininess, shininess);
	glUniform1i(iLocLMode, cur_light_mode);
	glUniform3fv(iLocCamPos, 1, &main_camera.position[0]);

	glUniform1i(iLocshader, 0);
	glViewport(0, 0, win_W / 2, win_H);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glUniform3fv(iLocPhongs.Ka, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
		glUniform3fv(iLocPhongs.Kd, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
		glUniform3fv(iLocPhongs.Ks, 1, &(models[cur_idx].shapes[i].material.Ks[0]));
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

	glUniform1i(iLocshader, 1);
	glViewport(win_W / 2, 0, win_W / 2, win_H);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glUniform3fv(iLocPhongs.Ka, 1, &(models[cur_idx].shapes[i].material.Ka[0]));
		glUniform3fv(iLocPhongs.Kd, 1, &(models[cur_idx].shapes[i].material.Kd[0]));
		glUniform3fv(iLocPhongs.Ks, 1, &(models[cur_idx].shapes[i].material.Ks[0]));
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Matrix4 T, R, S;
	// [TODO] Call back function for keyboard
	if (action == GLFW_PRESS) {
		switch (key)
		{
		case 'Z':
			cur_idx--;
			if (cur_idx < 0)
				cur_idx = (models.size() - 1);
			break;
		case 'X':
			cur_idx++;
			if (cur_idx > (models.size() - 1))
				cur_idx = 0;
			break;
		case 'O':
			setOrthogonal();
			break;
		case 'P':
			setPerspective();
			break;
		case 'T':
			cur_trans_mode = GeoTranslation;
			break;
		case 'S':
			cur_trans_mode = GeoScaling;
			break;
		case 'R':
			cur_trans_mode = GeoRotation;
			break;
		case 'E':
			cur_trans_mode = ViewEye;
			break;
		case 'C':
			cur_trans_mode = ViewCenter;
			break;
		case 'U':
			cur_trans_mode = ViewUp;
			break;
		case 'I':
			T = translate(models[cur_idx].position);
			R = rotate(models[cur_idx].rotation);
			S = scaling(models[cur_idx].scale);
			cout << "Translation Matrix:" << T << endl;
			cout << "Rotation Matrix:" << R << endl;
			cout << "Scaling Matrix:" << S << endl;
			cout << "Viewing Matrix:" << view_matrix << endl;
			cout << "Projection Matrix:" << project_matrix << endl;
			break;
		case 'L':
			cur_light_mode = LightMode((cur_light_mode + 1) % 3);
			main_light = &Lights[LightMode(cur_light_mode)];
			if (cur_light_mode == directional)
				cout << "Directional light" << endl;
			else if (cur_light_mode == positional)
				cout << "Positional(point) light" << endl;
			else
				cout << "Spot light" << endl;
			break;
		case 'K':
			cur_trans_mode = Light_editing;
			break;
		case 'J':
			cur_trans_mode = Shine_editing;
			break;
		default:
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	yoffset *= 0.03;
	switch (cur_trans_mode) {
	case GeoRotation:
		models[cur_idx].rotation.z += yoffset;
		break;
	case GeoTranslation:
		models[cur_idx].position.z += yoffset;
		break;
	case GeoScaling:
		models[cur_idx].scale.z += yoffset;
		break;
	case ViewEye:
		main_camera.position.z += yoffset;
		setViewingMatrix();
		break;
	case ViewCenter:
		main_camera.center.z += yoffset;
		setViewingMatrix();
		break;
	case ViewUp:
		main_camera.up_vector.z += yoffset;
		setViewingMatrix();
		break;
	case Light_editing:
		if (cur_light_mode == spot) {
			main_light->cutoff = (main_light->cutoff + 100 * yoffset < 180) ? ((main_light->cutoff + 100 * yoffset > 0) ? main_light->cutoff + 100 * yoffset : 0) : 0;
		}
		else {
			main_light->diffuse += Vector3(yoffset, yoffset, yoffset);
		}
		break;
	case Shine_editing:
		shininess += 100 * yoffset;
		break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if ((button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_LEFT) && action == GLFW_PRESS) {
		mouse_pressed = true;
	}
	else {
		mouse_pressed = false;
	}
}
double x = 0;
double y = 0;
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	double x_offset = (xpos - x) / WINDOW_WIDTH;
	double y_offset = -1 * (ypos - y) / WINDOW_HEIGHT;

	if (mouse_pressed) {
		switch (cur_trans_mode) {
		case GeoRotation:
			models[cur_idx].rotation.y += x_offset;
			models[cur_idx].rotation.x -= y_offset;
			break;
		case GeoTranslation:
			models[cur_idx].position.x += x_offset;
			models[cur_idx].position.y += y_offset;
			break;
		case GeoScaling:
			models[cur_idx].scale.x += x_offset;
			models[cur_idx].scale.y -= y_offset;
			break;
		case ViewCenter:
			main_camera.center.x += x_offset;
			main_camera.center.y += y_offset;
			setViewingMatrix();
			break;
		case ViewEye:
			main_camera.position.x += x_offset;
			main_camera.position.y += y_offset;
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector.x += x_offset * 10;
			main_camera.up_vector.y += y_offset * 10;
			setViewingMatrix();
			break;
		case Light_editing:
			main_light->position.x += x_offset;
			main_light->position.y += y_offset;
			break;
		}
	}
	x = xpos;
	y = ypos;

}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");
	iLocMV = glGetUniformLocation(p, "MV");
	
	iLocLights.position = glGetUniformLocation(p, "l_pos");
	
	iLocLights.exponent = glGetUniformLocation(p, "exponent");
	iLocLights.cutoff = glGetUniformLocation(p, "cutoff");
	iLocLights.iLocDiffuse = glGetUniformLocation(p, "Ip");
	iLocLights.iLocAmbient = glGetUniformLocation(p, "Ia");
	iLocShininess = glGetUniformLocation(p, "shininess");
	iLocshader = glGetUniformLocation(p, "shader");
	iLocLMode = glGetUniformLocation(p, "LMode");
	iLocCamPos = glGetUniformLocation(p, "camPos");

	iLocPhongs.Ka = glGetUniformLocation(p, "Ka");
	iLocPhongs.Kd = glGetUniformLocation(p, "Kd");
	iLocPhongs.Ks = glGetUniformLocation(p, "Ks");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT/2;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);
	main_light = &Lights[0];
	Lights[LightMode(directional)].position = Vector3(1.0f, 1.0f, 1.0f);
	Lights[LightMode(directional)].direction = Vector3(0.0f, 0.0f, 0.0f);
	Lights[LightMode(directional)].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Lights[LightMode(directional)].ambient = Vector3(0.15f, 0.15f, 0.15f);

	Lights[LightMode(positional)].position = Vector3(0.0f, 2.0f, 1.0f);
	Lights[LightMode(positional)].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Lights[LightMode(positional)].ambient = Vector3(0.15f, 0.15f, 0.15f);

	Lights[LightMode(spot)].position = Vector3(0.0f, 0.0f, 2.0f);
	Lights[LightMode(spot)].direction = Vector3(0.0f, 0.0f, -1.0f);
	Lights[LightMode(spot)].exponent = 50;
	Lights[LightMode(spot)].cutoff = 30;
	Lights[LightMode(spot)].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Lights[LightMode(spot)].ambient = Vector3(0.15f, 0.15f, 0.15f);

	shininess = 64.0f;
	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	for (int i = cur_idx; i < model_list.size(); i++)
	{
		LoadModels(model_list[i]);
	}
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "109062531 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
