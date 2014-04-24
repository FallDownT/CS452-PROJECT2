#version 130

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

in vec3 in_position;
in vec4 in_color;

out vec4 pass_color;

void main(){
  gl_Position=projectionMatrix*viewMatrix*modelMatrix*vec4(in_position,1.0); 
  pass_color=in_color;
}
