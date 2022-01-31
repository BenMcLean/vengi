uniform sampler2D u_texture;
$in vec2 v_texcoord;
$in vec4 v_color;
layout(location = 0) $out vec4 o_color;

void main() {
	vec2 texOffset = 1.0 / textureSize(u_texture, 0);
	vec2 tc0 = v_texcoord.st + vec2(-texOffset.x, -texOffset.y);
	vec2 tc1 = v_texcoord.st + vec2(         0.0, -texOffset.y);
	vec2 tc2 = v_texcoord.st + vec2(+texOffset.x, -texOffset.y);
	vec2 tc3 = v_texcoord.st + vec2(-texOffset.x,          0.0);
	vec2 tc4 = v_texcoord.st + vec2(         0.0,          0.0);
	vec2 tc5 = v_texcoord.st + vec2(+texOffset.x,          0.0);
	vec2 tc6 = v_texcoord.st + vec2(-texOffset.x, +texOffset.y);
	vec2 tc7 = v_texcoord.st + vec2(         0.0, +texOffset.y);
	vec2 tc8 = v_texcoord.st + vec2(+texOffset.x, +texOffset.y);

	vec4 col = vec4(0.0);
	col += $texture2D(u_texture, tc0);
	col += $texture2D(u_texture, tc1);
	col += $texture2D(u_texture, tc2);
	col += $texture2D(u_texture, tc3);
	vec4 textureColor = $texture2D(u_texture, tc4);
	col += textureColor;
	col += $texture2D(u_texture, tc5);
	col += $texture2D(u_texture, tc6);
	col += $texture2D(u_texture, tc7);
	col += $texture2D(u_texture, tc8);

	vec4 sum = 8.0 * textureColor - col;
	float alpha = 1.0;
	if (sum.rgb == vec3(0.0,0.0,0.0)) {
		alpha = 0.0;
	}
	o_color = vec4(sum.rgb, alpha) * v_color;
}
