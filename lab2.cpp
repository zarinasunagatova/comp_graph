#include "lodepng.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>

using namespace std;

struct Vertex {
	float x, y, z;
	Vertex() : x(0), y(0), z(0) {}
	Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Normal {
	float x, y, z;
	Normal() : x(0), y(0), z(0) {}
	Normal(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct BarycentricCoords {
	float lambda0, lambda1, lambda2;
};

bool load_obj(const std::string& filename,
	std::vector<Vertex>& vertices,
	std::vector<std::vector<int>>& polygons) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Не удалось открыть файл: " << filename << std::endl;
		return false;
	}
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v") {
			float x, y, z;
			iss >> x >> y >> z;
			vertices.push_back(Vertex(x, y, z));
		}
		else if (type == "f") {
			std::vector<int> polygon;
			std::string part;
			while (iss >> part) {
				size_t slash_pos = part.find('/');
				int v_idx = std::stoi(part.substr(0, slash_pos)) - 1;
				polygon.push_back(v_idx);
			}
			polygons.push_back(polygon);
		}
	}
	file.close();
	std::cout << "Загружено вершин: " << vertices.size()
		<< ", полигонов: " << polygons.size() << std::endl;
	return true;
}

BarycentricCoords compute_barycentric(int x, int y,
	float x0, float y0,
	float x1, float y1,
	float x2, float y2) {

	BarycentricCoords coords;
	float denominator = (x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2);
	if (std::abs(denominator) < 1e-6f) {
		coords.lambda0 = -1.0f;
		coords.lambda1 = -1.0f;
		coords.lambda2 = -1.0f;
		return coords;
	}
	coords.lambda0 = ((x - x2) * (y1 - y2) - (x1 - x2) * (y - y2)) / denominator;
	coords.lambda1 = ((x0 - x2) * (y - y2) - (x - x2) * (y0 - y2)) / denominator;
	coords.lambda2 = 1.0f - coords.lambda0 - coords.lambda1;

	return coords;
}

void draw_triangle(std::vector<unsigned char>& img, int w, int h,
	float x0, float y0, float x1, float y1, float x2, float y2,
	unsigned char r, unsigned char g, unsigned char b,
	std::vector<float>* zbuffer = nullptr,
	float z0 = 0, float z1 = 0, float z2 = 0) {

	int xmin = (int)std::floor(std::min({ x0, x1, x2 }));
	int xmax = (int)std::ceil(std::max({ x0, x1, x2 }));
	int ymin = (int)std::floor(std::min({ y0, y1, y2 }));
	int ymax = (int)std::ceil(std::max({ y0, y1, y2 }));
	xmin = std::max(0, xmin);
	xmax = std::min(w - 1, xmax);
	ymin = std::max(0, ymin);
	ymax = std::min(h - 1, ymax);

	for (int y = ymin; y <= ymax; y++) {
		for (int x = xmin; x <= xmax; x++) {
			BarycentricCoords coords = compute_barycentric(x, y, x0, y0, x1, y1, x2, y2);

			if (coords.lambda0 >= 0 && coords.lambda1 >= 0 && coords.lambda2 >= 0) {
				bool draw_pixel = true;

				if (zbuffer != nullptr) {
					float z = coords.lambda0 * z0 + coords.lambda1 * z1 + coords.lambda2 * z2;
					size_t idx = y * w + x;

					if (z < (*zbuffer)[idx]) {
						(*zbuffer)[idx] = z;
					}
					else {
						draw_pixel = false;
					}
				}

				if (draw_pixel) {
					size_t idx = (y * w + x) * 3;
					img[idx + 0] = r;
					img[idx + 1] = g;
					img[idx + 2] = b;
				}
			}
		}
	}
}


void task9(const std::vector<Vertex>& vertices,
	const std::vector<std::vector<int>>& polygons,
	int img_size = 1000, float scale = 5000.0f,
	float offset_x = 500.0f, float offset_y = 500.0f) {
	std::cout << "Задание 9. Тестирование отрисовки треугольников\\n";
	std::vector<unsigned char> img(img_size * img_size * 3, 255);

	int triangles_drawn = 0;
	for (const auto& poly : polygons) {
		if (poly.size() != 3) continue;

		const Vertex& v0 = vertices[poly[0]];
		const Vertex& v1 = vertices[poly[1]];
		const Vertex& v2 = vertices[poly[2]];

		float x0 = scale * v0.x + offset_x;
		float y0 = img_size - 1 - (scale * v0.y + offset_y);
		float x1 = scale * v1.x + offset_x;
		float y1 = img_size - 1 - (scale * v1.y + offset_y);
		float x2 = scale * v2.x + offset_x;
		float y2 = img_size - 1 - (scale * v2.y + offset_y);

		unsigned char r = (triangles_drawn * 51) % 256;
		unsigned char g = (triangles_drawn * 85) % 256;
		unsigned char b = (triangles_drawn * 127) % 256;

		draw_triangle(img, img_size, img_size, x0, y0, x1, y1, x2, y2, r, g, b);
		triangles_drawn++;
	}

	lodepng::encode("9_triangles_test.png", img, img_size, img_size, LCT_RGB);
	std::cout << "Сохранено: 9_triangles_test.png (" << triangles_drawn << " треугольников)\\n";
}



Normal compute_normal(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
	float ux = v1.x - v2.x;
	float uy = v1.y - v2.y;
	float uz = v1.z - v2.z;

	float vx = v1.x - v0.x;
	float vy = v1.y - v0.y;
	float vz = v1.z - v0.z;

	Normal normal;
	normal.x = uy * vz - uz * vy;
	normal.y = uz * vx - ux * vz;
	normal.z = ux * vy - uy * vx;

	return normal;
}

Normal normalize(const Normal& n) {
	float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
	if (length > 1e-6f) {
		return Normal(n.x / length, n.y / length, n.z / length);
	}
	return Normal(0, 0, 0);
}

void render_model(const std::vector<Vertex>& vertices,
	const std::vector<std::vector<int>>& polygons,
	int img_size = 1000,
	float scale = 5000.0f,
	float offset_x = 500.0f,
	float offset_y = 500.0f,
	bool use_culling = false,
	bool use_lighting = false,
	bool use_zbuffer = false,
	const std::string& filename = "output.png") {

	std::vector<unsigned char> img(img_size * img_size * 3, 0);

	std::vector<float> zbuffer;
	if (use_zbuffer) {
		zbuffer.resize(img_size * img_size, std::numeric_limits<float>::max());
	}

	Normal light_dir(0, 0, 1);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> color_dist(0, 255);

	int drawn_polygons = 0;
	int culled_polygons = 0;

	for (const auto& poly : polygons) {
		if (poly.size() != 3) {
			continue;
		}

		const Vertex& v0 = vertices[poly[0]];
		const Vertex& v1 = vertices[poly[1]];
		const Vertex& v2 = vertices[poly[2]];

		Normal normal = compute_normal(v0, v1, v2);
		Normal normalized_normal = normalize(normal);

		float dot_product = normalized_normal.x * light_dir.x +
			normalized_normal.y * light_dir.y +
			normalized_normal.z * light_dir.z;

		if (use_culling && dot_product >= 0) {
			culled_polygons++;
			continue;
		}

		float x0 = scale * v0.x + offset_x;
		float y0 = img_size - 1 - (scale * v0.y + offset_y);
		float x1 = scale * v1.x + offset_x;
		float y1 = img_size - 1 - (scale * v1.y + offset_y);
		float x2 = scale * v2.x + offset_x;
		float y2 = img_size - 1 - (scale * v2.y + offset_y);

		unsigned char r, g, b;
		if (use_lighting) {
			float intensity = -dot_product;
			intensity = std::max(0.0f, std::min(1.0f, intensity));

			r = (unsigned char)(255 * intensity);
			g = 0;
			b = 0;
		}
		else {
			r = color_dist(gen);
			g = color_dist(gen);
			b = color_dist(gen);
		}

		if (use_zbuffer) {
			draw_triangle(img, img_size, img_size, x0, y0, x1, y1, x2, y2,
				r, g, b, &zbuffer, v0.z, v1.z, v2.z);
		}
		else {
			draw_triangle(img, img_size, img_size, x0, y0, x1, y1, x2, y2, r, g, b);
		}

		drawn_polygons++;
	}

	std::cout << "  Отрисовано полигонов: " << drawn_polygons << std::endl;
	if (use_culling) {
		std::cout << "  Отсечено нелицевых граней: " << culled_polygons << std::endl;
	}

	lodepng::encode(filename, img, img_size, img_size, LCT_RGB);
	std::cout << "  Сохранено: " << filename << std::endl;
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "Rus");

	std::string obj_filename = "model_1.obj";
	std::vector<Vertex> vertices;
	std::vector<std::vector<int>> polygons;

	std::cout << "Загрузка модели из файла: " << obj_filename << std::endl;
	if (!load_obj(obj_filename, vertices, polygons)) {
		std::cerr << "Ошибка: не удалось загрузить модель. Убедитесь, что файл "
			<< obj_filename << " существует.\\n";
		return 1;
	}

	float min_x = vertices[0].x, max_x = vertices[0].x;
	float min_y = vertices[0].y, max_y = vertices[0].y;
	float min_z = vertices[0].z, max_z = vertices[0].z;

	for (const auto& v : vertices) {
		min_x = std::min(min_x, v.x);
		max_x = std::max(max_x, v.x);
		min_y = std::min(min_y, v.y);
		max_y = std::max(max_y, v.y);
		min_z = std::min(min_z, v.z);
		max_z = std::max(max_z, v.z);
	}

	float model_width = max_x - min_x;
	float model_height = max_y - min_y;
	float model_center_x = (min_x + max_x) / 2;
	float model_center_y = (min_y + max_y) / 2;

	int img_size = 1000;
	float scale = img_size * 0.8f / std::max(model_width, model_height);
	float offset_x = img_size / 2.0f - model_center_x * scale;
	float offset_y = img_size / 2.0f - model_center_y * scale;

	task9(vertices, polygons, img_size, scale, offset_x, offset_y);
	render_model(vertices, polygons, img_size, scale, offset_x, offset_y,
		false, false, false, "10_random_colors.png");

	render_model(vertices, polygons, img_size, scale, offset_x, offset_y,
		true, false, false, "12_backface_culling.png");

	render_model(vertices, polygons, img_size, scale, offset_x, offset_y,
		true, true, false, "13_lighting.png");

	render_model(vertices, polygons, img_size, scale, offset_x, offset_y,
		true, true, true, "14_zbuffer.png");

	std::cout << "\\nВсе задания выполнены!" << std::endl;

	return 0;
}



