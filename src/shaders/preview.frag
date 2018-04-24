R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 tex_coord;
uniform sampler2D sampler;
uniform bool show_border;
uniform bool show_cursor;
void main() {
	float d_x = min(tex_coord.x, 1.0 - tex_coord.x);
	float d_y = min(tex_coord.y, 1.0 - tex_coord.y);
	float floor_dy = 1.0 - tex_coord.y;
	if (show_cursor) {
		if (show_border && (floor_dy < 0.05) ) {
		fragment_color = vec4(1.0, 0.0, 0.0, 1.0);
		} else {
			fragment_color = vec4(texture(sampler, tex_coord).xyz, 1.0);
		}

	} else {
		if (show_border && (d_x < 0.05 || d_y < 0.05) ) {
		fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
		} else {
			fragment_color = vec4(texture(sampler, tex_coord).xyz, 1.0);
		}
	}

}
)zzz"
