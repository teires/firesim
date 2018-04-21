#version 330 core

uniform sampler2D inVelocity;
uniform sampler2D inQuantity;

uniform float cellSize;
uniform float timeStep;

in vec2 Tex;
out vec4 FragColor;

void main()
{
	vec2 pastPos = Tex - (texture(inVelocity, Tex).xy * timeStep);
	pastPos = clamp(pastPos, vec2(0.f, 0.f), vec2(1.f, 1.f));

	vec2 L_B = clamp(pastPos + vec2(-cellSize * .25f, -cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 R_B = clamp(pastPos + vec2( cellSize * .25f, -cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 L_T = clamp(pastPos + vec2(-cellSize * .25f,  cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 R_T = clamp(pastPos + vec2( cellSize * .25f,  cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));

	
	vec3 colorB = mix(texture(inQuantity, L_B).rgb, texture(inQuantity, R_B).rgb, .5f);
	vec3 colorT = mix(texture(inQuantity, L_T).rgb, texture(inQuantity, R_T).rgb, .5f);
	vec3 res = mix(colorB, colorT, 0.5f);

	//foward
	vec2 nextPos = Tex + (texture(inVelocity, Tex).xy * timeStep);
	nextPos = clamp(nextPos, vec2(0.f, 0.f), vec2(1.f, 1.f));

	vec2 L_B_N = clamp(nextPos + vec2(-cellSize * .25f, -cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 R_B_N = clamp(nextPos + vec2( cellSize * .25f, -cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 L_T_N = clamp(nextPos + vec2(-cellSize * .25f,  cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));
	vec2 R_T_N = clamp(nextPos + vec2( cellSize * .25f,  cellSize * .25f), vec2(0.f, 0.f), vec2(1.f, 1.f));

	
	vec3 colorB2 = mix(texture(inQuantity, L_B_N).rgb, texture(inQuantity, R_B_N).rgb, .5f);
	vec3 colorT2 = mix(texture(inQuantity, L_T_N).rgb, texture(inQuantity, R_T_N).rgb, .5f);
	vec3 res2 = mix(colorB2, colorT2, 0.5f);
	
	//res = mix(res, res2, 0.5f);
	res = res + 0.5f * (texture(inVelocity, Tex).xyz - res2);
	FragColor = vec4(res, 1.f);
}