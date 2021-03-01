#version 430

#define AO_DEINTERLEAVED 0
#define AO_BLUR 1

#define VERTEX_POS    0
#define VERTEX_NORMAL 1
#define VERTEX_COLOR  2

#define UBO_SCENE     0

#define AO_RANDOMTEX_SIZE 4

uniform  float   control_RadiusToScreen;
uniform  float   control_R2;
uniform  float   control_NegInvR2;
uniform  float   control_NDotVBias;
 
uniform  vec2    control_InvFullResolution;
uniform  vec2    control_InvQuarterResolution;
  
uniform  float   control_AOMultiplier;
uniform  float   control_PowExponent;
  
uniform  vec4    control_projInfo;
uniform  vec2    control_projScale;
uniform  int     control_projOrtho;

// The pragma below is critical for optimal performance
// in this fragment shader to let the shader compiler
// fully optimize the maths and batch the texture fetches
// optimally

#pragma optionNV(unroll all)

#ifndef AO_DEINTERLEAVED
#define AO_DEINTERLEAVED 1
#endif

#ifndef AO_BLUR
#define AO_BLUR 1
#endif

#ifndef AO_LAYERED
#define AO_LAYERED 1
#endif

#define M_PI 3.14159265f

// tweakables
const float  NUM_STEPS = 4;
const float  NUM_DIRECTIONS = 8; // texRandom/g_Jitter initialization depends on this

#if AO_DEINTERLEAVED

#if AO_LAYERED
  vec2 g_Float2Offset = control.float2Offsets[gl_PrimitiveID].xy;
  vec4 g_Jitter       = control.jitters[gl_PrimitiveID];
  
  layout(binding=0) uniform sampler2DArray texLinearDepth;
  layout(binding=1) uniform sampler2D texViewNormal;

  vec3 getQuarterCoord(vec2 UV){
    return vec3(UV,float(gl_PrimitiveID));
  }
  #if AO_LAYERED == 1
  
    #if AO_BLUR
      layout(binding=0,rg16f) uniform image2DArray imgOutput;
    #else
      layout(binding=0,r8) uniform image2DArray imgOutput;
    #endif

    void outputColor(vec4 color) {
      imageStore(imgOutput, ivec3(ivec2(gl_FragCoord.xy),gl_PrimitiveID), color);
    }
  #else
    layout(location=0,index=0) out vec4 out_Color;
  
    void outputColor(vec4 color) {
      out_Color = color;
    }
  #endif
#else
  layout(location=0) uniform vec2 g_Float2Offset;
  layout(location=1) uniform vec4 g_Jitter;
  
  layout(binding=0) uniform sampler2D texLinearDepth;
  layout(binding=1) uniform sampler2D texViewNormal;
  
  vec2 getQuarterCoord(vec2 UV){
    return UV;
  }

  layout(location=0,index=0) out vec4 out_Color;
  
  void outputColor(vec4 color) {
    out_Color = color;
  }
#endif
  
#else
  layout(binding=0) uniform sampler2D texLinearDepth;
  layout(binding=1) uniform sampler2D texRandom;
  
  layout(location=0,index=0) out vec4 out_Color;
  
  void outputColor(vec4 color) {
    out_Color = color;
  }
#endif

in vec2 texCoord;

//----------------------------------------------------------------------------------

vec3 UVToView(vec2 uv, float eye_z)
{
  return vec3((uv * control_projInfo.xy + control_projInfo.zw) * (control_projOrtho != 0 ? 1. : eye_z), eye_z);
}

#if AO_DEINTERLEAVED

vec3 FetchQuarterResViewPos(vec2 UV)
{
  float ViewDepth = textureLod(texLinearDepth,getQuarterCoord(UV),0).x;
  return UVToView(UV, ViewDepth);
}

#else //AO_DEINTERLEAVED

vec3 FetchViewPos(vec2 UV)
{
  float ViewDepth = textureLod(texLinearDepth,UV,0).x;
  return UVToView(UV, ViewDepth);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
  vec3 V1 = Pr - P;
  vec3 V2 = P - Pl;
  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal(vec2 UV, vec3 P)
{
  vec3 Pr = FetchViewPos(UV + vec2(control_InvFullResolution.x, 0));
  vec3 Pl = FetchViewPos(UV + vec2(-control_InvFullResolution.x, 0));
  vec3 Pt = FetchViewPos(UV + vec2(0, control_InvFullResolution.y));
  vec3 Pb = FetchViewPos(UV + vec2(0, -control_InvFullResolution.y));
  return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

#endif //AO_DEINTERLEAVED

//----------------------------------------------------------------------------------
float Falloff(float DistanceSquare)
{
  // 1 scalar mad instruction
  return DistanceSquare * control_NegInvR2 + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(vec3 P, vec3 N, vec3 S)
{
  vec3 V = S - P;
  float VdotV = dot(V, V);
  float NdotV = dot(N, V) * 1.0/sqrt(VdotV);

  // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
  return clamp(NdotV - control_NDotVBias,0,1) * clamp(Falloff(VdotV),0,1);
}

//----------------------------------------------------------------------------------
vec2 RotateDirection(vec2 Dir, vec2 CosSin)
{
  return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,
              Dir.x*CosSin.y + Dir.y*CosSin.x);
}

//----------------------------------------------------------------------------------
vec4 GetJitter()
{
#if AO_DEINTERLEAVED
  // Get the current jitter vector from the per-pass constant buffer
  return g_Jitter;
#else
  // (cos(Alpha),sin(Alpha),rand1,rand2)
  return textureLod( texRandom, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE), 0);
#endif
}

//----------------------------------------------------------------------------------
float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal)
{
#if AO_DEINTERLEAVED
  RadiusPixels /= 4.0;
#endif

  // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
  float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

  const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;
  float AO = 0;

  for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
  {
    float Angle = Alpha * DirectionIndex;

    // Compute normalized 2D direction
    vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);

    // Jitter starting sample within the first step
    float RayPixels = (Rand.z * StepSizePixels + 1.0);

    for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
    {
#if AO_DEINTERLEAVED
      vec2 SnappedUV = round(RayPixels * Direction) * control_InvQuarterResolution + FullResUV;
      vec3 S = FetchQuarterResViewPos(SnappedUV);
#else
      vec2 SnappedUV = round(RayPixels * Direction) * control_InvFullResolution + FullResUV;
      vec3 S = FetchViewPos(SnappedUV);
#endif

      RayPixels += StepSizePixels;

      AO += ComputeAO(ViewPosition, ViewNormal, S);
    }
  }

  AO *= control_AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
  return clamp(1.0 - AO * 2.0,0,1);
}

//----------------------------------------------------------------------------------
void main()
{
  
#if AO_DEINTERLEAVED
  vec2 base = floor(gl_FragCoord.xy) * 4.0 + g_Float2Offset;
  vec2 uv = base * (control_InvQuarterResolution / 4.0);

  vec3 ViewPosition = FetchQuarterResViewPos(uv);
  vec4 NormalAndAO =  texelFetch( texViewNormal, ivec2(base), 0);
  vec3 ViewNormal =  -(NormalAndAO.xyz * 2.0 - 1.0);
#else
  vec2 uv = texCoord;
  vec3 ViewPosition = FetchViewPos(uv);

  // Reconstruct view-space normal from nearest neighbors
  vec3 ViewNormal = -ReconstructNormal(uv, ViewPosition);
#endif

  // Compute projection of disk of radius control.R into screen space
  float RadiusPixels = control_RadiusToScreen / (control_projOrtho != 0 ? 1.0 : ViewPosition.z);

  // Get jitter vector for the current full-res pixel
  vec4 Rand = GetJitter();

  float AO = ComputeCoarseAO(uv, RadiusPixels, Rand, ViewPosition, ViewNormal);

#if AO_BLUR
  outputColor(vec4(pow(AO, control_PowExponent), ViewPosition.z, 0, 0));
#else
  outputColor(vec4(pow(AO, control_PowExponent)));
#endif
  
}
