#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D colorTex;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

bool less(vec4 a, vec4 b) // intensity comparison
{
	float sum_a = a.x + a.y + a.z;
	float sum_b = b.x + b.y + b.z;
	return sum_a < sum_b;
}

void main()
{
	// get pixels with offsets
    vec4 pixels[9];
	pixels[0] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(-1, -1));
	pixels[1] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(-1,  0));
	pixels[2] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(-1,  1));
	pixels[3] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2( 0, -1));
	pixels[4] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2( 0,  0));
	pixels[5] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2( 0,  1));
	pixels[6] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2( 1, -1));
	pixels[7] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2( 1,  0));
	pixels[8] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2( 1,  1));
	
	// bubble sort by intensity
	bool swapped = false;
	for (int i = 0; i < 8; i++)
	{
		swapped = false;
		for (int j = 0; j < 8-i; j++)
		{
			if (less(pixels[j+1], pixels[j]))
			{
				vec4 tmp = pixels[j];
				pixels[j] = pixels[j+1];
				pixels[j+1] = tmp;
				
				swapped = true;
			}
		}
		
		if (!swapped)
			break;
	}
	
	// return pixel with median intensity
	color = pixels[4];
	// uncomment to see original texture with noise
	// color = textureLod(colorTex, surf.texCoord, 0);
}
