/*
19a_blinnphong_tex_multiplelights.frag: as 16_blinnphong_multiplelights.frag, but with but with texturing

N.B. 1)  "18_phong_tex_multiplelights.vert" must be used as vertex shader

N.B. 2) In this example, we consider point lights only. For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

N.B. 3) there are other methods (more efficient) to pass multiple data to the shaders (search for Uniform Buffer Objects)

N.B. 4) with last versions of OpenGL, using structures like the one cited above, it is possible to pass a dynamic number of lights

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

#version 330 core

// number of lights in the scene
#define NR_LIGHTS 3

// output shader variable
out vec4 colorFrag;

// array with lights incidence directions (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDirs[NR_LIGHTS];
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;
// vector from fragment to camera (in view coordinate)
in vec3 vViewPosition;
// interpolated texture coordinates
in vec2 interp_UV;

// texture repetitions
uniform float repeat;

// texture sampler
uniform sampler2D tex;

// ambient, and specular components (passed from the application)
uniform vec3 ambientColor;
uniform vec3 specularColor;
// weight of the components
// in this case, we can pass separate values from the main application even if Ka+Kd+Ks>1. In more "realistic" situations, I have to set this sum = 1, or at least Kd+Ks = 1, by passing Kd as uniform, and then setting Ks = 1.0-Kd
uniform float Ka;
uniform float Kd;
uniform float Ks;
// attenuation parameters
uniform float constant;
uniform float linear;
uniform float quadratic;
// shininess coefficients (passed from the application)
uniform float shininess;


void main(){

  // we repeat the UVs and we sample the texture
  vec2 repeated_Uv = mod(interp_UV*repeat, 1.0);
  vec4 surfaceColor = texture(tex, repeated_Uv);

  // ambient component can be calculated at the beginning
  vec4 color = vec4(Ka*ambientColor,1.0);

  // normalization of the per-fragment normal
  vec3 N = normalize(vNormal);

    //for all the lights in the scene
    for(int i = 0; i < NR_LIGHTS; i++)
    {
      // we take the distance from the light source (before normalization, for the attenuation parameter)
      float distanceL = length(lightDirs[i].xyz);
      // normalization of the per-fragment light incidence direction
      vec3 L = normalize(lightDirs[i].xyz);

      // we calculate the attenuation factor (based on the distance from light source)
      float attenuation = 1.0/(constant + linear*distanceL + quadratic*(distanceL*distanceL));

      // Lambert coefficient
      float lambertian = max(dot(L,N), 0.0);

      // if the lambert coefficient is positive, then I can calculate the specular component
      if(lambertian > 0.0) {
          // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
          vec3 V = normalize( vViewPosition );

          // in the Blinn-Phong model we do not use the reflection vector, but the half vector
          vec3 H = normalize(L + V);

          // we use H to calculate the specular component
          float specAngle = max(dot(H, N), 0.0);
          // shininess application to the specular component
          float specular = pow(specAngle, shininess);

          // We add diffusive (= color sampled from texture) and specular components to the final color
          // N.B. ): in this implementation, the sum of the components can be different than 1
          color += Kd * lambertian * surfaceColor +
                          vec4(Ks * specular * specularColor,1.0);
        color*=attenuation;
      }
    }

    colorFrag  = color;

}
