#version 450

int gHitID;       // ID of the closest hit shape

struct SDF {
    int   type;       // 0 for sphere, 1 for round box, 2 for torus
    vec3  position;  // position of the shape in world space
    vec3  size;      // for round box this is the size of the box, for torus this is the major radius and minor radius
    float radius;   // For sphere this is the radius, for round box, this is the corner radius
    vec3 color;
};

SDF sdfArray[10]; // Array to hold SDF shapes


// Signed distance functions for different shapes
float sdSphere( vec3 position, float radius )
{
  return length(position)-radius;
}

float sdRoundBox( vec3 p, vec3 b, float r )
{
  vec3 q = abs(p) - b + r;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

// radius.x is the major radius, radius.y is the minor radius
float sdTorus( vec3 p, vec2 radius )
{
    // length(p.xy) - radius.x measures how far this point is from the torus ring center in the XY-plane.
  vec2 q = vec2(length(p.xy)-radius.x,p.z);
  return length(q)-radius.y;
}

float evalSDF(SDF s, vec3 p) {
    if (s.type == 0) {
        return sdSphere((p - s.position), s.radius);
    } else if (s.type == 1) {
        return sdRoundBox(p - s.position, s.size, s.radius);
    }
    else if(s.type == 2)
    return sdTorus(p - s.position, s.size.yz);

    return 1e5;
}
// Evaluate the scene by checking all SDF shapes
float evaluateScene(vec3 p) {
    float d = 1e5;
    int bestID = -1;
    for (int i = 0; i < 4; ++i) {
        float di = evalSDF(sdfArray[i], p);
        if(di < d)
        {
            d = di; // Update the closest distance
            bestID = i; // Update the closest hit ID
        }
    }
    
    gHitID = bestID;  // Store the ID of the closest hit shape
    return d;
}
// Estimate normal by central differences
vec3 SDFsNormal(vec3 p) {
    float h = 0.0001;
    vec2 k = vec2(1, -1);
    return normalize(
        k.xyy * evaluateScene(p + k.xyy * h) +
        k.yyx * evaluateScene(p + k.yyx * h) +
        k.yxy * evaluateScene(p + k.yxy * h) +
        k.xxx * evaluateScene(p + k.xxx * h)
    );
}

// Raymarching function
float raymarch(vec3 ro, vec3 rd, out vec3 hitPos) {
    float t = 0.0;
    for (int i = 0; i < 100; i++) {
        vec3 p = ro + rd * t;     // Current point in the ray
        float d = evaluateScene(p);
        if (d < 0.001) {
            hitPos = p;
            return t;
        }
        if (t > 50.0) break;
        t += d;
    }
    return -1.0; // No hit
}


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler[2];

layout(push_constant) uniform Push {
	mat4 modalMatrix;
	mat4 normalMatrix;
	int textureID;
} push;

void main()
{

    vec2 resolution = vec2(800.0, 600.0);
    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    vec3 color;
    vec3 baseColor;
    vec3 normal;
    
    vec3 ro = vec3(0.0, 0.0, 8.0);         // Ray origin
    vec3 rd = normalize(vec3(uv, -1));

    SDF circle = SDF(0, vec3(0.0), vec3(0.0), 1.0, vec3(0.0,0.0,1.0));
    SDF roundBox = SDF(1, vec3(1.9,0.0,0.0), vec3(1.0,1.0,1.0), 0.2,vec3(0.0,1.0,0.0));
    SDF roundBox2 = SDF(1, vec3(-1.9,0.0,0.0), vec3(1.0,1.0,1.0), 0.2, vec3(0.0,1.0,0.0));
    SDF torus = SDF(2, vec3(0.0), vec3(1.0,5.0,1.5), 0.2, vec3(1.0,0.0,0.0));

    sdfArray[0] = circle;
    sdfArray[1] = roundBox;
    sdfArray[2] = roundBox2;
    sdfArray[3] = torus;

    vec3 hitPos;
    float t = raymarch(ro, rd, hitPos);  // Raymarching to find the closest hit point
    
     if (t > 0.0) {

      normal = SDFsNormal(hitPos);
      color = sdfArray[gHitID].color;    
    }
    else{
    color = vec3(0.0, 0.0, 0.0);
    }
    outColor = vec4(color, 1.0);

	//if(push.textureID == 0)
	//outColor = texture(texSampler[0], fragTexCoord);
	//if(push.textureID == 1)
	//outColor = texture(texSampler[1], fragTexCoord);
}