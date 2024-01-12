#ifndef _GLOBAL_RENDER_
    #include "00_Global_render.fx"
#endif

VertexOutput vsShadow(VertexInput input)
{
    VertexOutput output;
    
    const float3 cameraRight_worldspace = float3(invShadowView._m00, invShadowView._m01, invShadowView._m02);
    const float3 cameraLeft_worldspace = float3(invShadowView._m10, invShadowView._m11, invShadowView._m12);
    const float3 v_wp = input.iPosition * relative_scale + cameraRight_worldspace * input.Position.x * relative_scale + cameraLeft_worldspace * input.Position.y * relative_scale;
    
    output.Position = float4(v_wp, 1);
    output.wPosition = input.iPosition * relative_scale;
    output.Position = mul(output.Position, shadowView);
    output.Position = mul(output.Position, shadowProjection);
    output.colorData = input.iColor;
    output.Uv = input.Uv;
    return output;
}

float psShadowDepth(VertexOutput input) : SV_Depth
{
    float3 N;
    N.xy = input.Uv * 2.0f - 1.0f;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1)
    {
        discard;
    }
    N.z = sqrt(1.0f - r2);
    float3 pos = input.wPosition + N * radius * relative_scale;
    return length(pos - invShadowView._m30_m31_m32);
}

float4 psShadowThichkness(VertexOutput input) : SV_Target
{
    float4 output;
    float thickness;
    [flatten]
    if (length(input.Uv) > 1)
    {
        discard;
    }
    float z = sqrt(1 - dot(input.Uv, input.Uv));
    float r = (radius * relative_scale);
    thickness = z * exp(-2.0f * r * r);
   
    output = float4(z, 0, 0, thickness_blend_aplha);
    return output;
}

VertexOutput_ScreenSpace vsScreen(uint id : SV_VertexID)
{
    VertexOutput_ScreenSpace output;
    output.Position = float4(NDC[id], 0, 1);
    output.Screen = output.Position.xy;
    output.Uv = ScreenUV[id];
    return output;
}

VertexOutput vsSphere(VertexInput input)
{
    VertexOutput output;
    output.Position = input.Position;
    output.Uv = input.Uv;
    
    matrix scaleup = { float4(relative_scale, 0, 0, 0), float4(0, relative_scale, 0, 0), float4(0, 0, relative_scale, 0), float4(input.iPosition * relative_scale, 1) };
    output.Position = mul(output.Position, scaleup);    
    output.wPosition = output.Position;
    output.Position = mul(output.Position, view);
    output.Position = mul(output.Position, projection);    

    output.colorData = input.iColor;
    
    return output;
}

VertexOutput vsQuad(VertexInput input)
{
    VertexOutput output;
    
    output.Uv = input.Uv;
    
    float3 cameraRight_worldspace = float3(invView._m00, invView._m01, invView._m02);
    float3 cameraLeft_worldspace = float3(invView._m10, invView._m11, invView._m12);
    float3 v_wp = input.iPosition * relative_scale + cameraRight_worldspace * input.Position.x * relative_scale + cameraLeft_worldspace * input.Position.y * relative_scale;
    
    output.Position = float4(v_wp, 1);
    output.wPosition = input.iPosition * relative_scale;
    output.Position = mul(output.Position, view);
    output.Position = mul(output.Position, projection);
    
    output.colorData = input.iColor;
    
    return output;
}

float4 psSphere(VertexOutput input) : SV_Target
{
    return float4(input.colorData, 1);
}

float4 psQuad(VertexOutput input) : SV_Target
{
    float3 N;
    N.xy = input.Uv * 2.0f - 1.0f;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1)
    {
        discard;
    }
    N.z = -sqrt(1.0f - r2);  
    
    float3 pos = input.wPosition + N * radius * relative_scale;
    //return float4((pos / (boundary * relative_scale)) * dot(-N, light), 1);
    
    return float4(input.colorData, 1);
}

float2 worldToTexelCoord(in float4 w)
{
    float4 p = mul(w, view);
    p = mul(p, projection);
    p /= p.w;
    p.y *= -1;
    p.xy *= 0.5f;
    p.xy += 0.5f;
    return p.xy;
}


float4 psBoundary(VertexOutput input) : SV_Target
{
    [flatten]
    if (length(input.Uv) > 1)
    {
        discard;
    }    
    return float4(0.5, 0.5, 0.5, 1);
}
float4 psRefractFrontFace(VertexOutput input) : SV_Target
{
    float3 N;    
    N.xy = input.Uv * 2.0f - 1.0f;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1)
    {
        discard;
    }
    N.z = -sqrt(1.0f - r2);    
    float3 pos = input.wPosition + N * radius * relative_scale;
    return float4(length(pos - invView._m30_m31_m32), 0, 0, 0);
}

float4 psRefractBackFace(VertexOutput input) : SV_Target
{
    float3 N;    
    N.xy = input.Uv * 2.0f - 1.0f;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1)
    {
        discard;
    }
    N.z = sqrt(1.0f - r2);    
    float3 pos = input.wPosition + N * radius * relative_scale;
    return float4(length(pos - invView._m30_m31_m32), 0, 0, 0);
}


VertexOutput_BG vsBackGround(VertexInput_BG input)
{
    VertexOutput_BG output;
    float s = (far - near) / 2.0 + near;    
    matrix world_transform = matrix(float4(s,0,0,0), float4(0,s,0,0), float4(0,0,s,0), float4(invView._m30_m31_m32,1));
    
    output.Position = mul(input.Position, world_transform);
    output.vPosition = output.Position;
    output.Position = mul(output.Position, view);
    output.Position = mul(output.Position, projection);

    return output;
}

float4 psBackGround(VertexOutput_BG input) : SV_Target
{    
    float3 normal = normalize(input.vPosition - invView._m30_m31_m32);
    return backGroundCube.Sample(LinearSampler, normal);
}

float3 depthToPos(in float z, in float2 uv)
{
    // uv to NDC
    uv.xy *= 2.0f;
    uv.xy -= 1.0f;
    uv.y *= -1.0f;
    // NDC to World
    float2 xy = 1.0f / float2(projection._11, projection._22);
    float4 pos = float4(xy * uv * z, z, 1.0f);
    pos = mul(pos, invView);
    return pos.xyz;
}

float3 getNormalFromDepth(in float3 pos, in float2 uv, in float2 offset, in float channel)
{
    
    float2 uvX = uv + float2(offset.x, 0);
    float2 uvX2 = uv - float2(offset.x, 0);    
    float2 uvY = uv + float2(0, offset.y);
    float2 uvY2 = uv - float2(0, offset.y);
    float2 dxDepth, dxDepth2, dyDepth, dyDepth2;
    
    dxDepth = smoothTexture.SampleLevel(LinearSampler, uvX, 0).xy;
    dxDepth2 = smoothTexture.SampleLevel(LinearSampler, uvX2, 0).xy;
    dyDepth = smoothTexture.SampleLevel(LinearSampler, uvY, 0).xy;
    dyDepth2 = smoothTexture.SampleLevel(LinearSampler, uvY2, 0).xy;
     
    if(channel == 0)
    {
        return normalize(cross(depthToPos(dxDepth.x, uvX) - pos, depthToPos(dyDepth.x, uvY) - pos));    
    }
    else
    {
        return normalize(cross(depthToPos(dxDepth.y, uvX) - pos, depthToPos(dyDepth.y, uvY) - pos));    
    }
    
}

float4 psDebug(VertexOutput_ScreenSpace input) : SV_Target
{
    return (0, 0, 0, 0);
}


struct BilateralFilteringOutput
{
    float4 rtv0 : SV_Target0;
};

void Blurring(out float4 result, in VertexOutput_ScreenSpace input)
{
    const float2 uv = input.Uv;
    const float2 uvOffset = 1.0f / float2(width, height);
    
    float2 origin = float2(frontTargetTexture.SampleLevel(LinearSampler, uv, 0), backTargetTexture.SampleLevel(LinearSampler, uv, 0));
    
    // 배경은 블러에서 제외한다.
    [flatten]
    if(origin.x >= far) 
    {
        result = float4(origin, 0, 0);
        return;
    }
    // 텍셀 편차
    deviation_texel *= uvOffset * filterRadius;
    // 깊이 편차
    deviation_depth *= (far - near);
    
    float2 r2;
    float2 g;
    
    float2 sum = float2(0, 0);
    float2 wsum = float2(0, 0);
    
    float2 sample;    
    float2 uv2;
    
    for (float x = -filterRadius; x <= filterRadius; x += 1.0f)
    {
        for (float y = -filterRadius; y <= filterRadius; y += 1.0f)
        {
            uv2 = uv + uvOffset * float2(x, y);
            
            if (uv2.x < 0 || uv2.x > 1 || uv2.y < 0 || uv2.y > 1)
            {
                continue;
            }
            sample.x = frontTargetTexture.SampleLevel(LinearSampler, uv2, 0).x;
            sample.y = backTargetTexture.SampleLevel(LinearSampler, uv2, 0).x;
    
            float r = length(float2(x, y)) * deviation_texel;
            r2 = (sample - origin) * deviation_depth;
            
            float w = exp(-(r * r));
            g = exp(-(r2 * r2));
            
            sum += sample * w * g;           
            wsum += w * g;
        }
    }
    [flatten]
    if (wsum.x > 0.0f)
    {
        sum.x /= wsum.x;
    }
    [flatten]
    if (wsum.y > 0.0f)
    {
        sum.y /= wsum.y;
    }    
    result = float4(sum, 0, 0);
}

BilateralFilteringOutput psSmoothing(VertexOutput_ScreenSpace input)
{
    BilateralFilteringOutput output;    
    Blurring(output.rtv0, input);
    
    return output;
}

///////////////////////////////////////////////////////
// Add blend mode 로 모든 파티클의 두께를 합친다.!!!!!!
///////////////////////////////////////////////////////
float4 psThichkness(VertexOutput input) : SV_Target
{
    float4 output;
    float thickness;    
    [flatten]
    if (length(input.Uv) > 1.0f)
    {
        discard;
    }
    const float z = sqrt(1.0f - dot(input.Uv, input.Uv));
    const float r = (radius * relative_scale);
    thickness = z * exp(-2.0f * r * r);
    output = float4(thickness, 0, 0, thickness_blend_aplha);
   
    return output;
}

float4 psForeGround(VertexOutput_ScreenSpace input) : SV_Target
{
    light = normalize(light);
    
    float2 surfaceDepth = smoothTexture.SampleLevel(LinearSampler, input.Uv, 0).xy;
    float front = surfaceDepth.x;
    float back = surfaceDepth.y;
    
    [flatten]
    if (front >= far )    
    {
        return float4(pow(backGroundTexture.Sample(LinearSampler, input.Uv).xyz, 1.0f ), 1);
    }
    
    const float distance = clamp(back - front, 0, far);
    
    const float3 absorptionColor = exp(Water_Absorption * -distance);;
    const float2 offset = float2(1.0f / width, 1.0f / height);
    
    const float thickness = thicknessTexture.Sample(PointSampler, input.Uv).x;
    
    const float3 frontPos = depthToPos(front, input.Uv);    
    const float3 normal = getNormalFromDepth(frontPos, input.Uv, offset, 0);
    
    float3 incident = normalize(frontPos - invView._m30_m31_m32);    
    const float3 viewRay = incident;
    const float NdotL = saturate(dot(normal, -light)) * 5.0f + 0.5f;
    
    ////////////////////////////////////////////////////////////////////    
    // surface shading
    ////////////////////////////////////////////////////////////////////
    float3 halfV = normalize(-incident + -light);
    float NdotH = saturate(dot(normal, halfV));
    float specular = pow(NdotH, specPower);
    float3 reflectV = reflect(incident, normal);
    
    float3 reflectColor = backGroundCube.Sample(LinearSampler, reflectV).rgb;
    reflectColor *= intensity * specular;
    
    ////////////////////////////////////////////////////////////////////
    // thickenss shading
    ////////////////////////////////////////////////////////////////////
    incident = refract(incident, normal, 1.0f / Water_RefractIndex);
    float3 refractColor = backGroundTexture.SampleLevel(LinearSampler, worldToTexelCoord(float4( incident + frontPos, 1)), 0).rgb;
    refractColor = lerp(refractColor, absorptionColor, thickness);
    
    float3 blendColor = reflectColor + refractColor.xyz;
    //float3 blendColor = lerp(reflectColor.xyz, refractColor.xyz, transparency);
    return float4(blendColor, 1);
}




