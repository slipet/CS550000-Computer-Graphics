#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 vertex_pos;
uniform mat4 mvp;
uniform mat4 MV;

uniform	vec3 l_pos;

uniform	int exponent;
uniform	float cutoff;
uniform vec3 Ip;
uniform	vec3 Ia;
uniform	float shininess;
uniform	vec3 Ka;
uniform	vec3 Kd;
uniform	vec3 Ks;
uniform	int shader;
uniform int LMode;
uniform vec3 camPos;

void main()
{
	// [TODO]
	gl_Position = mvp * vec4(aPos, 1.0);
	
	vertex_normal = (transpose(inverse(MV)) * vec4(aNormal, 0.0)).xyz;
	vertex_pos = vec4(MV * vec4(aPos, 1.0f)).xyz;

	if(shader == 0){		
		vec3 N = normalize(vertex_normal);
		vec3 L = (LMode == 0) ? normalize(l_pos - vec3(0.0f, 0.0f, 0.0f)) : normalize(l_pos - vertex_pos);

		vec3 Ipp = vec3(1.0f, 1.0f, 1.0f);
		vec3 H = normalize(L + camPos - vertex_pos);

		vec3 ambient = Ia * Ka;
		vec3 diffuse = Ip * Kd * max(dot(L, N), 0);
		vec3 specular = Ipp * Ks * pow(max(dot(H, N), 0.0), shininess);

		float d = length(l_pos - vertex_pos);

		float cos_cutoff = cos(cutoff);
		
		if(LMode == 0){
			vertex_color = ambient + diffuse + specular;
			
		}else if(LMode == 1){
			float fp = min(1 / (0.01 + 0.8 * d + 0.1 * d * d), 1);
			vertex_color = ambient + fp * (diffuse + specular);
			
		}else{
			float fp = min(1 / (0.05 + 0.3 * d + 0.6 * d * d), 1);
			float cos_vertex_angle = dot(L, normalize(vec3(0, 0, 1)));
			float spotlight_eff = (cos_vertex_angle > cos_cutoff) ? pow(max(cos_vertex_angle, 0), exponent) : 0;
			vertex_color = ambient + fp * spotlight_eff * (diffuse + specular);
		}
	}
	else{
		vertex_color = aColor;
	}
}

