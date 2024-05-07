/*
Approach taken from https://steps3d.narod.ru/tutorials/skin-tutorial.html
*/

#version 450

layout(location = 0) in vec2 tex;
layout(location = 0) out vec4 color;

layout(push_constant) uniform params_t
{
  vec2 step;
  float blurScale;
  float correction;
} params;

layout(binding = 0) uniform sampler2D colorMap;
layout(binding = 1) uniform sampler2D depthMap;

const float zNear     = 0.01;
const float zFar      = 100.0;
const float SSSS_FOVY = 45.0;

float linearDepth ( float d )
{
    return zFar * zNear / (d * (zFar - zNear) - zFar );
}

void    main ()
{
    const float gamma = 2.2;
    
    const vec4 kernel [] = vec4 [] (
        vec4 ( 0.560479,   0.669086,    0.784728,      0    ),
        vec4 ( 0.00471691, 0.000184771, 5.07566e-005, -2    ),
        vec4 ( 0.0192831,  0.00282018,  0.00084214,   -1.28 ),
        vec4 ( 0.03639,    0.0130999,   0.00643685,   -0.72 ),
        vec4 ( 0.0821904,  0.0358608,   0.0209261,    -0.32 ),
        vec4 ( 0.0771802,  0.113491,    0.0793803,    -0.08 ),
        vec4 ( 0.0771802,  0.113491,    0.0793803,     0.08 ),
        vec4 ( 0.0821904,  0.0358608,   0.0209261,     0.32 ),
        vec4 ( 0.03639,    0.0130999,   0.00643685,    0.72 ),
        vec4 ( 0.0192831,  0.00282018,  0.00084214,    1.28 ),
        vec4 ( 0.00471691, 0.000184771, 5.07565e-005,  2    )
    );
        // fetch color and linear depth for current pixel
    vec4    colorM = texture ( colorMap, tex );
    float   depthM = linearDepth ( texture ( depthMap, tex ).r );
    vec2    size   = vec2 ( textureSize ( colorMap, 0 ) );

        // accumulate center sample multiplying it with gaussian weight
    vec4    colorBlurred               = colorM;
    float   distanceToProjectionWindow = 1.0 / tan(0.5 * radians(SSSS_FOVY));
    float   scale                      = distanceToProjectionWindow / depthM;

        // Calculate the final step to fetch the surrounding pixels:
    vec2 finalStep = params.blurScale * scale * params.step / size;

    finalStep *= colorM.a;      // Modulate it using the alpha channel.
    finalStep *= 1.0 / 3.0;     // Divide by 3 as the kernels range from -3 to 3.

        // Accumulate the center sample:
    colorBlurred.rgb *= kernel[0].rgb;

    for ( int i = 1; i < 11; i++ )
    {
            // Fetch color and depth for current sample:
        vec2 offset = tex + kernel[i].a * finalStep;
        vec4 color  = texture ( colorMap, offset );

            // If the difference in depth is huge, we lerp color back to "colorM":
        float depth = linearDepth ( texture ( depthMap, offset).r );
        float s     = min ( 1.0, distanceToProjectionWindow * params.blurScale * abs ( depthM - depth) / params.correction );
        
        color.rgb = mix ( color.rgb, colorM.rgb, s );

            // Accumulate:
        colorBlurred.rgb += kernel[i].rgb * color.rgb;
    }

    color = colorBlurred;
    
    if ( params.step.y > params.step.x )      // for second pass step=(0,1)
    {
        color.rgb = pow ( color.rgb, vec3 ( 1.0 / gamma ) );    
    }
}