#version 130
in  vec3 fN;
in  vec3 fL;
in  vec3 fE;

out vec4 fColor;
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform float Shininess;

void main(){
	 vec3 N = normalize(fN);
	 vec3 E = normalize(fE);
	 vec3 L = normalize(fL);

	 vec3 H = normalize( L + E );
	 float Kd = max(dot(L, N), 0.0);
	 float Ks = pow(max(dot(N, H), 0.0), Shininess);

	 vec4 ambient = AmbientProduct;
	 vec4 diffuse = Kd*DiffuseProduct;
	 vec4 specular = Ks*SpecularProduct;
	 
	 
	 // discard the specular highlight if the light's behind the vertex
     if( dot(L, N) < 0.0 ) {
	    specular = vec4(0.0, 0.0, 0.0, 1.0);
     }

     fColor = ambient + diffuse + specular;
     fColor.a = 1.0;
}
