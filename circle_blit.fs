#version 410

out vec4 fragcolor;
in vec2 uv;
uniform vec2 offset;
uniform float radius;
uniform float level;
uniform vec3 color;
void main(){
  if(length(offset - uv) < radius){
      fragcolor = vec4(color,level);
  }else{
	// fragcolor = vec4(0,1,1,1);
	discard;
  }
}
