#version 410

layout (location = 0) in vec2 vertex_position_cenario;
layout (location = 1) in vec2 texture_mapping_cenario;
layout (location = 2) in vec2 vertex_position_objeto;
layout (location = 3) in vec2 texture_mapping_objeto;

out vec2 texture_coords;
uniform float layer_z;
uniform float tx;
uniform float ty;
uniform bool isObject;

void main () {
    if(isObject){
		texture_coords = texture_mapping_objeto;
		gl_Position = vec4(vertex_position_objeto.x + tx, vertex_position_objeto.y + ty, layer_z, 1.0);
	}
	else {
		texture_coords = texture_mapping_cenario;
		gl_Position = vec4(vertex_position_cenario.x + tx, vertex_position_cenario.y + ty, layer_z, 1.0);
	}
}
