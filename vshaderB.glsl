#version 130

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 LightPosition;

in vec4 in_position;
in vec3 in_normals;

out vec3 fN;
out vec3 fE;
out vec3 fL;

void main(){
	fN = in_normals;
	fE = ((viewMatrix*modelMatrix)*in_position).xyz;
	fL = LightPosition.xyz;

	if( LightPosition.w != 0.0 ) {
		fL = LightPosition.xyz - in_position.xyz;
	}

	gl_Position=projectionMatrix*viewMatrix*modelMatrix*in_position; 
}
