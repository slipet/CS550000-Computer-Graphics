#version 330



in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_position;
in vec2 texCoord;

out vec4 fragColor;

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

// [TODO] passing texture from main.cpp
// Hint: sampler2D
uniform sampler2D tex;

/*void main() {
	fragColor = vec4(texCoord.xy, 0, 1);

	// [TODO] sampleing from texture
	// Hint: texture
}*/

void main() {
	if(Shader == 0)
		
		fragColor = texture(tex, texCoord) * vec4(vertex_color, 1.0f);
		
	else{
		//vertex_color = aColor;
		

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
			fragColor = vec4(ambient + diffuse + specular, 1.0f);
			//vertex_color = specular;
		}else if(LightMode == 1){
			float fp = min(1 / (0.01 + 0.8 * d + 0.1 * d * d), 1);
			fragColor = vec4(ambient + fp * (diffuse + specular), 1.0f);
			//vertex_color = specular;
		}else{
			float fp = min(1 / (0.05 + 0.3 * d + 0.6 * d * d), 1);
			float cos_vertex_angle = dot(L, normalize(vec3(0, 0, 1)));
			float spotlight_eff = (cos_vertex_angle > cos_cutoff) ? pow(max(cos_vertex_angle, 0), exponent) : 0;
			fragColor = vec4(ambient + fp * spotlight_eff * (diffuse + specular), 1.0f);
		}
		
		fragColor = texture(tex, vec2(texCoord.x,texCoord.y)) * fragColor;
	}
}