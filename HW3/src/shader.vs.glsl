#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;

out vec3 vertex_position;
out vec3 vertex_color;
out vec3 vertex_normal;

uniform mat4 um4p;	// projection matrix
uniform mat4 um4v;	// camera viewing transformation matrix
uniform mat4 um4m;	// rotation matrix

uniform	vec3 l_position;
uniform vec3 l_direction;
uniform	int exponent;
uniform	float cutoff;
uniform vec3 Ip;
uniform	vec3 Ia;
uniform	float shininess;
uniform	vec3 Ka;
uniform	vec3 Kd;
uniform	vec3 Ks;
uniform	int Shader;
uniform int LightMode;
uniform vec3 camPos;

uniform vec2 texoffset;
uniform int texcontrol;
void main() 
{
	if(texcontrol==1)
		texCoord = vec2(aTexCoord.x+texoffset.x,aTexCoord.y+texoffset.y);
	else
		texCoord = vec2(aTexCoord.x,aTexCoord.y);
	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
	vertex_normal = (transpose(inverse(um4m)) * vec4(aNormal, 0.0)).xyz;
	vertex_position = vec4(um4m * vec4(aPos, 1.0f)).xyz;

	if(Shader == 0){		
		vec3 N = normalize(vertex_normal);
		vec3 L = (LightMode == 0) ? normalize(l_position - vec3(0.0f, 0.0f, 0.0f)) : normalize(l_position - vertex_position);

		vec3 Ipp = vec3(1.0f, 1.0f, 1.0f);
		vec3 H = normalize(L + camPos - vertex_position);

		vec3 ambient = Ia * Ka;
		vec3 diffuse = Ip * Kd * max(dot(L, N), 0);
		vec3 specular = Ipp * Ks * pow(max(dot(H, N), 0.0), shininess);

		float d = length(l_position - vertex_position);

		float cos_cutoff = cos(cutoff);
		
		if(LightMode == 0){
			vertex_color = ambient + diffuse + specular;
			//vertex_color = specular;
		}else if(LightMode == 1){
			float fp = min(1 / (0.01 + 0.8 * d + 0.1 * d * d), 1);
			vertex_color = ambient + fp * (diffuse + specular);
			//vertex_color = specular;
		}else{
			float fp = min(1 / (0.05 + 0.3 * d + 0.6 * d * d), 1);
			float cos_vertex_angle = dot(L, normalize(vec3(0, 0, 1)));
			float spotlight_eff = (cos_vertex_angle > cos_cutoff) ? pow(max(cos_vertex_angle, 0), exponent) : 0;
			vertex_color = ambient + fp * spotlight_eff * (diffuse + specular);
		}
		vertex_color = aColor * vertex_color;
	}
	else{
		vertex_color = aColor;
	}
}
